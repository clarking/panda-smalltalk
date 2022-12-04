
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "st-symbol.h"
#include "st-dictionary.h"
#include "st-array.h"
#include "st-behavior.h"
#include "st-object.h"

bool st_symbol_equal(st_oop object, st_oop other) {
	if (object == other)
		return true;
	
	if (st_object_class(object) == st_object_class(other))
		return false;
	
	return st_byte_array_equal(object, other);
}

static st_oop string_new(st_oop class, const char *bytes) {
	st_oop string;
	char *data;
	int len;
	
	len = strlen(bytes);
	string = st_object_new_arrayed(class, len);
	data = st_byte_array_bytes(string);
	memcpy(data, bytes, len);
	return string;
}

st_oop st_string_new(const char *bytes) {
	return string_new(ST_STRING_CLASS, bytes);
}

st_oop st_symbol_new(const char *bytes) {
	return st_set_intern_cstring(ST_SYMBOLS, bytes);
}
