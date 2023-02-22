
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include "types.h"
#include "object.h"

#define ST_FLOAT(oop) ((struct st_float *) st_detag_pointer (oop))

struct st_float {
	struct ObjHeader parent;
	double value;
};

Oop st_float_new(double value);

Oop st_float_allocate(Oop class);

static inline double st_float_value(Oop object) {
	return ST_FLOAT (object)->value;
}

static inline void st_float_set_value(Oop object, double value) {
	ST_FLOAT (object)->value = value;
}

static inline bool st_float_equal(Oop object, Oop other) {
	if (st_object_class(other) != ST_FLOAT_CLASS)
		return false;
	return st_float_value(object) == st_float_value(other);
}

static inline st_uint st_float_hash(Oop object) {
	return (st_uint) st_float_value(object);
}

