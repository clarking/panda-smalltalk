
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"

#define  ST_HANDLE(oop)       ((Handle *) st_detag_pointer (oop))
#define  ST_HANDLE_VALUE(oop) (ST_HANDLE (oop)->value)

Oop st_handle_allocate(Oop class);

