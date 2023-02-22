
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"

// prime number - 1
#define ST_ADVANCE_SIZE 106720

Oop st_dictionary_new(void);

Oop st_dictionary_new_with_capacity(int capacity);

Oop st_dictionary_at(Oop dict, Oop key);

void st_dictionary_at_put(Oop dict, Oop key, Oop value);

Oop st_dictionary_association_at(Oop dict, Oop key);

Oop st_set_new(void);

uint set_find_cstring(Oop set, const char *string);

Oop st_set_intern_cstring(Oop set, const char *string);

Oop st_set_new_with_capacity(int capacity);

bool st_set_includes(Oop set, Oop object);

Oop st_set_like(Oop set, Oop object);

void st_set_add(Oop set, Oop object);

uint dict_find(Oop dict, Oop object);

uint set_find(Oop set, Oop object);

void dict_no_check_add(Oop dict, Oop object);

void set_no_check_add(Oop set, Oop object);

uint occupied(Oop collection);

uint size_for_capacity(uint capacity);

void initialize(Oop collection, int capacity);

void dict_check_grow(Oop dict);

void set_check_grow(Oop set);