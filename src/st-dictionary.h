
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

// prime number - 1
#define ST_ADVANCE_SIZE 106720

st_oop st_dictionary_new(void);

st_oop st_dictionary_new_with_capacity(int capacity);

st_oop st_dictionary_at(st_oop dict, st_oop key);

void st_dictionary_at_put(st_oop dict, st_oop key, st_oop value);

st_oop st_dictionary_association_at(st_oop dict, st_oop key);

st_oop st_set_new(void);

st_oop st_set_intern_cstring(st_oop set, const char *string);

st_oop st_set_new_with_capacity(int capacity);

bool st_set_includes(st_oop set, st_oop object);

st_oop st_set_like(st_oop set, st_oop object);

void st_set_add(st_oop set, st_oop object);
