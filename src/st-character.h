
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

static inline st_oop st_character_new(wchar_t wc);

static inline wchar_t st_character_value(st_oop character);

static inline bool st_character_equal(st_oop m, st_oop n);

static inline int st_character_hash(st_oop character);

/* inline definitions */

static inline st_oop
st_character_new(wchar_t wc) {
	return (st_oop) ((wc << ST_TAG_SIZE) + ST_CHARACTER_TAG);
}

static inline wchar_t
st_character_value(st_oop character) {
	return ((wchar_t) character) >> ST_TAG_SIZE;
}

static inline bool
st_character_equal(st_oop m, st_oop n) {
	return m == n;
}

static inline int
st_character_hash(st_oop character) {
	return (int) st_character_value(character);
}

