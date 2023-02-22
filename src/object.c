/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "object.h"
#include "universe.h"
#include "behavior.h"
#include "small-integer.h"
#include "association.h"
#include "float.h"
#include "array.h"
#include "symbol.h"
#include "character.h"
#include "handle.h"

void st_object_initialize_header(Oop object, Oop class) {
	ST_OBJECT_MARK(object) = 0 | ST_MARK_TAG;
	ST_OBJECT_CLASS(object) = class;
	st_object_set_format(object, st_smi_value(ST_BEHAVIOR_FORMAT (class)));
	st_object_set_instance_size(object, st_smi_value(ST_BEHAVIOR_INSTANCE_SIZE (class)));
}

bool st_object_equal(Oop object, Oop other) {
	if (st_object_class(object) == ST_SMI_CLASS)
		return st_smi_equal(object, other);
	
	if (st_object_class(object) == ST_CHARACTER_CLASS)
		return st_character_equal(object, other);
	
	if (ST_OBJECT_CLASS(object) == ST_FLOAT_CLASS)
		return st_float_equal(object, other);
	
	if (ST_OBJECT_CLASS(object) == ST_ASSOCIATION_CLASS)
		return st_association_equal(object, other);
	
	if (ST_OBJECT_CLASS(object) == ST_SYMBOL_CLASS)
		return st_symbol_equal(object, other);
	
	if (ST_OBJECT_CLASS(object) == ST_BYTE_ARRAY_CLASS ||
	    ST_OBJECT_CLASS(object) == ST_STRING_CLASS)
		return st_byte_array_equal(object, other);
	
	return object == other;
}

uint st_object_hash(Oop object) {
	if (st_object_class(object) == ST_SMI_CLASS)
		return st_smi_hash(object);
	
	if (st_object_class(object) == ST_BYTE_ARRAY_CLASS ||
	    st_object_class(object) == ST_STRING_CLASS ||
	    st_object_class(object) == ST_SYMBOL_CLASS)
		return st_byte_array_hash(object);
	
	if (st_object_class(object) == ST_FLOAT_CLASS)
		return st_float_hash(object);
	
	if (st_object_class(object) == ST_CHARACTER_CLASS)
		return st_character_hash(object);
	
	if (st_object_class(object) == ST_ASSOCIATION_CLASS)
		return st_association_hash(object);
	
	return object >> 2;
}

Oop st_object_allocate(Oop class) {
	Oop *fields;
	uint size, instance_size;
	Oop object;
	
	instance_size = st_smi_value(ST_BEHAVIOR_INSTANCE_SIZE (class));
	size = ST_SIZE_OOPS(ObjHeader) + instance_size;
	object = st_memory_allocate(size);
	if (object == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		object = st_memory_allocate(size);
		st_assert (object != 0);
	}
	
	st_object_initialize_header(object, class);
	
	fields = ST_OBJECT_FIELDS(object);
	for (uint i = 0; i < instance_size; i++)
		fields[i] = ST_NIL;
	return object;
}

Oop st_handle_allocate(Oop class) {
	Oop *fields;
	Oop object;
	uint size;
	
	size = ST_SIZE_OOPS(Handle);
	object = st_memory_allocate(size);
	if (object == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		object = st_memory_allocate(size);
		st_assert (object != 0);
	}
	st_object_initialize_header(object, class);
	return object;
}

void st_object_set_format(Oop object, st_format format) {
	_ST_OBJECT_SET_BITFIELD(ST_OBJECT_MARK(object), FORMAT, format);
}

st_format st_object_format(Oop object) {
	return _ST_OBJECT_GET_BITFIELD(ST_OBJECT_MARK(object), FORMAT);
}

void st_object_set_hashed(Oop object, bool hashed) {
	_ST_OBJECT_SET_BITFIELD(ST_OBJECT_MARK(object), HASH, hashed);
}

bool st_object_is_hashed(Oop object) {
	return _ST_OBJECT_GET_BITFIELD(ST_OBJECT_MARK(object), HASH);
}

uint st_object_instance_size(Oop object) {
	return _ST_OBJECT_GET_BITFIELD(ST_OBJECT_MARK(object), SIZE);
}

uint st_object_set_instance_size(Oop object, uint size) {
	_ST_OBJECT_SET_BITFIELD(ST_OBJECT_MARK(object), SIZE, size);
}

int st_object_tag(Oop object) {
	return object & st_tag_mask;
}

bool st_object_is_heap(Oop object) {
	return st_object_tag(object) == ST_POINTER_TAG;
}

bool st_object_is_smi(Oop object) {
	return st_object_tag(object) == ST_SMI_TAG;
}

bool st_object_is_character(Oop object) {
	return st_object_tag(object) == ST_CHARACTER_TAG;
}

bool st_object_is_mark(Oop object) {
	return st_object_tag(object) == ST_MARK_TAG;
}

Oop st_object_class(Oop object) {
	if (ST_UNLIKELY(st_object_is_smi(object)))
		return ST_SMI_CLASS;
	if (ST_UNLIKELY(st_object_is_character(object)))
		return ST_CHARACTER_CLASS;
	return ST_OBJECT_CLASS(object);
}

bool st_object_is_symbol(Oop object) {
	return st_object_class(object) == ST_SYMBOL_CLASS;
}

bool st_object_is_string(Oop object) {
	return st_object_class(object) == ST_STRING_CLASS;
}

bool st_object_is_array(Oop object) {
	return st_object_class(object) == ST_ARRAY_CLASS;
}

bool st_object_is_byte_array(Oop object) {
	return st_object_class(object) == ST_BYTE_ARRAY_CLASS;
}

bool st_object_is_float(Oop object) {
	return st_object_class(object) == ST_FLOAT_CLASS;
}
