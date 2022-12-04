
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include "st-types.h"
#include "st-object.h"

#define ST_ARRAYED_OBJECT(oop) ((st_arrayed_object *) st_detag_pointer (oop))
#define ST_ARRAY(oop)          ((st_array *)          st_detag_pointer (oop))
#define ST_FLOAT_ARRAY(oop)    ((st_float_array *)    st_detag_pointer (oop))
#define ST_WORD_ARRAY(oop)     ((st_word_array *)     st_detag_pointer (oop))
#define ST_BYTE_ARRAY(oop)     ((st_byte_array *)     st_detag_pointer (oop))


bool st_byte_array_equal(st_oop object, st_oop other);

st_uint st_byte_array_hash(st_oop object);

st_oop st_array_allocate(st_oop class, st_uint size);

st_oop st_float_array_allocate(st_oop class, int size);

st_oop st_word_array_allocate(st_oop class, int size);

st_oop st_byte_array_allocate(st_oop class, int size);

st_oop st_arrayed_object_size(st_oop object);

st_oop *st_array_elements(st_oop object);

st_oop st_array_at(st_oop object, int i);

void st_array_at_put(st_oop object, int i, st_oop value);

st_uint *st_word_array_elements(st_oop object);

st_uint st_word_array_at(st_oop object, int i);

void st_word_array_at_put(st_oop object, int i, st_uint value);

char *st_byte_array_bytes(st_oop object);

st_uchar st_byte_array_at(st_oop object, int i);

void st_byte_array_at_put(st_oop object, int i, st_uchar value);

double *st_float_array_elements(st_oop array);

double st_float_array_at(st_oop array, int i);

void st_float_array_at_put(st_oop array, int i, double value);

