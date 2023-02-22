
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "behavior.h"
#include "object.h"
#include "universe.h"
#include "array.h"
#include "handle.h"

List *st_behavior_all_instance_variables(Oop class) {
	List *list = NULL;
	Oop names;
	int size;
	
	if (class == ST_NIL)
		return NULL;
	
	names = ST_BEHAVIOR_INSTANCE_VARIABLES(class);
	if (names != ST_NIL) {
		size = st_smi_value(st_arrayed_object_size(names));
		for (int i = 1; i <= size; i++)
			list = st_list_prepend(list, (void *) st_strdup(st_byte_array_bytes(st_array_at(names, i))));
	}
	
	return st_list_concat(st_behavior_all_instance_variables(ST_BEHAVIOR_SUPERCLASS(class)),
			st_list_reverse(list));
}

Oop st_object_new(Oop class) {
	switch (st_smi_value(ST_BEHAVIOR_FORMAT(class))) {
		case ST_FORMAT_OBJECT:
			return st_object_allocate(class);
		case ST_FORMAT_CONTEXT:
			abort(); // not implemented
		case ST_FORMAT_FLOAT:
			return st_float_allocate(class);
		case ST_FORMAT_LARGE_INTEGER:
			return st_large_integer_allocate(class, NULL);
		case ST_FORMAT_HANDLE:
			return st_handle_allocate(class);
		default:
			abort(); // should not reach
	}
}

Oop st_object_new_arrayed(Oop class, int size) {
	switch (st_smi_value(ST_BEHAVIOR_FORMAT (class))) {
		case ST_FORMAT_ARRAY:
			return st_array_allocate(class, size);
		case ST_FORMAT_BYTE_ARRAY:
			return st_byte_array_allocate(class, size);
		case ST_FORMAT_WORD_ARRAY:
			return st_word_array_allocate(class, size);
		case ST_FORMAT_FLOAT_ARRAY:
			return st_float_array_allocate(class, size);
		case ST_FORMAT_INTEGER_ARRAY:
			abort(); // not implemented
		default:
			abort(); // should not reach
	}
}
