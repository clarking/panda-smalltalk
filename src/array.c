
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "array.h"
#include "universe.h"
#include "utils.h"
#include "types.h"

Oop st_array_allocate(Oop class, uint size) {
	Oop array;
	Oop *elements;
	
	st_assert (size >= 0);
	
	array = st_memory_allocate(ST_SIZE_OOPS(Array) + size);
	if (array == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		array = st_memory_allocate(ST_SIZE_OOPS(Array) + size);
		st_assert (array != 0);
	}
	st_object_initialize_header(array, class);
	
	ST_ARRAYED_OBJECT (array)->size = st_smi_new(size);
	elements = ST_ARRAY (array)->elements;
	for (uint i = 0; i < size; i++)
		elements[i] = ST_NIL;
	
	return array;
}

/* ByteArray */
Oop st_byte_array_allocate(Oop class, int size) {
	uint size_oops;
	Oop array;
	
	st_assert (size >= 0);
	
	/* add 1 byte for NULL terminator. Allows toll-free bridging with C string function */
	size_oops = ST_ROUNDED_UP_OOPS (size + 1);
	
	array = st_memory_allocate(ST_SIZE_OOPS(ByteArray) + size_oops);
	if (array == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		array = st_memory_allocate(ST_SIZE_OOPS(Array) + size_oops);
		st_assert (array != 0);
	}
	
	st_object_initialize_header(array, class);
	
	ST_ARRAYED_OBJECT (array)->size = st_smi_new(size);
	memset(st_byte_array_bytes(array), 0, ST_OOPS_TO_BYTES (size_oops));
	
	return array;
}

bool st_byte_array_equal(Oop object, Oop other) {
	int size, size_other;
	
	if (st_object_class(other) != ST_BYTE_ARRAY_CLASS &&
	    st_object_class(other) != ST_STRING_CLASS &&
	    st_object_class(other) != ST_SYMBOL_CLASS)
		return false;
	
	size = st_smi_value(ST_ARRAYED_OBJECT (object)->size);
	size_other = st_smi_value(ST_ARRAYED_OBJECT (other)->size);
	
	if (size != size_other)
		return false;
	
	return memcmp(st_byte_array_bytes(object), st_byte_array_bytes(other), size) == 0;
}

uint st_byte_array_hash(Oop object) {
	return st_string_hash((char *) st_byte_array_bytes(object));
}

/* WordArray */
Oop st_word_array_allocate(Oop class, int size) {
	Oop array;
	int size_oops;
	uint *elements;
	
	st_assert(size >= 0);
	
	size_oops = size / (sizeof(Oop) / sizeof(uint));
	
	array = st_memory_allocate(ST_SIZE_OOPS(WordArray) + size_oops);
	if (array == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		array = st_memory_allocate(ST_SIZE_OOPS(Array) + size_oops);
		st_assert(array != 0);
	}
	st_object_initialize_header(array, class);
	
	ST_ARRAYED_OBJECT(array)->size = st_smi_new(size);
	elements = st_word_array_elements(array);
	for (int i = 0; i < size; i++)
		elements[i] = 0;
	
	return array;
}

/* FloatArray */
Oop st_float_array_allocate(Oop class, int size) {
	Oop object;
	double *elements;
	int size_oops;
	
	st_assert (size >= 0);
	
	/* get actual size in oops (dependent on whether system is 64bit or 32bit) */
	size_oops = size * (sizeof(double) / sizeof(Oop));
	
	object = st_memory_allocate(ST_SIZE_OOPS(FloatArray) + size_oops);
	if (object == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		object = st_memory_allocate(ST_SIZE_OOPS(Array) + size_oops);
		st_assert (object != 0);
	}
	st_object_initialize_header(object, class);
	
	ST_ARRAYED_OBJECT(object)->size = st_smi_new(size);
	
	elements = ST_FLOAT_ARRAY (object)->elements;
	for (int i = 0; i < size; i++)
		elements[i] = (double) 0;
	
	return object;
}


Oop st_arrayed_object_size(Oop object) {
	return ST_ARRAYED_OBJECT(object)->size;
}

Oop *st_array_elements(Oop object) {
	return ST_ARRAY (object)->elements;
}

Oop st_array_at(Oop object, int i) {
	return (ST_ARRAY(object)->elements - 1)[i];
}

void st_array_at_put(Oop object, int i, Oop value) {
	(ST_ARRAY(object)->elements - 1)[i] = value;
}

uint *st_word_array_elements(Oop object) {
	return ST_WORD_ARRAY(object)->elements;
}

uint st_word_array_at(Oop object, int i) {
	return ST_WORD_ARRAY(object)->elements[i - 1];
}

void st_word_array_at_put(Oop object, int i, uint value) {
	ST_WORD_ARRAY(object)->elements[i - 1] = value;
}

char *st_byte_array_bytes(Oop object) {
	return (char *) ST_BYTE_ARRAY (object)->bytes;
}

uchar st_byte_array_at(Oop object, int i) {
	return st_byte_array_bytes(object)[i - 1];
}

void st_byte_array_at_put(Oop object, int i, uchar value) {
	st_byte_array_bytes(object)[i - 1] = value;
}

double *st_float_array_elements(Oop array) {
	return ST_FLOAT_ARRAY(array)->elements;
}

double st_float_array_at(Oop array, int i) {
	return ST_FLOAT_ARRAY(array)->elements[i - 1];
}

void st_float_array_at_put(Oop array, int i, double value) {
	ST_FLOAT_ARRAY(array)->elements[i - 1] = value;
}
