/**
 * Copyright (c) 2012 Anup Patel.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file vmm_host_ram.c
 * @author Anup patel (anup@brainfault.org)
 * @brief Source file for RAM management.
 */

#include <arch_cpu.h>
#include <arch_board.h>
#include <vmm_error.h>
#include <vmm_string.h>
#include <vmm_stdio.h>
#include <vmm_host_aspace.h>
#include <mathlib.h>
#include <bitmap.h>

struct vmm_host_ram_ctrl {
	unsigned long *ram_bmap;
	u32 ram_bmap_sz;
	u32 ram_bmap_free;
	u32 ram_frame_count;
	physical_addr_t ram_start;
	physical_size_t ram_size;
};

static struct vmm_host_ram_ctrl rctrl;

int vmm_host_ram_alloc(physical_addr_t * pa, physical_size_t sz, bool aligned)
{
	u32 i, found, binc, bcnt, bpos, bfree;

	bcnt = 0;
	while (sz > 0) {
		bcnt++;
		if (sz > VMM_PAGE_SIZE) {
			sz -= VMM_PAGE_SIZE;
		} else {
			sz = 0;
		}
	}

	if (rctrl.ram_bmap_free < bcnt) {
		return VMM_EFAIL;
	}

	found = 0;
	if (aligned && (sz > VMM_PAGE_SIZE)) {
		bpos = umod32(rctrl.ram_start, sz);
		if (bpos) {
			bpos = VMM_SIZE_TO_PAGE(sz);
		}
		binc = bcnt;
	} else {
		bpos = 0;
		binc = 1;
	}
	for ( ; bpos < (rctrl.ram_size >> VMM_PAGE_SHIFT); bpos += binc) {
		bfree = 0;
		for (i = bpos; i < (bpos + bcnt); i++) {
			if (bitmap_isset(rctrl.ram_bmap, i)) {
				break;
			}
			bfree++;
		}
		if (bfree == bcnt) {
			found = 1;
			break;
		}
	}
	if (!found) {
		return VMM_EFAIL;
	}

	*pa = rctrl.ram_start + bpos * VMM_PAGE_SIZE;
	bitmap_set(rctrl.ram_bmap, bpos, bcnt);
	rctrl.ram_bmap_free -= bcnt;

	return VMM_OK;
}

int vmm_host_ram_reserve(physical_addr_t pa, physical_size_t sz)
{
	u32 i, bcnt, bpos, bfree;

	if ((pa < rctrl.ram_start) ||
	    ((rctrl.ram_start + rctrl.ram_size) <= pa)) {
		return VMM_EFAIL;
	}

	bcnt = 0;
	while (sz > 0) {
		bcnt++;
		if (sz > VMM_PAGE_SIZE) {
			sz -= VMM_PAGE_SIZE;
		} else {
			sz = 0;
		}
	}

	if (rctrl.ram_bmap_free < bcnt) {
		return VMM_EFAIL;
	}

	bpos = (pa - rctrl.ram_start) >> VMM_PAGE_SHIFT;
	bfree = 0;
	for (i = bpos; i < (bpos + bcnt); i++) {
		if (bitmap_isset(rctrl.ram_bmap, i)) {
			break;
		}
		bfree++;
	}

	if (bfree != bcnt) {
		return VMM_EFAIL;
	}

	bitmap_set(rctrl.ram_bmap, bpos, bcnt);
	rctrl.ram_bmap_free -= bcnt;

	return VMM_OK;
}

int vmm_host_ram_free(physical_addr_t pa, physical_size_t sz)
{
	u32 bcnt, bpos;

	if (pa < rctrl.ram_start ||
	    (rctrl.ram_start + rctrl.ram_size) <= pa) {
		return VMM_EFAIL;
	}

	bcnt = 0;
	while (sz > 0) {
		bcnt++;
		if (sz > VMM_PAGE_SIZE) {
			sz -= VMM_PAGE_SIZE;
		} else {
			sz = 0;
		}
	}

	bpos = (pa - rctrl.ram_start) >> VMM_PAGE_SHIFT;

	bitmap_clear(rctrl.ram_bmap, bpos, bcnt);
	rctrl.ram_bmap_free += bcnt;

	return VMM_OK;
}

physical_addr_t vmm_host_ram_base(void)
{
	return rctrl.ram_start;
}

bool vmm_host_ram_frame_isfree(physical_addr_t pa)
{
	u32 bpos;

	if (pa < rctrl.ram_start ||
	    (rctrl.ram_start + rctrl.ram_size) <= pa) {
		return TRUE;
	}

	bpos = (pa - rctrl.ram_start) >> VMM_PAGE_SHIFT;

	if (bitmap_isset(rctrl.ram_bmap, bpos)) {
		return FALSE;
	}

	return TRUE;
}

u32 vmm_host_ram_free_frame_count(void)
{
	return rctrl.ram_bmap_free;
}

u32 vmm_host_ram_total_frame_count(void)
{
	return rctrl.ram_frame_count;
}

physical_size_t vmm_host_ram_size(void)
{
	return rctrl.ram_size;
}

virtual_size_t vmm_host_ram_estimate_hksize(physical_size_t ram_size)
{
	return bitmap_estimate_size(ram_size >> VMM_PAGE_SHIFT);
}

int __init vmm_host_ram_init(physical_addr_t base, 
			     physical_size_t size,
			     virtual_addr_t hkbase, 
			     physical_addr_t resv_pa, 
			     virtual_size_t resv_sz)
{
	int start, last, max;

	vmm_memset(&rctrl, 0, sizeof(rctrl));

	rctrl.ram_start = base;
	rctrl.ram_size = size;
	rctrl.ram_start &= ~VMM_PAGE_MASK;
	rctrl.ram_size &= ~VMM_PAGE_MASK;
	rctrl.ram_frame_count = rctrl.ram_size >> VMM_PAGE_SHIFT;
	rctrl.ram_bmap = (unsigned long *)hkbase;
	rctrl.ram_bmap_sz = bitmap_estimate_size(rctrl.ram_frame_count);
	rctrl.ram_bmap_free = rctrl.ram_frame_count;

	bitmap_zero(rctrl.ram_bmap, rctrl.ram_frame_count);

	max = ((rctrl.ram_start + rctrl.ram_size) >> VMM_PAGE_SHIFT);
	start = ((resv_pa - rctrl.ram_start) >> VMM_PAGE_SHIFT);
	last = start + (resv_sz >> VMM_PAGE_SHIFT);
	last = (last < max) ? last : max;
	bitmap_set(rctrl.ram_bmap, start, last - start);
	rctrl.ram_bmap_free -=  last - start;

	return VMM_OK;
}

