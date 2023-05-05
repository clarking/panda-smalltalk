
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "float.h"
#include "behavior.h"

Oop st_float_allocate(Oop class) {
	uint size;
	Oop object;

	size = ST_SIZE_OOPS(Float);
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

Oop st_float_new(double value) {
	Oop object;
	object = st_object_new(ST_FLOAT_CLASS);
	st_float_set_value(object, value);
	return object;
}
