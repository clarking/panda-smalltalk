
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once


#include "object.h"
#include "types.h"

#define ST_LARGE_INTEGER(oop) ((LargeInt *) st_detag_pointer (oop))

Oop st_large_integer_new(mp_int *value);

Oop st_large_integer_new_from_smi(int integer);

Oop st_large_integer_new_from_string(const char *string, uint radix);

char *st_large_integer_to_string(Oop integer, uint radix);

Oop st_large_integer_allocate(Oop class, mp_int *value);

/* inline definitions */
static inline mp_int *st_large_integer_value(Oop integer) {
	return &ST_LARGE_INTEGER (integer)->value;
}

