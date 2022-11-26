
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#include "st-machine.h"

typedef void (*st_primitive_func)(struct st_machine *machine);

struct st_primitive {
	const char *name;
	st_primitive_func func;
};

extern const struct st_primitive st_primitives[];

int st_primitive_index_for_name(const char *name);

