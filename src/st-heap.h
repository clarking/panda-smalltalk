
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

typedef struct st_heap {
	st_uchar *start; /* start of reserved address space */
	st_uchar *p;     /* end of committed address space (`start' to `p' is thus writeable memory) */
	st_uchar *end;   /* end of reserved address space */
} st_heap;

st_heap *st_heap_new(st_uint reserved_size);

bool st_heap_grow(st_heap *heap, st_uint grow_size);

bool st_heap_shrink(st_heap *heap, st_uint shrink_size);

void st_heap_destroy(st_heap *heap);
