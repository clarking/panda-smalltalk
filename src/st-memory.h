
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <ptr_array.h>
#include "st-types.h"
#include "st-identity-hashtable.h"
#include "st-heap.h"
#include "st-utils.h"

/* threshold is 8 Mb or 16 Mb depending on whether system is 32 or 64 bits */
#define ST_COLLECTION_THRESHOLD (sizeof (st_oop) * 2 * 1024 * 1024)

typedef struct st_memory {
	st_heap *heap;
	
	st_oop *start, *end;
	st_oop *p;
	
	st_oop *mark_stack;
	st_uint mark_stack_size;
	
	st_uchar *mark_bits;
	st_uchar *alloc_bits;
	st_uint bits_size; /* in bytes */
	
	st_oop **offsets;
	st_uint offsets_size; /* in bytes */
	
	ptr_array roots;
	st_uint counter;
	
	/* free context pool */
	st_oop free_context;
	
	/* statistics */
	struct timespec total_pause_time;     /* total accumulated pause time */
	st_ulong bytes_allocated;             /* current number of allocated bytes */
	st_ulong bytes_collected;             /* number of bytes collected in last compaction */
	
	st_identity_hashtable *ht;
	
} st_memory;

st_memory *st_memory_new(void);

void st_memory_destroy(void);

void st_memory_add_root(st_oop object);

void st_memory_remove_root(st_oop object);

st_oop st_memory_allocate(st_uint size);

st_oop st_memory_allocate_context(void);

void st_memory_recycle_context(st_oop context);

void st_memory_perform_gc(void);

st_oop st_memory_remap_reference(st_oop reference);

