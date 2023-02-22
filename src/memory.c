
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "memory.h"
#include "utils.h"
#include "float.h"
#include "large-integer.h"
#include "object.h"
#include "array.h"
#include "context.h"
#include "method.h"
#include "handle.h"


void timer_start(struct timespec *spec) {
	//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, spec);
}

void timer_stop(struct timespec *spec) {
	struct timespec tmp;
	//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tmp);
	st_timespec_difference(spec, &tmp, spec);
}

void verify(Oop object) {
	st_assert (st_object_is_mark(ST_OBJECT_MARK(object)));
}

void ensure_metadata(void) {
	/* The bit arrays are implemented using bytes as smallest element of storage.
	 * If there are N oops in the heap, then we need ((N + 7) / 8) bytes
	 * for a bit array with 32 elements. We need to reserve space for
	 * two bit arrays (mark, alloc), as well as the offsets array.
	 */
	uint size, bits_size, offsets_size;
	uchar *metadata_start;
	
	size = memory->end - memory->start;
	bits_size = ((size + 7) / 8);
	offsets_size = (size / BLOCK_SIZE_OOPS) * sizeof(Oop *);
	
	st_free(memory->mark_bits);
	st_free(memory->alloc_bits);
	st_free(memory->offsets);
	
	memory->mark_bits = st_malloc(bits_size);
	memory->alloc_bits = st_malloc(bits_size);
	memory->bits_size = bits_size;
	
	memory->offsets = st_malloc(offsets_size);
	memory->offsets_size = offsets_size;
}

void grow_heap(uint min_size_oops) {
	/* we grow the heap by roughly 0.25 or size_oops, whichever is larger */
	
	uint size, grow_size;
	MemHeap *heap;
	
	heap = memory->heap;
	size = heap->p - heap->start;
	grow_size = MAX (size / 4, min_size_oops * sizeof(Oop));
	
	st_heap_grow(heap, grow_size);
	
	memory->start = (Oop *) heap->start;
	memory->end = (Oop *) heap->p;
	
	ensure_metadata();
}

ObjMemory *st_memory_new(void) {
	Oop *ptr;
	ulong size_bits;
	MemHeap *heap;
	
	heap = st_heap_new(RESERVED_SIZE);
	if (!heap)
		abort();
	
	if (!st_heap_grow(heap, INITIAL_COMMIT_SIZE))
		abort();
	
	memory = st_new0 (ObjMemory);
	
	memory->heap = heap;
	memory->start = (Oop *) heap->start;
	memory->end = (Oop *) heap->p;
	memory->p = memory->start;
	
	memory->roots = PtrArray_new(15);
	
	memory->total_pause_time.tv_sec = 0;
	memory->total_pause_time.tv_nsec = 0;
	memory->counter = 0;
	
	memory->mark_stack = st_malloc(MARK_STACK_SIZE);
	memory->mark_stack_size = MARK_STACK_SIZE;
	
	memory->mark_bits = NULL;
	memory->alloc_bits = NULL;
	memory->offsets = NULL;
	
	memory->free_context = 0;
	
	memory->ht = st_identity_hashtable_new();
	
	ensure_metadata();
	
	return memory;
}

void st_memory_destroy(void) {
	st_free(memory);
}

void st_memory_add_root(Oop object) {
	PtrArray_append(memory->roots, (void *) object);
}

void st_memory_remove_root(Oop object) {
	PtrArray_remove_fast(memory->roots, (void *) object);
}

Oop st_memory_allocate(uint size) {
	Oop *chunk;
	
	st_assert (size >= 2);
	
	if (memory->counter > ST_COLLECTION_THRESHOLD)
		return 0;
	if ((memory->p + size) >= memory->end)
		grow_heap(size);
	
	chunk = memory->p;
	memory->p += size;
	memory->counter += (size * sizeof(Oop));
	
	return st_tag_pointer(chunk);
}

Oop st_memory_allocate_context(void) {
	Oop context;
	
	if (ST_LIKELY (memory->free_context)) {
		context = memory->free_context;
		memory->free_context = ContextPart_SENDER (memory->free_context);
		return context;
	}
	
	context = st_memory_allocate(ST_SIZE_OOPS(struct MethodContext) + 32);
	if (context == 0) {
		st_memory_perform_gc();
		context = st_memory_allocate(ST_SIZE_OOPS(struct MethodContext) + 32);
		st_assert (context != 0);
	}
	st_object_initialize_header(context, MethodContext_CLASS);
	
	return context;
}

void st_memory_recycle_context(Oop context) {
	ContextPart_SENDER (context) = memory->free_context;
	memory->free_context = context;
}

bool get_bit(uchar *bits, uint index) {
	return (bits[index >> 3] >> (index & 0x7)) & 1;
}

void set_bit(uchar *bits, uint index) {
	bits[index >> 3] |= 1 << (index & 0x7);
}

uint bit_index(Oop object) {
	return st_detag_pointer(object) - memory->start;
}

bool ismarked(Oop object) {
	return get_bit(memory->mark_bits, bit_index(object));
}

void set_marked(Oop object) {
	set_bit(memory->mark_bits, bit_index(object));
}

bool get_alloc_bit(Oop object) {
	return get_bit(memory->alloc_bits, bit_index(object));
}

void set_alloc_bit(Oop object) {
	set_bit(memory->alloc_bits, bit_index(object));
}

uint get_block_index(Oop *object) {
	return (object - memory->start) / BLOCK_SIZE_OOPS;
}

uint object_size(Oop object) {
	switch (st_object_format(object)) {
		case ST_FORMAT_OBJECT:
			return ST_SIZE_OOPS(struct ObjHeader) + st_object_instance_size(object);
		case ST_FORMAT_FLOAT:
			return ST_SIZE_OOPS(struct Float);
		case ST_FORMAT_LARGE_INTEGER:
			return ST_SIZE_OOPS(struct LargeInt);
		case ST_FORMAT_HANDLE:
			return ST_SIZE_OOPS(struct Handle);
		case ST_FORMAT_ARRAY:
			return ST_SIZE_OOPS(struct ArrayedObject) + st_smi_value(st_arrayed_object_size(object));
		case ST_FORMAT_BYTE_ARRAY:
			return ST_SIZE_OOPS(struct ArrayedObject) +
			       ST_ROUNDED_UP_OOPS (st_smi_value(st_arrayed_object_size(object)) + 1);
		case ST_FORMAT_WORD_ARRAY:
			return ST_SIZE_OOPS(struct ArrayedObject)
			       + (st_smi_value(st_arrayed_object_size(object)) / (sizeof(Oop) / sizeof(uint)));
		case ST_FORMAT_FLOAT_ARRAY:
			return ST_SIZE_OOPS(struct ArrayedObject) +
			       (st_smi_value(st_arrayed_object_size(object)) * ST_SIZE_OOPS(double));
		case ST_FORMAT_INTEGER_ARRAY:
			/* object format not used yet */
			abort();
			break;
		case ST_FORMAT_CONTEXT:
			return ST_SIZE_OOPS(struct ObjHeader) + st_object_instance_size(object) + 32;
	}
	/* should not reach */
	abort();
	return 0;
}

void object_contents(Oop object, Oop **oops, uint *size) {
	switch (st_object_format(object)) {
		case ST_FORMAT_OBJECT:
			*oops = ST_OBJECT_FIELDS(object);
			*size = st_object_instance_size(object);
			break;
		case ST_FORMAT_ARRAY:
			*oops = st_array_elements(object);
			*size = st_smi_value(st_arrayed_object_size(object));
			break;
		case ST_FORMAT_CONTEXT:
			*oops = ST_OBJECT_FIELDS(object);
			*size = st_object_instance_size(object) + st_smi_value(ContextPart_SP (object));
			break;
		case ST_FORMAT_FLOAT:
		case ST_FORMAT_LARGE_INTEGER:
		case ST_FORMAT_HANDLE:
		case ST_FORMAT_BYTE_ARRAY:
		case ST_FORMAT_WORD_ARRAY:
		case ST_FORMAT_FLOAT_ARRAY:
		case ST_FORMAT_INTEGER_ARRAY:
			*oops = NULL;
			*size = 0;
			break;
		default:
			/* should not reach */
			abort();
	}
}

uint compute_ordinal_number(ObjMemory *memory, Oop ref) {
	uint i, k, ordinal;
	
	ordinal = 0;
	k = bit_index(ref);
	i = k & ~(BLOCK_SIZE_OOPS - 1);
	
	while (true) {
		if (get_bit(memory->mark_bits, i)) {
			ordinal++;
			if (i == k)
				return ordinal;
		}
		i++;
	}
}

Oop remap_oop(Oop ref) {
	uint b, i = 0, j = 0;
	uint ordinal;
	Oop *offset;
	
	if (!st_object_is_heap(ref) || ref == ST_NIL)
		return ref;
	
	ordinal = compute_ordinal_number(memory, ref);
	offset = memory->offsets[get_block_index(st_detag_pointer(ref))];
	b = bit_index(st_tag_pointer(offset));
	
	while (true) {
		if (get_bit(memory->alloc_bits, b + i)) {
			if (++j == ordinal) {
				return st_tag_pointer(offset + i);
			}
		}
		i++;
	}
	
	abort();
	return 0;
}

void st_memory_remap(void) {
	/* Remaps all object references in the heap */
	Oop *oops, *p;
	uint size;
	
	p = memory->start;
	while (p < memory->p) {
		p[1] = remap_oop(p[1]);
		object_contents(st_tag_pointer(p), &oops, &size);
		for (uint i = 0; i < size; i++) {
			oops[i] = remap_oop(oops[i]);
		}
		p += object_size(st_tag_pointer(p));
	}
}

void basic_finalize(Oop object) {
	if (ST_UNLIKELY(st_object_format(object) == ST_FORMAT_LARGE_INTEGER))
		mp_clear(st_large_integer_value(object));
}

void st_memory_compact(void) {
	Oop *p, *from, *to;
	uint size;
	uint block = 0;
	
	p = memory->start;
	
	while (ismarked(st_tag_pointer(p)) && p < memory->p) {
		set_alloc_bit(st_tag_pointer(p));
		if (block < (get_block_index(p) + 1)) {
			block = get_block_index(p);
			memory->offsets[block] = p;
			block += 1;
		}
		p += object_size(st_tag_pointer(p));
	}
	to = p;
	
	while (!ismarked(st_tag_pointer(p)) && p < memory->p) {
		basic_finalize(st_tag_pointer(p));
		p += object_size(st_tag_pointer(p));
	}
	from = p;
	
	if (from == to && p >= memory->p)
		goto out;
	
	while (from < memory->p) {
		
		if (ismarked(st_tag_pointer(from))) {
			
			if (st_object_is_hashed(st_tag_pointer(from)))
				st_identity_hashtable_rehash_object(memory->ht, st_tag_pointer(from), st_tag_pointer(to));
			
			set_alloc_bit(st_tag_pointer(to));
			size = object_size(st_tag_pointer(from));
			st_oops_move(to, from, size);
			
			if (block < (get_block_index(from) + 1)) {
				block = get_block_index(from);
				memory->offsets[block] = to;
				block += 1;
			}
			
			to += size;
			from += size;
		}
		else {
			basic_finalize(st_tag_pointer(from));
			if (st_object_is_hashed(st_tag_pointer(from)))
				st_identity_hashtable_remove(memory->ht, st_tag_pointer(from));
			from += object_size(st_tag_pointer(from));
		}
	}
	
	out:
	
	memory->bytes_collected = (memory->p - to) * sizeof(Oop);
	memory->bytes_allocated -= memory->bytes_collected;
	memory->p = to;
}

uint grow_marking_stack(void) {
	memory->mark_stack_size *= 2;
	memory->mark_stack = st_realloc(memory->mark_stack, memory->mark_stack_size);
	
	return memory->mark_stack_size / sizeof(Oop);
}

void st_memory_mark(void) {
	Oop object;
	Oop *oops, *stack;
	uint size, stack_size, sp;
	
	sp = 0;
	stack = memory->mark_stack;
	stack_size = memory->mark_stack_size / sizeof(Oop);
	
	for (uint i = 0; i < memory->roots->length; i++)
		stack[sp++] = (Oop) PtrArray_get_index(memory->roots, i);
	stack[sp++] = __machine.context;
	
	while (sp > 0) {
		object = stack[--sp];
		if (!st_object_is_heap(object) || ismarked(object))
			continue;
		
		set_marked(object);
		if (ST_UNLIKELY(sp >= stack_size)) {
			stack_size = grow_marking_stack();
			stack = memory->mark_stack;
			st_log("gc", "increased size of marking stack");
		}
		stack[sp++] = ST_OBJECT_CLASS(object);
		object_contents(object, &oops, &size);
		for (uint i = 0; i < size; i++) {
			if (ST_UNLIKELY(sp >= stack_size)) {
				stack_size = grow_marking_stack();
				stack = memory->mark_stack;
				st_log("gc", "increased size of marking stack");
			}
			if (oops[i] != ST_NIL) {
				stack[sp++] = oops[i];
			}
		}
	}
}

void remap_machine(VirtualMachine *machine) {
	Oop context, home;
	
	context = remap_oop(machine->context);
	if (ST_OBJECT_CLASS(context) == BlockContext_CLASS) {
		home = BlockContext_HOME (context);
		machine->method = MethodContext_METHOD (home);
		machine->receiver = MethodContext_RECEIVER (home);
		machine->temps = MethodContext_STACK (home);
		machine->stack = BlockContext_STACK (context);
	}
	else {
		machine->method = MethodContext_METHOD (context);
		machine->receiver = MethodContext_RECEIVER (context);
		machine->temps = MethodContext_STACK (context);
		machine->stack = MethodContext_STACK (context);
	}
	
	machine->context = context;
	machine->bytecode = st_method_bytecode_bytes(machine->method);
	machine->message_receiver = remap_oop(machine->message_receiver);
	machine->message_selector = remap_oop(machine->message_selector);
	machine->new_method = remap_oop(machine->new_method);
}

void remap_globals(void) {
	uint i;
	
	for (i = 0; i < ST_N_ELEMENTS (__machine.globals); i++)
		__machine.globals[i] = remap_oop(__machine.globals[i]);
	
	for (i = 0; i < ST_N_ELEMENTS (__machine.selectors); i++)
		__machine.selectors[i] = remap_oop(__machine.selectors[i]);
	
	for (i = 0; i < memory->roots->length; i++) {
		PtrArray_set_index(memory->roots,
		                    i,
		                    (void *) remap_oop((Oop) PtrArray_get_index(memory->roots, i)));
	}
}

void clear_metadata(void) {
	memset(memory->mark_bits, 0, memory->bits_size);
	memset(memory->alloc_bits, 0, memory->bits_size);
	memset(memory->offsets, 0, memory->offsets_size);
}

void st_memory_perform_gc(void) {
	garbage_collect();
}

void garbage_collect(void) {
	double times[3];
	struct timespec tm;
	
	/* clear context pool */
	memory->free_context = 0;
	memory->bytes_allocated += memory->counter;
	
	clear_metadata();
	
	/* marking */
	timer_start(&tm);
	st_memory_mark();
	timer_stop(&tm);
	
	times[0] = st_timespec_to_double_seconds(&tm);
	st_timespec_add(&memory->total_pause_time, &tm, &memory->total_pause_time);
	
	/* compaction */
	timer_start(&tm);
	st_memory_compact();
	timer_stop(&tm);
	
	times[1] = st_timespec_to_double_seconds(&tm);
	st_timespec_add(&memory->total_pause_time, &tm, &memory->total_pause_time);
	
	/* remapping */
	timer_start(&tm);
	st_memory_remap();
	remap_globals();
	remap_machine(&__machine);
	timer_stop(&tm);
	
	times[2] = st_timespec_to_double_seconds(&tm);
	st_timespec_add(&memory->total_pause_time, &tm, &memory->total_pause_time);
	
	st_machine_clear_caches(&__machine);
	memory->counter = 0;
	
	st_log("gc", "\n"
	             "collected:       %uK\n"
	             "heapSize:        %uK\n"
	             "marking time:    %.6fs\n"
	             "compaction time: %.6fs\n"
	             "remapping time:  %.6fs\n",
	       memory->bytes_collected / 1024,
	       (memory->bytes_collected + memory->bytes_allocated) / 1024,
	       times[0], times[1], times[2]);
}

Oop st_memory_remap_reference(Oop reference) {
	return remap_oop(reference);
}