
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "large-integer.h"
#include "universe.h"
#include "types.h"

#define VALUE(oop)  (&(ST_LARGE_INTEGER(oop)->value))

Oop st_large_integer_new_from_string(const char *string, uint radix) {
	mp_int value;
	int result;
	
	st_assert (string != NULL);
	
	result = mp_init(&value);
	if (result != MP_OKAY)
		goto out;
	
	result = mp_read_radix(&value, string, radix);
	if (result != MP_OKAY)
		goto out;
	
	return st_large_integer_new(&value);
	
	out:
	mp_clear(&value);
	fprintf(stderr, "%s", mp_error_to_string(result));
	return ST_NIL;
}

char *st_large_integer_to_string(Oop integer, uint radix) {
	size_t size;
	mp_err err;
	err = mp_radix_size(VALUE (integer), radix, &size);
	if (err != MP_OKAY)
		goto out;
	
	char *str = st_malloc(size);
	err = mp_read_radix(VALUE (integer), str, radix);
	if (err != MP_OKAY)
		goto out;
	
	return str;
	out:
	fprintf(stderr, "%s", mp_error_to_string(err));
	return NULL;
}

Oop st_large_integer_allocate(Oop class, mp_int *value) {
	Oop object;
	uint size;
	
	size = ST_SIZE_OOPS(struct LargeInt);
	object = st_memory_allocate(size);
	if (object == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		object = st_memory_allocate(size);
		st_assert (object != 0);
	}
	
	st_object_initialize_header(object, class);
	
	if (value)
		*VALUE (object) = *value;
	
	return object;
}

Oop st_large_integer_new(mp_int *value) {
	return st_large_integer_allocate(ST_LARGE_INTEGER_CLASS, value);
}
