
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#include "st-machine.h"


#define VALUE(oop) (&(ST_LARGE_INTEGER(oop)->value))

/* useful macros to avoid duplication of error-handling code */

#define OP_PROLOGUE             \
    mp_int value;               \
    mp_init (&value);

#define BINARY_OP(op, a, b)    \
OP_PROLOGUE                    \
    result = op (VALUE (a), VALUE (b), &value);

#define BINARY_DIV_OP(op, a, b)                       \
OP_PROLOGUE                                           \
    result = op (VALUE (a), VALUE (b), &value, NULL);

#define UNARY_OP(op, a)              \
OP_PROLOGUE                          \
    result = op (VALUE (a), &value);


#define ST_DIGIT_RADIX (1L << DIGIT_BIT)

typedef void (*st_prim_func)(struct st_machine *machine);

struct st_primitive {
	const char *name;
	st_prim_func func;
};

extern const struct st_primitive st_primitives[];

int st_prim_index_for_name(const char *name);

