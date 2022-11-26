/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "st-object.h"
#include "st-universe.h"
#include "st-behavior.h"
#include "st-small-integer.h"
#include "st-association.h"
#include "st-float.h"
#include "st-array.h"
#include "st-symbol.h"
#include "st-character.h"
#include "st-handle.h"

void st_object_initialize_header(st_oop object, st_oop class) {
	ST_OBJECT_MARK (object) = 0 | ST_MARK_TAG;
	ST_OBJECT_CLASS (object) = class;
	st_object_set_format(object, st_smi_value(ST_BEHAVIOR_FORMAT (class)));
	st_object_set_instance_size(object, st_smi_value(ST_BEHAVIOR_INSTANCE_SIZE (class)));
}

bool st_object_equal(st_oop object, st_oop other) {
	if (st_object_class(object) == ST_SMI_CLASS)
		return st_smi_equal(object, other);
	
	if (st_object_class(object) == ST_CHARACTER_CLASS)
		return st_character_equal(object, other);
	
	if (ST_OBJECT_CLASS (object) == ST_FLOAT_CLASS)
		return st_float_equal(object, other);
	
	if (ST_OBJECT_CLASS (object) == ST_ASSOCIATION_CLASS)
		return st_association_equal(object, other);
	
	if (ST_OBJECT_CLASS (object) == ST_SYMBOL_CLASS)
		return st_symbol_equal(object, other);
	
	if (ST_OBJECT_CLASS (object) == ST_BYTE_ARRAY_CLASS ||
	    ST_OBJECT_CLASS (object) == ST_STRING_CLASS)
		return st_byte_array_equal(object, other);
	
	return object == other;
}

st_uint st_object_hash(st_oop object) {
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

st_oop st_object_allocate(st_oop class) {
	st_oop *fields;
	st_uint size, instance_size;
	st_oop object;
	
	instance_size = st_smi_value(ST_BEHAVIOR_INSTANCE_SIZE (class));
	size = ST_SIZE_OOPS (struct st_header) + instance_size;
	object = st_memory_allocate(size);
	if (object == 0) {
		st_memory_perform_gc();
		class = st_memory_remap_reference(class);
		object = st_memory_allocate(size);
		st_assert (object != 0);
	}
	
	st_object_initialize_header(object, class);
	
	fields = ST_OBJECT_FIELDS (object);
	for (st_uint i = 0; i < instance_size; i++)
		fields[i] = ST_NIL;
	return object;
}

st_oop st_handle_allocate(st_oop class) {
	st_oop *fields;
	st_oop object;
	st_uint size;
	
	size = ST_SIZE_OOPS (struct st_handle);
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
