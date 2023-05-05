
/*
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */
#include "character.h"

Oop st_character_new(wchar_t wc) {
	return (Oop) ((wc << ST_TAG_SIZE) + ST_CHARACTER_TAG);
}

wchar_t st_character_value(Oop character) {
	return ((wchar_t) character) >> ST_TAG_SIZE;
}

bool st_character_equal(Oop m, Oop n) {
	return m == n;
}

int st_character_hash(Oop character) {
	return (int) st_character_value(character);
}