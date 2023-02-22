/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "small-integer.h"
#include "utils.h"
#include "universe.h"

#define ST_HEADER(oop)        ((ObjHeader *) st_detag_pointer (oop))
#define ST_OBJECT_MARK(oop)   (ST_HEADER (oop)->mark)
#define ST_OBJECT_CLASS(oop)  (ST_HEADER (oop)->class)
#define ST_OBJECT_FIELDS(oop) (ST_HEADER (oop)->fields)

#define _ST_OBJECT_SET_BITFIELD(bitfield, field, value)            \
    ((bitfield) = (((bitfield) & ~(_ST_OBJECT_##field##_MASK << _ST_OBJECT_##field##_SHIFT)) \
         | (((value) & _ST_OBJECT_##field##_MASK) << _ST_OBJECT_##field##_SHIFT)))

#define _ST_OBJECT_GET_BITFIELD(bitfield, field)            \
    (((bitfield) >> _ST_OBJECT_##field##_SHIFT) & _ST_OBJECT_##field##_MASK)


Oop st_object_allocate(Oop class);

void st_object_initialize_header(Oop object, Oop class);

bool st_object_equal(Oop object, Oop other);

uint st_object_hash(Oop object);

void st_object_set_format(Oop object, st_format format);

st_format st_object_format(Oop object);

void st_object_set_hashed(Oop object, bool hashed);

bool st_object_is_hashed(Oop object);

uint st_object_instance_size(Oop object);

uint st_object_set_instance_size(Oop object, uint size);

int st_object_tag(Oop object);

bool st_object_is_heap(Oop object);

bool st_object_is_smi(Oop object);

bool st_object_is_character(Oop object);

bool st_object_is_mark(Oop object);

Oop st_object_class(Oop object);

bool st_object_is_symbol(Oop object);

bool st_object_is_string(Oop object);

bool st_object_is_array(Oop object);

bool st_object_is_byte_array(Oop object);

bool st_object_is_float(Oop object);
