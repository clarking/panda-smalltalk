/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */
#include "dictionary.h"
#include "types.h"
#include "array.h"
#include "small-integer.h"
#include "utils.h"
#include "universe.h"
#include "association.h"
#include "object.h"
#include "behavior.h"

#define DEFAULT_CAPACITY     8
#define MINIMUM_CAPACITY     4
#define ST_HASHED_COLLECTION(oop) ((HashedCollection *) st_detag_pointer (oop))
#define SIZE(collection)     (ST_HASHED_COLLECTION (collection)->size)
#define DELETED(collection)  (ST_HASHED_COLLECTION (collection)->deleted)
#define ARRAY(collection)    (ST_HASHED_COLLECTION (collection)->array)
#define ARRAY_SIZE(array)    (st_smi_value (ST_ARRAYED_OBJECT (array)->size))

/* Common methods */

uint occupied(Oop collection) {
	return st_smi_value(SIZE (collection)) + st_smi_value(DELETED (collection));
}

uint size_for_capacity(uint capacity) {
	uint size;
	size = MINIMUM_CAPACITY;
	while (size < capacity)
		size = size + size;
	return size;
}

void initialize(Oop collection, int capacity) {
	st_assert (capacity > 0);
	capacity = size_for_capacity(capacity);
	
	SIZE (collection) = st_smi_new(0);
	DELETED (collection) = st_smi_new(0);
	ARRAY (collection) = st_object_new_arrayed(ST_ARRAY_CLASS, capacity);
}

void dict_check_grow(Oop dict) {
	Oop old, object;
	uint size, n;
	size = n = ARRAY_SIZE (ARRAY(dict));
	
	if (occupied(dict) * 2 <= size)
		return;
	
	size *= 2;
	old = ARRAY (dict);
	ARRAY (dict) = st_object_new_arrayed(ST_ARRAY_CLASS, size);
	DELETED (dict) = st_smi_new(0);
	
	for (uint i = 1; i <= n; i++) {
		object = st_array_at(old, i);
		if (object != ST_NIL) {
			st_array_at_put(ARRAY (dict), dict_find(dict, ST_ASSOCIATION_KEY (object)), object);
		}
	}
}

uint dict_find(Oop dict, Oop key) {
	Oop el;
	uint mask;
	uint i;
	
	mask = ARRAY_SIZE (ARRAY(dict)) - 1;
	i = (st_object_hash(key) & mask) + 1;
	
	while (true) {
		el = st_array_at(ARRAY (dict), i);
		if (el == ST_NIL || key == ST_ASSOCIATION_KEY (el))
			return i;
		i = ((i + ST_ADVANCE_SIZE) & mask) + 1;
	}
}

void st_dictionary_at_put(Oop dict, Oop key, Oop value) {
	uint index;
	Oop assoc;
	
	index = dict_find(dict, key);
	assoc = st_array_at(ARRAY (dict), index);
	
	if (assoc == ST_NIL) {
		st_array_at_put(ARRAY (dict), index, st_association_new(key, value));
		SIZE (dict) = st_smi_increment(SIZE (dict));
		dict_check_grow(dict);
	}
	else {
		ST_ASSOCIATION_VALUE (assoc) = value;
	}
}

Oop st_dictionary_at(Oop dict, Oop key) {
	int index;
	Oop assoc;
	assoc = st_array_at(ARRAY (dict), dict_find(dict, key));
	if (assoc != ST_NIL)
		return ST_ASSOCIATION_VALUE (assoc);
	
	return ST_NIL;
}

Oop st_dictionary_association_at(Oop dict, Oop key) {
	return st_array_at(ARRAY (dict), dict_find(dict, key));
}

Oop st_dictionary_new(void) {
	return st_dictionary_new_with_capacity(DEFAULT_CAPACITY);
}

Oop st_dictionary_new_with_capacity(int capacity) {
	Oop dict;
	dict = st_object_new(ST_DICTIONARY_CLASS);
	initialize(dict, capacity);
	return dict;
}

/* Set implementation */

uint set_find_cstring(Oop set, const char *string) {
	Oop el;
	uint mask, i;
	
	mask = ARRAY_SIZE (ARRAY(set)) - 1;
	i = (st_string_hash(string) & mask) + 1;
	
	while (true) {
		el = st_array_at(ARRAY (set), i);
		if (el == ST_NIL || (strcmp(st_byte_array_bytes(el), string) == 0))
			return i;
		i = ((i + ST_ADVANCE_SIZE) & mask) + 1;
	}
}

uint set_find(Oop set, Oop object) {
	Oop el;
	uint mask, i;
	
	mask = ARRAY_SIZE (ARRAY(set)) - 1;
	i = (st_object_hash(object) & mask) + 1;
	
	while (true) {
		el = st_array_at(ARRAY (set), i);
		if (el == ST_NIL || el == object)
			return i;
		i = ((i + ST_ADVANCE_SIZE) & mask) + 1;
	}
}

void set_check_grow(Oop set) {
	Oop old, object;
	uint size, n;
	
	size = n = ARRAY_SIZE (ARRAY(set));
	if (occupied(set) * 2 <= size)
		return;
	
	size *= 2;
	old = ARRAY (set);
	ARRAY (set) = st_object_new_arrayed(ST_ARRAY_CLASS, size);
	DELETED (set) = st_smi_new(0);
	
	for (uint i = 1; i <= n; i++) {
		object = st_array_at(old, i);
		if (object != ST_NIL)
			st_array_at_put(ARRAY (set), set_find(set, object), object);
	}
}

Oop st_set_intern_cstring(Oop set, const char *string) {
	Oop intern;
	uint i, len;
	
	st_assert (string != NULL);
	
	i = set_find_cstring(set, string);
	intern = st_array_at(ARRAY (set), i);
	if (intern != ST_NIL)
		return intern;
	
	len = strlen(string);
	intern = st_object_new_arrayed(ST_SYMBOL_CLASS, len);
	memcpy(st_byte_array_bytes(intern), string, len);
	st_array_at_put(ARRAY (set), i, intern);
	SIZE (set) = st_smi_increment(SIZE (set));
	set_check_grow(set);
	return intern;
}

void st_set_add(Oop set, Oop object) {
	st_array_at_put(ARRAY (set), set_find(set, object), object);
	SIZE (set) = st_smi_increment(SIZE (set));
	set_check_grow(set);
}

bool st_set_includes(Oop set, Oop object) {
	return st_array_at(ARRAY (set), set_find(set, object)) != ST_NIL;
}

Oop st_set_like(Oop set, Oop object) {
	return st_array_at(ARRAY (set), set_find(set, object));
}

Oop st_set_new_with_capacity(int capacity) {
	Oop set;
	set = st_object_new(ST_SET_CLASS);
	initialize(set, capacity);
	return set;
}

Oop st_set_new(void) {
	return st_set_new_with_capacity(DEFAULT_CAPACITY);
}