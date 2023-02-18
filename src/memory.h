
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once


#include "st-types.h"
#include "st-identity-hashtable.h"
#include "st-heap.h"
#include "st-utils.h"

// RESERVE 1000 MB worth of virtual address space
#define RESERVED_SIZE        (1000 * 1024 * 1024)
#define INITIAL_COMMIT_SIZE  (1 * 1024 * 1024)

#define MARK_STACK_SIZE      (256 * 1024)
#define MARK_STACK_SIZE_OOPS (MARK_STACK_SIZE / sizeof (st_oop))

#define BLOCK_SIZE       256
#define BLOCK_SIZE_OOPS  (BLOCK_SIZE / sizeof (st_oop))


st_oop remap_oop(st_oop ref);

void garbage_collect();

void timer_start(struct timespec *spec);

void timer_stop(struct timespec *spec);

void verify(st_oop object);

void ensure_metadata(void);

void grow_heap(st_uint min_size_oops);

st_memory *st_memory_new(void);

void st_memory_destroy(void);

void st_memory_add_root(st_oop object);

void st_memory_remove_root(st_oop object);

st_oop st_memory_allocate(st_uint size);

st_oop st_memory_allocate_context(void);

void st_memory_recycle_context(st_oop context);

bool get_bit(st_uchar *bits, st_uint index);

void set_bit(st_uchar *bits, st_uint index);

st_uint bit_index(st_oop object);

bool ismarked(st_oop object);

void set_marked(st_oop object);

bool get_alloc_bit(st_oop object);

void set_alloc_bit(st_oop object);

void st_memory_perform_gc(void);

st_oop st_memory_remap_reference(st_oop reference);