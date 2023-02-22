
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"


MemHeap *st_heap_new(st_uint reserved_size);

bool st_heap_grow(MemHeap *heap, st_uint grow_size);

bool st_heap_shrink(MemHeap *heap, st_uint shrink_size);

void st_heap_destroy(MemHeap *heap);
