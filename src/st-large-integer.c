/*
 * st-large-integer.c
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

#include "st-large-integer.h"
#include "st-universe.h"
#include "st-types.h"

#define VALUE(oop)  (&(ST_LARGE_INTEGER(oop)->value))

st_oop st_large_integer_new_from_string(const char *string, st_uint radix) {
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
	fprintf(stderr, mp_error_to_string(result));
	return ST_NIL;
}

char *st_large_integer_to_string(st_oop integer, st_uint radix) {
	int result;
	int size;

	result = mp_radix_size(VALUE (integer), radix, &size);
	if (result != MP_OKAY)
		goto out;

	char *str = st_malloc(size);

	mp_toradix(VALUE (integer), str, radix);
	if (result != MP_OKAY)
		goto out;

	return str;

	out:
	fprintf(stderr, mp_error_to_string(result));
	return NULL;
}

st_oop st_large_integer_allocate(st_oop class, mp_int *value) {
	st_oop object;
	st_uint size;

	size = ST_SIZE_OOPS (struct st_large_integer);
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

st_oop st_large_integer_new(mp_int *value) {
	return st_large_integer_allocate(ST_LARGE_INTEGER_CLASS, value);

}
