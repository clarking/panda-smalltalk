
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <tommath.h>
#include "st-object.h"
#include "st-types.h"

#define ST_LARGE_INTEGER(oop) ((struct st_large_integer *) st_detag_pointer (oop))

struct st_large_integer {
	struct st_header parent;
	mp_int value;
};

st_oop st_large_integer_new(mp_int *value);

st_oop st_large_integer_new_from_smi(int integer);

st_oop st_large_integer_new_from_string(const char *string, st_uint radix);

char *st_large_integer_to_string(st_oop integer, st_uint radix);

st_oop st_large_integer_allocate(st_oop class, mp_int *value);

/* inline definitions */
static inline mp_int *st_large_integer_value(st_oop integer) {
	return &ST_LARGE_INTEGER (integer)->value;
}

