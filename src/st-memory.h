/*
 * st-memory.h
 *
 * Copyright (C) 2008 Vincent Geddes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#ifndef __ST_MEMORY__
#define __ST_MEMORY__

#include <st-types.h>
#include <st-identity-hashtable.h>
#include <st-heap.h>
#include <st-utils.h>
#include "ptr_array.h"

/* threshold is 8 Mb or 16 Mb depending on whether system is 32 or 64 bits */
#define ST_COLLECTION_THRESHOLD (sizeof (st_oop) * 2 * 1024 * 1024)


typedef struct st_memory
{
    st_heap   *heap;

    st_oop    *start, *end;
    st_oop    *p;

    st_oop    *mark_stack;
    st_uint    mark_stack_size;

    st_uchar  *mark_bits;
    st_uchar  *alloc_bits;
    st_uint    bits_size; /* in bytes */

    st_oop   **offsets;
    st_uint    offsets_size; /* in bytes */

    ptr_array  roots;
    st_uint    counter;

    /* free context pool */
    st_oop     free_context;

    /* statistics */
    struct timespec total_pause_time;     /* total accumulated pause time */
    st_ulong bytes_allocated;             /* current number of allocated bytes */
    st_ulong bytes_collected;             /* number of bytes collected in last compaction */

    st_identity_hashtable *ht;

} st_memory;

st_memory *st_memory_new             (void);
void       st_memory_destroy         (void);
void       st_memory_add_root        (st_oop object);
void       st_memory_remove_root     (st_oop object);
st_oop     st_memory_allocate        (st_uint size);

st_oop     st_memory_allocate_context (void);
void       st_memory_recycle_context  (st_oop context);

void       st_memory_perform_gc       (void);

st_oop     st_memory_remap_reference  (st_oop reference);

#endif /* __ST_MEMORY__ */
