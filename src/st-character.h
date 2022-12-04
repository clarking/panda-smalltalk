
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

st_oop st_character_new(wchar_t wc);

wchar_t st_character_value(st_oop character);

bool st_character_equal(st_oop m, st_oop n);

int st_character_hash(st_oop character);