/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"
#include "st-small-integer.h"
#include "st-utils.h"
#include "st-universe.h"

#define ST_HEADER(oop)        ((st_header *) st_detag_pointer (oop))
#define ST_OBJECT_MARK(oop)   (ST_HEADER (oop)->mark)
#define ST_OBJECT_CLASS(oop)  (ST_HEADER (oop)->class)
#define ST_OBJECT_FIELDS(oop) (ST_HEADER (oop)->fields)

#define _ST_OBJECT_SET_BITFIELD(bitfield, field, value)            \
    ((bitfield) = (((bitfield) & ~(_ST_OBJECT_##field##_MASK << _ST_OBJECT_##field##_SHIFT)) \
         | (((value) & _ST_OBJECT_##field##_MASK) << _ST_OBJECT_##field##_SHIFT)))

#define _ST_OBJECT_GET_BITFIELD(bitfield, field)            \
    (((bitfield) >> _ST_OBJECT_##field##_SHIFT) & _ST_OBJECT_##field##_MASK)


st_oop st_object_allocate(st_oop class);

void st_object_initialize_header(st_oop object, st_oop class);

bool st_object_equal(st_oop object, st_oop other);

st_uint st_object_hash(st_oop object);

void st_object_set_format(st_oop object, st_format format);

st_format st_object_format(st_oop object);

void st_object_set_hashed(st_oop object, bool hashed);

bool st_object_is_hashed(st_oop object);

st_uint st_object_instance_size(st_oop object);

st_uint st_object_set_instance_size(st_oop object, st_uint size);

int st_object_tag(st_oop object);

bool st_object_is_heap(st_oop object);

bool st_object_is_smi(st_oop object);

bool st_object_is_character(st_oop object);

bool st_object_is_mark(st_oop object);

st_oop st_object_class(st_oop object);

bool st_object_is_symbol(st_oop object);

bool st_object_is_string(st_oop object);

bool st_object_is_array(st_oop object);

bool st_object_is_byte_array(st_oop object);

bool st_object_is_float(st_oop object);
