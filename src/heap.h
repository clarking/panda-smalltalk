
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"

MemHeap *st_heap_new(uint reserved_size);

bool st_heap_grow(MemHeap *heap, uint grow_size);

bool st_heap_shrink(MemHeap *heap, uint shrink_size);

void st_heap_destroy(MemHeap *heap);

