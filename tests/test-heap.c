

/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <compiler.h>
#include <universe.h>
#include <system.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	
	MemHeap *heap;
	heap = st_heap_new(1024 * 1024 * 1024);
	if (!heap)
		abort();
	
	st_assert (st_heap_grow(heap, 512 * 1024 * 1024));
	st_assert (st_heap_grow(heap, 256 * 1024 * 1024));
	st_assert (st_heap_shrink(heap, 512 * 1024 * 1024));
	while (1)
		sleep(1);
	
	return 0;
}

