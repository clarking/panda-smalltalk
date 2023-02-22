
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "memory.h"

Oop st_string_new(const char *bytes);

Oop st_symbol_new(const char *bytes);

bool st_symbol_equal(Oop object, Oop other);