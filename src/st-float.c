
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "st-float.h"
#include "st-behavior.h"

st_oop st_float_allocate(st_oop class) {
	st_uint size;
	st_oop object;
	
	size = ST_SIZE_OOPS (struct st_float);
	object = st_memory_allocate(size);
	if (object == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		object = st_memory_allocate(size);
		st_assert (object != 0);
	}
	
	st_object_initialize_header(object, class);
	st_float_set_value(object, 0.0);
	return object;
}

st_oop st_float_new(double value) {
	st_oop object;
	object = st_object_new(ST_FLOAT_CLASS);
	st_float_set_value(object, value);
	return object;
}
