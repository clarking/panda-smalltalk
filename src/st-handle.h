
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <st-types.h>

struct st_handle {
	struct st_header header;
	uintptr_t value;
};

#define  ST_HANDLE(oop)       ((struct st_handle *) st_detag_pointer (oop))
#define  ST_HANDLE_VALUE(oop) (ST_HANDLE (oop)->value)

st_oop st_handle_allocate(st_oop class);

