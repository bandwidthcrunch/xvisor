/**
 * Copyright (c) 2012 Sukanto Ghosh.
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
 * @file arm_heap.c
 * @author Sukanto Ghosh (sukantoghosh@gmail.com)
 * @brief source file for heap managment
 *
 * Adapted from tests/arm32/pb-a8/basic/arm_heap.c
 *
 */

#include <arm_heap.h>

void * heap_curr;
extern u8 _heap_start;

void * arm_malloc(size_t size)
{
	void * retval;
	retval = heap_curr;
	heap_curr += size;
	return retval;
}

void arm_heap_init(void)
{
	heap_curr = &_heap_start;
}

