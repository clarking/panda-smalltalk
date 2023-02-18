
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "st-array.h"
#include "st-universe.h"
#include "st-utils.h"

st_oop st_array_allocate(st_oop class, st_uint size) {
	st_oop array;
	st_oop *elements;
	
	st_assert (size >= 0);
	
	array = st_memory_allocate(ST_SIZE_OOPS (struct st_array) + size);
	if (array == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		array = st_memory_allocate(ST_SIZE_OOPS (struct st_array) + size);
		st_assert (array != 0);
	}
	st_object_initialize_header(array, class);
	
	ST_ARRAYED_OBJECT (array)->size = st_smi_new(size);
	elements = ST_ARRAY (array)->elements;
	for (st_uint i = 0; i < size; i++)
		elements[i] = ST_NIL;
	
	return array;
}

/* ByteArray */
st_oop st_byte_array_allocate(st_oop class, int size) {
	st_uint size_oops;
	st_oop array;
	
	st_assert (size >= 0);
	
	/* add 1 byte for NULL terminator. Allows toll-free bridging with C string function */
	size_oops = ST_ROUNDED_UP_OOPS (size + 1);
	
	array = st_memory_allocate(ST_SIZE_OOPS (struct st_byte_array) + size_oops);
	if (array == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		array = st_memory_allocate(ST_SIZE_OOPS (struct st_array) + size_oops);
		st_assert (array != 0);
	}
	
	st_object_initialize_header(array, class);
	
	ST_ARRAYED_OBJECT (array)->size = st_smi_new(size);
	memset(st_byte_array_bytes(array), 0, ST_OOPS_TO_BYTES (size_oops));
	
	return array;
}

bool st_byte_array_equal(st_oop object, st_oop other) {
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

st_uint st_byte_array_hash(st_oop object) {
	return st_string_hash((char *) st_byte_array_bytes(object));
}

/* WordArray */
st_oop st_word_array_allocate(st_oop class, int size) {
	st_oop array;
	int size_oops;
	st_uint *elements;
	
	st_assert (size >= 0);
	
	size_oops = size / (sizeof(st_oop) / sizeof(st_uint));
	
	array = st_memory_allocate(ST_SIZE_OOPS (struct st_word_array) + size_oops);
	if (array == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		array = st_memory_allocate(ST_SIZE_OOPS (struct st_array) + size_oops);
		st_assert (array != 0);
	}
	st_object_initialize_header(array, class);
	
	ST_ARRAYED_OBJECT (array)->size = st_smi_new(size);
	elements = st_word_array_elements(array);
	for (int i = 0; i < size; i++)
		elements[i] = 0;
	
	return array;
}

/* FloatArray */
st_oop st_float_array_allocate(st_oop class, int size) {
	st_oop object;
	double *elements;
	int size_oops;
	
	st_assert (size >= 0);
	
	/* get actual size in oops (dependent on whether system is 64bit or 32bit) */
	size_oops = size * (sizeof(double) / sizeof(st_oop));
	
	object = st_memory_allocate(ST_SIZE_OOPS (struct st_float_array) + size_oops);
	if (object == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		object = st_memory_allocate(ST_SIZE_OOPS (struct st_array) + size_oops);
		st_assert (object != 0);
	}
	st_object_initialize_header(object, class);
	
	ST_ARRAYED_OBJECT (object)->size = st_smi_new(size);
	
	elements = ST_FLOAT_ARRAY (object)->elements;
	for (int i = 0; i < size; i++)
		elements[i] = (double) 0;
	
	return object;
}


st_oop st_arrayed_object_size(st_oop object) {
	return ST_ARRAYED_OBJECT (object)->size;
}

st_oop *st_array_elements(st_oop object) {
	return ST_ARRAY (object)->elements;
}

st_oop st_array_at(st_oop object, int i) {
	return (ST_ARRAY (object)->elements - 1)[i];
}

void st_array_at_put(st_oop object, int i, st_oop value) {
	(ST_ARRAY (object)->elements - 1)[i] = value;
}

st_uint *st_word_array_elements(st_oop object) {
	return ST_WORD_ARRAY (object)->elements;
}

st_uint st_word_array_at(st_oop object, int i) {
	return ST_WORD_ARRAY (object)->elements[i - 1];
}

void st_word_array_at_put(st_oop object, int i, st_uint value) {
	ST_WORD_ARRAY (object)->elements[i - 1] = value;
}

char *st_byte_array_bytes(st_oop object) {
	return (char *) ST_BYTE_ARRAY (object)->bytes;
}

st_uchar st_byte_array_at(st_oop object, int i) {
	return st_byte_array_bytes(object)[i - 1];
}

void st_byte_array_at_put(st_oop object, int i, st_uchar value) {
	st_byte_array_bytes(object)[i - 1] = value;
}

double *st_float_array_elements(st_oop array) {
	return ST_FLOAT_ARRAY (array)->elements;
}

double st_float_array_at(st_oop array, int i) {
	return ST_FLOAT_ARRAY (array)->elements[i - 1];
}

void st_float_array_at_put(st_oop array, int i, double value) {
	ST_FLOAT_ARRAY (array)->elements[i - 1] = value;
}
