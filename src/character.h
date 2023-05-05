
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"

Oop st_character_new(wchar_t wc);

wchar_t st_character_value(Oop character);

bool st_character_equal(Oop m, Oop n);

int st_character_hash(Oop character);