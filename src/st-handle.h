
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

#define  ST_HANDLE(oop)       ((st_handle *) st_detag_pointer (oop))
#define  ST_HANDLE_VALUE(oop) (ST_HANDLE (oop)->value)

st_oop st_handle_allocate(st_oop class);

