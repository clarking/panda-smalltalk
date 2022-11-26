
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Check host data model. We only support LP64 at the moment.
 */
#ifndef __LP64__
#  define ST_HOST32             1
#  define ST_HOST64             0
#  define ST_BITS_PER_WORD     32
#  define ST_BITS_PER_INTEGER  32
#else
#  define ST_HOST32            0
#  define ST_HOST64            1
#  define ST_BITS_PER_WORD     64
#  define ST_BITS_PER_INTEGER  32
#endif

#define ST_SMALL_INTEGER_MIN  (-ST_SMALL_INTEGER_MAX - 1)
#define ST_SMALL_INTEGER_MAX  536870911

enum {
	ST_SMI_TAG,
	ST_POINTER_TAG,
	ST_CHARACTER_TAG,
	ST_MARK_TAG,
};

#define ST_TAG_SIZE 2

/* basic oop pointer:
 * integral type wide enough to hold a C pointer.
 * Can either point to a heap object or contain a smi or Character immediate.
 */
typedef uintptr_t st_oop;

typedef unsigned char st_uchar;
typedef unsigned short st_ushort;
typedef unsigned long st_ulong;
typedef unsigned int st_uint;
typedef void *st_pointer;
typedef st_uint st_unichar;

static inline st_oop
st_tag_pointer(st_pointer p) {
	return ((st_oop) p) + ST_POINTER_TAG;
}

static inline st_oop *
st_detag_pointer(st_oop oop) {
	return (st_oop *) (oop - ST_POINTER_TAG);
}

struct _global {
	int width, maxhelppos, indent;
	const char *helppfx;
	const char *filepath;
	char sf[3]; /* Initialised to 0 from here on. */
	const char *prog, *usage, *message, *helplf;
	char helpsf, **argv;
	struct opt_spec *opts, *curr;
	int opts1st, helppos;
	bool verbose;
	bool repl;
};

typedef struct _global global;