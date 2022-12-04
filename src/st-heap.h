
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"


st_heap *st_heap_new(st_uint reserved_size);

bool st_heap_grow(st_heap *heap, st_uint grow_size);

bool st_heap_shrink(st_heap *heap, st_uint shrink_size);

void st_heap_destroy(st_heap *heap);
