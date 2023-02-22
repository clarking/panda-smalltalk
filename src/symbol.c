
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "symbol.h"
#include "dictionary.h"
#include "array.h"
#include "behavior.h"
#include "object.h"

bool st_symbol_equal(Oop object, Oop other) {
	if (object == other)
		return true;
	
	if (st_object_class(object) == st_object_class(other))
		return false;
	
	return st_byte_array_equal(object, other);
}

static Oop string_new(Oop class, const char *bytes) {
	Oop string;
	char *data;
	int len;
	
	len = strlen(bytes);
	string = st_object_new_arrayed(class, len);
	data = st_byte_array_bytes(string);
	memcpy(data, bytes, len);
	return string;
}

Oop st_string_new(const char *bytes) {
	return string_new(ST_STRING_CLASS, bytes);
}

Oop st_symbol_new(const char *bytes) {
	return st_set_intern_cstring(ST_SYMBOLS, bytes);
}
