
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "heap.h"
#include "utils.h"
#include "system.h"

#define PAGE_SIZE (st_system_pagesize ())

static inline uint round_pagesize(uint size) {
	return ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
}

MemHeap *st_heap_new(uint reserved_size) {
	
	// Create a new heap with a reserved address space.
	void *result;
	MemHeap *heap;
	uint size;
	
	st_assert (reserved_size > 0);
	size = round_pagesize(reserved_size);
	result = st_system_reserve_memory(NULL, size);
	if (result == NULL)
		return NULL;
	heap = st_new0 (MemHeap);
	heap->start = result;
	heap->end = result + size;
	heap->p = result;
	return heap;
}

bool st_heap_grow(MemHeap *heap, uint grow_size) {
	
	// Grows the heap by the specified amount (in bytes).
	// The primitive will not succeed if the heap runs out of reserved address space.
	void *result;
	uint size;
	st_assert (grow_size > 0);
	size = round_pagesize(grow_size);
	
	if ((heap->p + size) >= heap->end)
		return false;
	
	result = st_system_commit_memory(heap->p, size);
	if (result == NULL)
		return false;
	
	heap->p += size;
	return true;
}

bool st_heap_shrink(MemHeap *heap, uint shrink_size) {
	// Shrinks the heap by the specified amount (in bytes).
	void *result;
	uint size;
	
	st_assert (shrink_size > 0);
	size = round_pagesize(shrink_size);
	
	if ((heap->p - size) < heap->start)
		return false;
	
	result = st_system_decommit_memory(heap->p - size, size);
	if (result == NULL)
		return false;
	
	heap->p -= size;
	return true;
}

void st_heap_destroy(MemHeap *heap) {
	st_system_release_memory(heap->start, heap->end - heap->start);
	st_free(heap);
}