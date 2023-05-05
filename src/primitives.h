
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#include "machine.h"


#define VALUE(oop) (&(ST_LARGE_INTEGER(oop)->value))

/* useful macros to avoid duplication of error-handling code */

#define OP_PROLOGUE             \
    mp_int value;               \
    mp_err err;                 \
    err = mp_init (&value);     \
    if(err!=MP_OKAY) {           \
        fprintf(stderr, "%s", mp_error_to_string(err)); \
    abort();                       \
    }


#define BINARY_OP(op, a, b)    \
OP_PROLOGUE                    \
    result = op (VALUE (a), VALUE (b), &value);

#define BINARY_DIV_OP(op, a, b)                       \
OP_PROLOGUE                                           \
    result = op (VALUE (a), VALUE (b), &value, NULL);

#define UNARY_OP(op, a)              \
OP_PROLOGUE                          \
    result = op (VALUE (a), &value);


#define ST_DIGIT_RADIX (1L << MP_DIGIT_BIT)

typedef void (*PrimitiveFunc)(VirtualMachine *machine);

typedef struct Primitive {
	const char *name;
	PrimitiveFunc func;
} Primitive;

extern const Primitive st_primitives[];

int st_prim_index_for_name(const char *name);

