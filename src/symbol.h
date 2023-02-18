
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"
#include "st-memory.h"

st_oop st_string_new(const char *bytes);

st_oop st_symbol_new(const char *bytes);

bool st_symbol_equal(st_oop object, st_oop other);