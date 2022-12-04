
/*
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */
#include "st-character.h"


st_oop st_character_new(wchar_t wc) {
	return (st_oop) ((wc << ST_TAG_SIZE) + ST_CHARACTER_TAG);
}

wchar_t st_character_value(st_oop character) {
	return ((wchar_t) character) >> ST_TAG_SIZE;
}

bool st_character_equal(st_oop m, st_oop n) {
	return m == n;
}

int st_character_hash(st_oop character) {
	return (int) st_character_value(character);
}