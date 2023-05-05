
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "object.h"
#include "array.h"

#define ST_METHOD(oop) ((Method *) (st_detag_pointer (oop)))

typedef struct Method {
	ObjHeader parent;
	Oop header;
	Oop bytecode;
	Oop literals;
	Oop selector;
} Method;


#define ST_METHOD_NORMAL 0
#define ST_METHOD_RETURN_RECEIVER 1
#define ST_METHOD_RETURN_INSTVAR 2
#define ST_METHOD_RETURN_LITERAL 3
#define ST_METHOD_PRIMITIVE 4

typedef uint8_t MethodFlags;

#define ST_METHOD_LITERAL_NIL 0
#define ST_METHOD_LITERAL_TRUE 1
#define ST_METHOD_LITERAL_FALSE 2
#define ST_METHOD_LITERAL_MINUS_ONE 3
#define ST_METHOD_LITERAL_ZERO 4
#define ST_METHOD_LITERAL_ONE 5
#define ST_METHOD_LITERAL_TWO 6

typedef uint8_t MethodLiteralType;

#define ST_METHOD_HEADER(oop)   (ST_METHOD (oop)->header)
#define ST_METHOD_LITERALS(oop) (ST_METHOD (oop)->literals)
#define ST_METHOD_BYTECODE(oop) (ST_METHOD (oop)->bytecode)
#define ST_METHOD_SELECTOR(oop) (ST_METHOD (oop)->selector)

/*
 * CompiledMethod Header:
 *
 * The header is a smi containing various bitfields.
 *
 * A flag bitfield in the header indicates how the method must be executed. Generally
 * it provides various optimization hints to the interpreter. The flag bitfield
 * is alway stored in bits 31-29. The format of the remaining bits in the header
 * varies according to the flag value.  
 * 
 * flag (3 bits):
 *   0 : Activate method and execute its bytecodes 
 *   1 : The method simply returns 'self'. Has no side-effects. Don't bother creating a new activation.
 *   2 : The method simply returns an instance variable. Ditto.
 *   3 : The method simply returns a literal. Ditto.
 *   4 : The method performs a primitive operation.
 *
 * Bitfield format
 * 
 * flag = 0:
 *   [ flag: 3 | arg_count: 5 | temp_count: 6 | unused: 7 | large_context: 1 | primitive: 8 | tag: 2 ]
 *
 *   arg_count:      number of args
 *   temp_count:     number of temps
 *   stack_depth:    depth of stack
 *   primitive:      index of a primitive method
 *   tag:            The usual smi tag
 *
 * flag = 1:
 *   [ flag: 3 | unused: 27 | tag: 2 ]
 *
 * flag = 2:
 *   header: [ flag: 3 | unused: 11 | instvar: 16 | tag: 2 ]
 *
 *   instvar_index:  Index of instvar
 *
 * flag = 3:
 *   header: [ flag: 3 | unused: 23 | literal: 4 | tag: 2 ]
 *
 *   literal: 
 *      nil:             0
 *      true:            1
 *      false:           2
 *      -1:              3
 *       0:              4
 *       1:              5
 *       2:              6
 */

#define ST_METHOD_SET_BITFIELD(bitfield, field, value)            \
    ((bitfield) = ((bitfield) & ~(_ST_METHOD_##field##_MASK << _ST_METHOD_##field##_SHIFT)) \
     | (((value) & _ST_METHOD_##field##_MASK) << _ST_METHOD_##field##_SHIFT))

#define ST_METHOD_GET_BITFIELD(bitfield, field)            \
    (((bitfield) >> _ST_METHOD_##field##_SHIFT) & _ST_METHOD_##field##_MASK)


#define _ST_METHOD_FLAG_BITS        3
#define _ST_METHOD_ARG_BITS        5
#define _ST_METHOD_TEMP_BITS        6
#define _ST_METHOD_UNUSED_BITS    7
#define _ST_METHOD_LARGE_BITS        1
#define _ST_METHOD_INSTVAR_BITS        16
#define _ST_METHOD_LITERAL_BITS        4
#define _ST_METHOD_PRIMITIVE_BITS    8

#define _ST_METHOD_PRIMITIVE_SHIFT  ST_TAG_SIZE
#define _ST_METHOD_INSTVAR_SHIFT    ST_TAG_SIZE
#define _ST_METHOD_LITERAL_SHIFT    ST_TAG_SIZE
#define _ST_METHOD_LARGE_SHIFT    (_ST_METHOD_PRIMITIVE_BITS + _ST_METHOD_PRIMITIVE_SHIFT)
#define _ST_METHOD_UNUSED_SHIFT    (_ST_METHOD_LARGE_BITS + _ST_METHOD_LARGE_SHIFT)
#define _ST_METHOD_TEMP_SHIFT        (_ST_METHOD_UNUSED_BITS + _ST_METHOD_UNUSED_SHIFT)
#define _ST_METHOD_ARG_SHIFT        (_ST_METHOD_TEMP_BITS + _ST_METHOD_TEMP_SHIFT)
#define _ST_METHOD_FLAG_SHIFT        (_ST_METHOD_ARG_BITS + _ST_METHOD_ARG_SHIFT)

#define _ST_METHOD_PRIMITIVE_MASK   (ST_NTH_MASK (_ST_METHOD_PRIMITIVE_BITS))
#define _ST_METHOD_LARGE_MASK    (ST_NTH_MASK (_ST_METHOD_LARGE_BITS))
#define _ST_METHOD_INSTVAR_MASK    (ST_NTH_MASK (_ST_METHOD_INSTVAR_BITS))
#define _ST_METHOD_LITERAL_MASK    (ST_NTH_MASK (_ST_METHOD_LITERAL_BITS))
#define _ST_METHOD_TEMP_MASK        (ST_NTH_MASK (_ST_METHOD_TEMP_BITS))
#define _ST_METHOD_ARG_MASK        (ST_NTH_MASK (_ST_METHOD_ARG_BITS))
#define _ST_METHOD_FLAG_MASK        (ST_NTH_MASK (_ST_METHOD_FLAG_BITS))


static inline int st_method_get_temp_count(Oop method) {
	return ST_METHOD_GET_BITFIELD (ST_METHOD_HEADER(method), TEMP);
}

static inline int st_method_get_arg_count(Oop method) {
	return ST_METHOD_GET_BITFIELD (ST_METHOD_HEADER(method), ARG);
}

static inline int st_method_get_large_context(Oop method) {
	return ST_METHOD_GET_BITFIELD (ST_METHOD_HEADER(method), LARGE);
}

static inline int st_method_get_prim_index(Oop method) {
	return ST_METHOD_GET_BITFIELD (ST_METHOD_HEADER(method), PRIMITIVE);
}

static inline uint8_t st_method_get_flags(Oop method) {
	return ST_METHOD_GET_BITFIELD (ST_METHOD_HEADER(method), FLAG);
}

static inline void st_method_set_flags(Oop method, MethodFlags flags) {
	ST_METHOD_SET_BITFIELD (ST_METHOD_HEADER(method), FLAG, flags);
}

static inline void st_method_set_arg_count(Oop method, int count) {
	ST_METHOD_SET_BITFIELD (ST_METHOD_HEADER(method), ARG, count);
}

static inline void st_method_set_temp_count(Oop method, int count) {
	ST_METHOD_SET_BITFIELD (ST_METHOD_HEADER(method), TEMP, count);
}

static inline void st_method_set_large_context(Oop method, bool is_large) {
	ST_METHOD_SET_BITFIELD (ST_METHOD_HEADER(method), LARGE, is_large);
}

static void st_method_set_prim_index(Oop method, int index) {
	ST_METHOD_SET_BITFIELD (ST_METHOD_HEADER(method), PRIMITIVE, index);
}

static inline void st_method_set_instvar_index(Oop method, int index) {
	ST_METHOD_SET_BITFIELD (ST_METHOD_HEADER(method), INSTVAR, index);
}

static inline void st_method_set_literal_type(Oop method, MethodLiteralType literal_type) {
	ST_METHOD_SET_BITFIELD (ST_METHOD_HEADER(method), LITERAL, literal_type);
}

static inline char *st_method_bytecode_bytes(Oop method) {
	return st_byte_array_bytes(ST_METHOD_BYTECODE (method));
}
