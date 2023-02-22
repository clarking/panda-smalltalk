
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once


#include "types.h"
#include "identity-hashtable.h"
#include "heap.h"
#include "utils.h"

// RESERVE 1000 MB worth of virtual address space
#define RESERVED_SIZE        (1000 * 1024 * 1024)
#define INITIAL_COMMIT_SIZE  (1 * 1024 * 1024)

#define MARK_STACK_SIZE      (256 * 1024)
#define MARK_STACK_SIZE_OOPS (MARK_STACK_SIZE / sizeof (Oop))

#define BLOCK_SIZE       256
#define BLOCK_SIZE_OOPS  (BLOCK_SIZE / sizeof (Oop))


Oop remap_oop(Oop ref);

void garbage_collect();

void timer_start(struct timespec *spec);

void timer_stop(struct timespec *spec);

void verify(Oop object);

void ensure_metadata(void);

void grow_heap(uint min_size_oops);

ObjMemory *st_memory_new(void);

void st_memory_destroy(void);

void st_memory_add_root(Oop object);

void st_memory_remove_root(Oop object);

Oop st_memory_allocate(uint size);

Oop st_memory_allocate_context(void);

void st_memory_recycle_context(Oop context);

bool get_bit(uchar *bits, uint index);

void set_bit(uchar *bits, uint index);

uint bit_index(Oop object);

bool ismarked(Oop object);

void set_marked(Oop object);

bool get_alloc_bit(Oop object);

void set_alloc_bit(Oop object);

void st_memory_perform_gc(void);

Oop st_memory_remap_reference(Oop reference);