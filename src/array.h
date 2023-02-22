
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include "types.h"
#include "object.h"

#define ST_ARRAYED_OBJECT(oop) ((ArrayedObject *) st_detag_pointer (oop))
#define ST_ARRAY(oop)          ((Array *)          st_detag_pointer (oop))
#define ST_FLOAT_ARRAY(oop)    ((FloatArray *)    st_detag_pointer (oop))
#define ST_WORD_ARRAY(oop)     ((WordArray *)     st_detag_pointer (oop))
#define ST_BYTE_ARRAY(oop)     ((ByteArray *)     st_detag_pointer (oop))


bool st_byte_array_equal(Oop object, Oop other);

st_uint st_byte_array_hash(Oop object);

Oop st_array_allocate(Oop class, st_uint size);

Oop st_float_array_allocate(Oop class, int size);

Oop st_word_array_allocate(Oop class, int size);

Oop st_byte_array_allocate(Oop class, int size);

Oop st_arrayed_object_size(Oop object);

Oop *st_array_elements(Oop object);

Oop st_array_at(Oop object, int i);

void st_array_at_put(Oop object, int i, Oop value);

st_uint *st_word_array_elements(Oop object);

st_uint st_word_array_at(Oop object, int i);

void st_word_array_at_put(Oop object, int i, st_uint value);

char *st_byte_array_bytes(Oop object);

st_uchar st_byte_array_at(Oop object, int i);

void st_byte_array_at_put(Oop object, int i, st_uchar value);

double *st_float_array_elements(Oop array);

double st_float_array_at(Oop array, int i);

void st_float_array_at_put(Oop array, int i, double value);

