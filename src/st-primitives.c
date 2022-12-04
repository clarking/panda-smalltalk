
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "st-primitives.h"
#include "st-machine.h"
#include "st-array.h"
#include "st-large-integer.h"
#include "st-float.h"
#include "st-object.h"
#include "st-behavior.h"
#include "st-context.h"
#include "st-method.h"
#include "st-symbol.h"
#include "st-character.h"
#include "st-dictionary.h"
#include "st-compiler.h"
#include "st-handle.h"

static inline void set_success(st_machine *machine, bool success) {
	machine->success = machine->success && success;
}

static inline int pop_integer(st_machine *machine) {
	st_oop object = ST_STACK_POP (machine);
	if (ST_LIKELY (st_object_is_smi(object)))
		return st_smi_value(object);
	machine->success = false;
	return 0;
}

static inline int pop_integer32(st_machine *machine) {
	st_oop object = ST_STACK_POP (machine);
	if (ST_LIKELY (st_object_is_smi(object)))
		return st_smi_value(object);
	else if (st_object_class(object) == ST_LARGE_INTEGER_CLASS)
		return (int) mp_get_int(st_large_integer_value(object));
	machine->success = false;
	return 0;
}

static void prim_small_int_add(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	int result;
	if (ST_LIKELY (machine->success)) {
		result = x + y;
		if (((result << 1) ^ (result << 2)) >= 0) {
			ST_STACK_PUSH (machine, st_smi_new(result));
			return;
		} else
			machine->success = false;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_sub(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	int result;
	if (ST_LIKELY (machine->success)) {
		result = x + y;
		if (((result << 1) ^ (result << 2)) >= 0) {
			ST_STACK_PUSH (machine, st_smi_new(result));
			return;
		} else
			machine->success = false;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_lt(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	
	if (ST_LIKELY (machine->success)) {
		result = (x < y) ? ST_TRUE : ST_FALSE;
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_gt(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	if (ST_LIKELY (machine->success)) {
		result = (x > y) ? ST_TRUE : ST_FALSE;
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_le(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	if (ST_LIKELY (machine->success)) {
		result = (x <= y) ? ST_TRUE : ST_FALSE;
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_ge(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	if (ST_LIKELY (machine->success)) {
		result = (x >= y) ? ST_TRUE : ST_FALSE;
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_eq(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	if (ST_LIKELY (machine->success)) {
		result = (x == y) ? ST_TRUE : ST_FALSE;
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_ne(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	if (ST_LIKELY (machine->success)) {
		result = (x != y) ? ST_TRUE : ST_FALSE;
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_mul(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	int64_t result;
	if (machine->success) {
		result = x * y;
		if (result >= ST_SMALL_INTEGER_MIN && result <= ST_SMALL_INTEGER_MAX) {
			ST_STACK_PUSH (machine, st_smi_new((int) result));
			return;
		} else
			machine->success = false;
	}
	
	ST_STACK_UNPOP (machine, 2);
}

/* selector: / */
static void prim_small_int_div_sel(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	if (ST_LIKELY (machine->success)) {
		if (y != 0 && x % y == 0) {
			result = st_smi_new(x / y);
			ST_STACK_PUSH (machine, result);
			return;
		} else
			machine->success = false;
	}
	
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_div(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	
	if (ST_LIKELY (machine->success)) {
		if (y != 0) {
			result = st_smi_new(x / y);
			ST_STACK_PUSH (machine, result);
			return;
		} else
			machine->success = false;
	}
	
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_mod(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	
	if (ST_LIKELY (machine->success)) {
		result = st_smi_new(x % y);
		ST_STACK_PUSH (machine, result);
		return;
	}
	
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_bitOr(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result = ST_NIL;
	if (ST_LIKELY (machine->success)) {
		result = st_smi_new(x | y);
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_bitXor(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result;
	if (ST_LIKELY (machine->success)) {
		result = st_smi_new(x ^ y);
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_bitAnd(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result = ST_NIL;
	if (ST_LIKELY (machine->success)) {
		result = st_smi_new(x & y);
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_bitShift(st_machine *machine) {
	int y = pop_integer(machine);
	int x = pop_integer(machine);
	st_oop result = ST_NIL;
	if (ST_LIKELY (machine->success)) {
		if (y > 0)
			result = st_smi_new(x << y);
		else if (y < 0)
			result = st_smi_new(x >> (-y));
		else
			result = st_smi_new(x);
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 2);
}

static void prim_small_int_asFloat(st_machine *machine) {
	int x = pop_integer(machine);
	st_oop result = ST_NIL;
	if (ST_LIKELY (machine->success)) {
		result = st_float_new((double) x);
		ST_STACK_PUSH (machine, result);
		return;
	}
	ST_STACK_UNPOP (machine, 1);
}

static void prim_small_int_asLargeInt(st_machine *machine) {
	int receiver = pop_integer(machine);
	mp_int value;
	st_oop result;
	mp_init_set(&value, abs(receiver));
	if (receiver < 0)
		mp_neg(&value, &value);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static inline st_oop prim_pop_large_int(st_machine *machine) {
	st_oop object = ST_STACK_POP (machine);
	set_success(machine, st_object_class(object) == ST_LARGE_INTEGER_CLASS);
	return object;
}

static void prim_large_int_add(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_add, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_sub(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_sub, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_mul(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_mul, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_div_sel(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	mp_int quotient, remainder;
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	mp_init_multi(&quotient, &remainder, NULL);
	mp_div(VALUE (a), VALUE (b), &quotient, &remainder);
	
	int size;
	char *str;
	
	mp_radix_size(&remainder, 10, &size);
	str = st_malloc(size);
	mp_toradix(&remainder, str, 10);
	if (mp_cmp_d(&remainder, 0) == MP_EQ) {
		result = st_large_integer_new(&quotient);
		ST_STACK_PUSH (machine, result);
		mp_clear(&remainder);
	} else {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 2);
		mp_clear_multi(&quotient, &remainder, NULL);
	}
}

static void prim_large_int_div(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_DIV_OP (mp_div, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_mod(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_mod, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_gcd(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_gcd, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_lcm(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_lcm, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_eq(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	int relation = mp_cmp(VALUE (a), VALUE (b));
	st_oop result = (relation == MP_EQ) ? ST_TRUE : ST_FALSE;
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_ne(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	int relation = mp_cmp(VALUE (a), VALUE (b));
	st_oop result = (relation == MP_EQ) ? ST_FALSE : ST_TRUE;
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_lt(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	int relation = mp_cmp(VALUE (a), VALUE (b));
	st_oop result = (relation == MP_LT) ? ST_TRUE : ST_FALSE;
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_gt(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	
	
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	int relation = mp_cmp(VALUE (a), VALUE (b));
	st_oop result = (relation == MP_GT) ? ST_TRUE : ST_FALSE;
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_le(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	int relation = mp_cmp(VALUE (a), VALUE (b));
	st_oop result = (relation == MP_LT || relation == MP_EQ) ? ST_TRUE : ST_FALSE;
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_ge(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	int relation = mp_cmp(VALUE (a), VALUE (b));
	st_oop result = (relation == MP_GT || relation == MP_EQ) ? ST_TRUE : ST_FALSE;
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_squared(st_machine *machine) {
	st_oop receiver = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 1);
		return;
	}
	UNARY_OP (mp_sqr, receiver);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_bitOr(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_or, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_bitAnd(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_and, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_bitXor(st_machine *machine) {
	st_oop b = prim_pop_large_int(machine);
	st_oop a = prim_pop_large_int(machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	BINARY_OP (mp_xor, a, b);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_bitShift(st_machine *machine) {
	int displacement = pop_integer32(machine);
	st_oop receiver = prim_pop_large_int(machine);
	st_oop result;
	mp_int value;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	mp_init(&value);
	if (displacement >= 0)
		mp_mul_2d(VALUE (receiver), displacement, &value);
	else
		mp_div_2d(VALUE (receiver), abs(displacement), &value, NULL);
	result = st_large_integer_new(&value);
	ST_STACK_PUSH (machine, result);
}

static void prim_large_int_asFloat(st_machine *machine) {
	st_oop receiver = prim_pop_large_int(machine);
	double result;
	mp_int *m;
	int i;
	m = st_large_integer_value(receiver);
	if (m->used == 0) {
		ST_STACK_PUSH (machine, st_float_new(0));
		return;
	}
	i = m->used;
	result = DIGIT (m, i);
	while (--i >= 0)
		result = (result * ST_DIGIT_RADIX) + DIGIT (m, i);
	
	if (m->sign == MP_NEG)
		result = -result;
	ST_STACK_PUSH (machine, st_float_new(result));
}

static void prim_large_int_printStringBase(st_machine *machine) {
	int radix = pop_integer(machine);
	st_oop x = prim_pop_large_int(machine);
	char *string;
	st_oop result;
	if (radix < 2 || radix > 36)
		set_success(machine, false);
	if (machine->success) {
		string = st_large_integer_to_string(x, radix);
		result = st_string_new(string);
	}
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_large_int_hash(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	mp_int *value;
	int result;
	const char *c;
	unsigned int hash;
	size_t len;
	value = st_large_integer_value(receiver);
	c = (const char *) value->dp;
	len = value->used * sizeof(mp_digit);
	hash = 5381;
	for (size_t i = 0; i < len; i++)
		if (c[i])
			hash = ((hash << 5) + hash) + c[i];
	
	result = hash;
	if (result < 0)
		result = -result;
	ST_STACK_PUSH (machine, st_smi_new(result));
}

static st_oop prim_pop_float(st_machine *machine) {
	st_oop object = ST_STACK_POP (machine);
	set_success(machine, st_object_class(object) == ST_FLOAT_CLASS);
	return object;
}

static void prim_float_add(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	if (machine->success)
		result = st_float_new(st_float_value(x) + st_float_value(y));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_sub(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = st_float_new(st_float_value(x) - st_float_value(y));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_lt(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = isless (st_float_value(x), st_float_value(y)) ? ST_TRUE : ST_FALSE;
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_gt(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = isgreater (st_float_value(x), st_float_value(y)) ? ST_TRUE : ST_FALSE;
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_le(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = islessequal (st_float_value(x), st_float_value(y)) ? ST_TRUE : ST_FALSE;
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_ge(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = isgreaterequal (st_float_value(x), st_float_value(y)) ? ST_TRUE : ST_FALSE;
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_eq(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = (st_float_value(x) == st_float_value(y)) ? ST_TRUE : ST_FALSE;
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_ne(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = (st_float_value(x) != st_float_value(y)) ? ST_TRUE : ST_FALSE;
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_mul(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	if (machine->success)
		result = st_float_new(st_float_value(x) * st_float_value(y));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_div(st_machine *machine) {
	st_oop y = prim_pop_float(machine);
	st_oop x = prim_pop_float(machine);
	st_oop result = ST_NIL;
	
	set_success(machine, y != 0);
	
	if (machine->success)
		result = st_float_new(st_float_value(x) / st_float_value(y));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_float_sin(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(sin(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_cos(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(cos(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_tan(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(tan(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_arcSin(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(asin(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_arcCos(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(acos(value));
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_arcTan(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(atan(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_sqrt(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(sqrt(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_log(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(log10(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_ln(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(log(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_exp(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double value = st_float_value(receiver);
	st_oop result = st_float_new(exp(value));
	
	if (machine->success)
		ST_STACK_PUSH (machine, result);
	else
		ST_STACK_UNPOP (machine, 1);
}

static void prim_float_truncated(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	int result = (int) trunc(st_float_value(receiver));
	ST_STACK_PUSH (machine, st_smi_new(result));
}

static void prim_float_fractionPart(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double frac_part, int_part;
	st_oop result;
	
	frac_part = modf(st_float_value(receiver), &int_part);
	result = st_float_new(frac_part);
	ST_STACK_PUSH (machine, result);
}

static void prim_float_integerPart(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	double int_part;
	st_oop result;
	modf(st_float_value(receiver), &int_part);
	result = st_smi_new((int) int_part);
	ST_STACK_PUSH (machine, result);
}

static void prim_float_hash(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	unsigned int hash = 0;
	int result;
	double value;
	unsigned char *c;
	
	value = st_float_value(receiver);
	if (value == 0)
		value = fabs(value);
	
	c = (unsigned char *) &value;
	for (size_t i = 0; i < sizeof(double); i++)
		hash = (hash * 971) ^ c[i];
	result = hash;
	if (result < 0)
		result = -result;
	ST_STACK_PUSH (machine, st_smi_new(result));
}

static void prim_float_printStringBase(st_machine *machine) {
	(void) pop_integer(machine);
	st_oop receiver = ST_STACK_POP (machine);
	char *tmp;
	st_oop string;
	
	if (!machine->success ||
	    !st_object_is_heap(receiver) ||
	    st_object_format(receiver) != ST_FORMAT_FLOAT) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	
	// ignore base for the time being
	tmp = st_strdup_printf("%g", st_float_value(receiver));
	string = st_string_new(tmp);
	st_free(tmp);
	ST_STACK_PUSH (machine, string);
}

static void prim_obj_error(st_machine *machine) {
	st_oop message;
	st_oop traceback;
	traceback = ST_STACK_POP (machine);
	message = ST_STACK_POP (machine);
	(void) ST_STACK_POP (machine);
	
	if (!st_object_is_heap(traceback) ||
	    st_object_format(traceback) != ST_FORMAT_BYTE_ARRAY) {
		abort(); // can't resume execution in this prim
	}
	
	if (!st_object_is_heap(message) ||
	    st_object_format(message) != ST_FORMAT_BYTE_ARRAY) {
	}
	
	printf("An error occurred during program execution\n");
	printf("message: %s\n\n", st_byte_array_bytes(message));
	
	printf("Traceback:\n");
	puts(st_byte_array_bytes(traceback));
	/* set success to false to signal error */
	machine->success = false;
	longjmp(machine->main_loop, 0);
}

static void prim_obj_class(st_machine *machine) {
	st_oop object;
	object = ST_STACK_POP (machine);
	ST_STACK_PUSH (machine, st_object_class(object));
}

static void prim_obj_identityHash(st_machine *machine) {
	st_oop object;
	st_uint hash;
	
	object = ST_STACK_POP (machine);
	if (st_object_is_smi(object))
		hash = st_smi_hash(object);
	else if (st_object_is_character(object))
		hash = st_character_hash(object);
	else {
		st_object_set_hashed(object, true);
		hash = st_identity_hashtable_hash(memory->ht, object);
	}
	ST_STACK_PUSH (machine, st_smi_new(hash));
}

static void prim_obj_copy(st_machine *machine) {
	
	st_oop copy;
	st_oop class;
	int size;
	
	(void) ST_STACK_POP (machine);
	if (!st_object_is_heap(machine->message_receiver)) {
		ST_STACK_PUSH (machine, machine->message_receiver);
		return;
	}
	
	switch (st_object_format(machine->message_receiver)) {
		case ST_FORMAT_OBJECT: {
			class = ST_OBJECT_CLASS (machine->message_receiver);
			size = st_smi_value(ST_BEHAVIOR_INSTANCE_SIZE (class));
			copy = st_object_new(class);
			st_oops_copy(ST_OBJECT_FIELDS (copy), ST_OBJECT_FIELDS (machine->message_receiver), size);
			break;
		}
		case ST_FORMAT_ARRAY: {
			size = st_smi_value(ST_ARRAYED_OBJECT (machine->message_receiver)->size);
			copy = st_object_new_arrayed(ST_OBJECT_CLASS (machine->message_receiver), size);
			st_oops_copy(ST_ARRAY (copy)->elements, ST_ARRAY (machine->message_receiver)->elements, size);
			break;
		}
		case ST_FORMAT_BYTE_ARRAY: {
			size = st_smi_value(ST_ARRAYED_OBJECT (machine->message_receiver)->size);
			copy = st_object_new_arrayed(ST_OBJECT_CLASS (machine->message_receiver), size);
			memcpy(st_byte_array_bytes(copy), st_byte_array_bytes(machine->message_receiver), size);
			break;
		}
		case ST_FORMAT_FLOAT_ARRAY: {
			size = st_smi_value(st_arrayed_object_size(machine->message_receiver));
			copy = st_object_new_arrayed(ST_OBJECT_CLASS (machine->message_receiver), size);
			memcpy(st_float_array_elements(copy),
					st_float_array_elements(machine->message_receiver),
					sizeof(double) * size);
			break;
		}
		case ST_FORMAT_WORD_ARRAY: {
			size = st_smi_value(st_arrayed_object_size(machine->message_receiver));
			copy = st_object_new_arrayed(ST_OBJECT_CLASS (machine->message_receiver), size);
			memcpy(st_word_array_elements(copy),
					st_word_array_elements(machine->message_receiver),
					sizeof(st_uint) * size);
			break;
		}
		case ST_FORMAT_FLOAT: {
			copy = st_object_new(ST_FLOAT_CLASS);
			st_float_set_value(copy, st_float_value(machine->message_receiver));
			break;
		}
		case ST_FORMAT_LARGE_INTEGER: {
			int result;
			copy = st_object_new(ST_LARGE_INTEGER_CLASS);
			result = mp_init_copy(st_large_integer_value(copy), st_large_integer_value(machine->message_receiver));
			if (result != MP_OKAY)
				abort();
			break;
		}
		case ST_FORMAT_HANDLE:
			copy = st_object_new(ST_HANDLE_CLASS);
			ST_HANDLE_VALUE (copy) = ST_HANDLE_VALUE (machine->message_receiver);
			break;
		case ST_FORMAT_CONTEXT:
		case ST_FORMAT_INTEGER_ARRAY:
		default:
			abort(); /* not implemented yet */
	}
	ST_STACK_PUSH (machine, copy);
}

static void prim_obj_equivalent(st_machine *machine) {
	st_oop y = ST_STACK_POP (machine);
	st_oop x = ST_STACK_POP (machine);
	ST_STACK_PUSH (machine, ((x == y) ? ST_TRUE : ST_FALSE));
}

static st_oop prim_lookup_method(st_oop class, st_oop selector) {
	st_oop method;
	st_oop parent = class;
	while (parent != ST_NIL) {
		method = st_dictionary_at(ST_BEHAVIOR_METHOD_DICTIONARY (parent), selector);
		if (method != ST_NIL)
			return method;
		parent = ST_BEHAVIOR_SUPERCLASS (parent);
	}
	return 0;
}

static void prim_obj_perform(st_machine *machine) {
	st_oop receiver;
	st_oop selector;
	st_oop method;
	st_uint selector_index;
	
	selector = machine->message_selector;
	machine->message_selector = machine->stack[machine->sp - machine->message_argcount];
	receiver = machine->message_receiver;
	set_success(machine, st_object_is_symbol(machine->message_selector));
	method = prim_lookup_method(st_object_class(receiver), machine->message_selector);
	set_success(machine, (size_t) st_method_get_arg_count(method) == (machine->message_argcount - 1));
	
	if (machine->success) {
		selector_index = machine->sp - machine->message_argcount;
		st_oops_move(machine->stack + selector_index,
				machine->stack + selector_index + 1,
				machine->message_argcount - 1);
		machine->sp -= 1;
		machine->message_argcount -= 1;
		machine->new_method = method;
		st_machine_execute_method(machine);
	} else
		machine->message_selector = selector;
}

static void prim_obj_perform_withArguments(st_machine *machine) {
	st_oop receiver;
	st_oop selector;
	st_oop method;
	st_oop array;
	int array_size;
	
	array = ST_STACK_POP (machine);
	set_success(machine, st_object_format(array) == ST_FORMAT_ARRAY);
	
	if (ST_OBJECT_CLASS (machine->context) == ST_BLOCK_CONTEXT_CLASS)
		method = ST_METHOD_CONTEXT_METHOD (ST_BLOCK_CONTEXT_HOME(machine->context));
	else
		method = ST_METHOD_CONTEXT_METHOD (machine->context);
	
	array_size = st_smi_value(st_arrayed_object_size(array));
	set_success(machine, (machine->sp + array_size - 1) < 32);
	
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 1);
		return;
	}
	
	selector = machine->message_selector;
	machine->message_selector = ST_STACK_POP (machine);
	receiver = ST_STACK_PEEK (machine);
	machine->message_argcount = array_size;
	set_success(machine, st_object_is_symbol(machine->message_selector));
	st_oops_copy(machine->stack + machine->sp, st_array_elements(array), array_size);
	machine->sp += array_size;
	method = prim_lookup_method(st_object_class(receiver), machine->message_selector);
	set_success(machine, st_method_get_arg_count(method) == array_size);
	
	if (machine->success) {
		machine->new_method = method;
		st_machine_execute_method(machine);
	} else {
		machine->sp -= machine->message_argcount;
		ST_STACK_PUSH (machine, machine->message_selector);
		ST_STACK_PUSH (machine, array);
		machine->message_argcount = 2;
		machine->message_selector = selector;
	}
}

static void prim_behavior_new(st_machine *machine) {
	st_oop class;
	st_oop instance;
	class = ST_STACK_POP (machine);
	switch (st_smi_value(ST_BEHAVIOR_FORMAT (class))) {
		case ST_FORMAT_OBJECT:
			instance = st_object_allocate(class);
			break;
		case ST_FORMAT_CONTEXT: /* not implemented */
			abort();
			break;
		case ST_FORMAT_FLOAT:
			instance = st_float_allocate(class);
			break;
		case ST_FORMAT_LARGE_INTEGER:
			instance = st_large_integer_allocate(class, NULL);
			break;
		case ST_FORMAT_HANDLE:
			instance = st_handle_allocate(class);
			break;
		default: /* should not reach */
			abort();
	}
	ST_STACK_PUSH (machine, instance);
}

static void prim_behavior_newSize(st_machine *machine) {
	int size;
	st_oop class;
	st_oop instance;
	size = pop_integer32(machine);
	class = ST_STACK_POP (machine);
	
	switch (st_smi_value(ST_BEHAVIOR_FORMAT (class))) {
		case ST_FORMAT_ARRAY:
			instance = st_array_allocate(class, size);
			break;
		case ST_FORMAT_BYTE_ARRAY:
			instance = st_byte_array_allocate(class, size);
			break;
		case ST_FORMAT_WORD_ARRAY:
			instance = st_word_array_allocate(class, size);
			break;
		case ST_FORMAT_FLOAT_ARRAY:
			instance = st_float_array_allocate(class, size);
			break;
		case ST_FORMAT_INTEGER_ARRAY:
			abort(); /* not implemented */
			break;
		default:
			abort(); /* should not reach */
	}
	ST_STACK_PUSH (machine, instance);
}

static void prim_behavior_compile(st_machine *machine) {
	st_compiler_error error;
	st_oop receiver;
	st_oop string;
	
	string = ST_STACK_POP (machine);
	receiver = ST_STACK_POP (machine);
	if (!st_object_is_heap(string) || st_object_format(string) != ST_FORMAT_BYTE_ARRAY) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	
	if (!st_compile_string(receiver, (char *) st_byte_array_bytes(string), &error)) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	ST_STACK_PUSH (machine, receiver);
}

static void prim_seq_collection_size(st_machine *machine) {
	st_oop object;
	object = ST_STACK_POP (machine);
	ST_STACK_PUSH (machine, st_arrayed_object_size(object));
}

static void prim_array_at(st_machine *machine) {
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	ST_STACK_PUSH (machine, st_array_at(receiver, index));
}

static void prim_array_at_put(st_machine *machine) {
	st_oop object = ST_STACK_POP (machine);
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	st_array_at_put(receiver, index, object);
	ST_STACK_PUSH (machine, object);
}

static void prim_byteArray_at(st_machine *machine) {
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	st_oop result;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	result = st_smi_new(st_byte_array_at(receiver, index));
	ST_STACK_PUSH (machine, result);
}

static void prim_byteArray_at_put(st_machine *machine) {
	int byte = pop_integer(machine);
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	st_byte_array_at_put(receiver, index, byte);
	ST_STACK_PUSH (machine, st_smi_new(byte));
}

static void prim_byteArray_hash(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	st_uint hash;
	hash = st_byte_array_hash(receiver);
	ST_STACK_PUSH (machine, st_smi_new(hash));
}

static void prim_byteString_at(st_machine *machine) {
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	st_oop character;
	if (ST_UNLIKELY (!machine->success)) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	character = st_character_new(st_byte_array_at(receiver, index));
	ST_STACK_PUSH (machine, character);
}

static void prim_byteString_at_put(st_machine *machine) {
	st_oop character = ST_STACK_POP (machine);
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	set_success(machine, st_object_class(character) == ST_CHARACTER_CLASS);
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	st_byte_array_at_put(receiver, index, (st_uchar) st_character_value(character));
	ST_STACK_PUSH (machine, character);
}

static void prim_byteString_size(st_machine *machine) {
	st_oop receiver;
	st_uint size;
	receiver = ST_STACK_POP (machine);
	size = st_arrayed_object_size(receiver);
	/* TODO: allow size to go into a LargeInteger on overflow */
	ST_STACK_PUSH (machine, size);
}

static void prim_byteString_compare(st_machine *machine) {
	st_oop argument = ST_STACK_POP (machine);
	st_oop receiver = ST_STACK_POP (machine);
	int order;
	
	if (st_object_format(argument) != ST_FORMAT_BYTE_ARRAY)
		set_success(machine, false);
	
	if (machine->success)
		order = strcmp((const char *) st_byte_array_bytes(receiver), (const char *) st_byte_array_bytes(argument));
	
	if (machine->success)
		ST_STACK_PUSH (machine, st_smi_new(order));
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_wideString_at(st_machine *machine) {
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	
	st_unichar c;
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	
	if (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	c = st_word_array_at(receiver, index);
	ST_STACK_PUSH (machine, st_character_new(c));
}

static void prim_wideString_at_put(st_machine *machine) {
	st_oop character = ST_STACK_POP (machine);
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	set_success(machine, st_object_class(character) == ST_CHARACTER_CLASS);
	if (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	st_word_array_at_put(receiver, index, character);
	ST_STACK_PUSH (machine, character);
}

static void prim_wordArray_at(st_machine *machine) {
	st_oop receiver;
	int index;
	st_uint element;
	
	index = pop_integer32(machine);
	receiver = ST_STACK_POP (machine);
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	element = st_word_array_at(receiver, index);
	ST_STACK_PUSH (machine, st_smi_new(element));
}

static void prim_wordArray_at_put(st_machine *machine) {
	int value = pop_integer(machine);
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	st_word_array_at_put(receiver, index, value);
	ST_STACK_PUSH (machine, st_smi_new(value));
}

static void prim_floatArray_at(st_machine *machine) {
	st_oop receiver;
	int index;
	double element;
	
	index = pop_integer32(machine);
	receiver = ST_STACK_POP (machine);
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	element = st_float_array_at(receiver, index);
	ST_STACK_PUSH (machine, st_float_new(element));
}

static void prim_floatArray_at_put(st_machine *machine) {
	st_oop flt = ST_STACK_POP (machine);
	int index = pop_integer32(machine);
	st_oop receiver = ST_STACK_POP (machine);
	set_success(machine, st_object_is_heap(flt) && st_object_format(flt) == ST_FORMAT_FLOAT);
	if (ST_UNLIKELY (index < 1 || index > st_smi_value(st_arrayed_object_size(receiver)))) {
		set_success(machine, false);
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	if (!machine->success) {
		ST_STACK_UNPOP (machine, 3);
		return;
	}
	st_float_array_at_put(receiver, index, st_float_value(flt));
	ST_STACK_PUSH (machine, flt);
}

static void prim_blockContext_value(st_machine *machine) {
	st_oop block;
	size_t argcount;
	block = machine->message_receiver;
	argcount = st_smi_value(ST_BLOCK_CONTEXT_ARGCOUNT (block));
	if (ST_UNLIKELY (argcount != machine->message_argcount)) {
		machine->success = false;
		return;
	}
	st_oops_copy(ST_BLOCK_CONTEXT_STACK (block), machine->stack + machine->sp - argcount, argcount);
	machine->sp -= machine->message_argcount + 1;
	ST_CONTEXT_PART_IP (block) = ST_BLOCK_CONTEXT_INITIALIP (block);
	ST_CONTEXT_PART_SP (block) = st_smi_new(argcount);
	ST_CONTEXT_PART_SENDER (block) = machine->context;
	st_machine_set_active_context(machine, block);
}

static void prim_blockContext_valueWithArguments(st_machine *machine) {
	st_oop block;
	st_oop values;
	int argcount;
	
	block = machine->message_receiver;
	values = ST_STACK_PEEK (machine);
	if (st_object_class(values) != ST_ARRAY_CLASS) {
		set_success(machine, false);
		return;
	}
	
	argcount = st_smi_value(ST_BLOCK_CONTEXT_ARGCOUNT (block));
	if (argcount != st_smi_value(st_arrayed_object_size(values))) {
		set_success(machine, false);
		return;
	}
	
	st_oops_copy(ST_BLOCK_CONTEXT_STACK (block), ST_ARRAY (values)->elements, argcount);
	machine->sp -= machine->message_argcount + 1;
	ST_CONTEXT_PART_IP (block) = ST_BLOCK_CONTEXT_INITIALIP (block);
	ST_CONTEXT_PART_SP (block) = st_smi_new(argcount);
	ST_CONTEXT_PART_SENDER (block) = machine->context;
	st_machine_set_active_context(machine, block);
}

static void prim_sys_exitWithResult(st_machine *machine) {
	machine->success = true; // set success to true to signal that everything was alright */
	longjmp(machine->main_loop, 0);
}

static void prim_char_value(st_machine *machine) {
	st_oop receiver = ST_STACK_POP (machine);
	ST_STACK_PUSH (machine, st_smi_new(st_character_value(receiver)));
}

static void prim_char_for(st_machine *machine) {
	int value;
	value = pop_integer(machine);
	(void) ST_STACK_POP (machine);
	
	if (machine->success)
		ST_STACK_PUSH (machine, st_character_new(value));
	else
		ST_STACK_UNPOP (machine, 2);
}

static void prim_file_stream_open(st_machine *machine) {
	st_oop filename;
	st_oop handle;
	char *str;
	int mode;
	int fd;
	
	mode = pop_integer32(machine);
	filename = ST_STACK_POP (machine);
	if (st_object_format(filename) != ST_FORMAT_BYTE_ARRAY) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	
	if (mode != O_RDONLY || mode != O_WRONLY) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	
	str = st_byte_array_bytes(filename);
	fd = open(str, O_WRONLY | O_CREAT, 0644);
	if (fd < 0) {
		fprintf(stderr, "%s", strerror(errno));
		machine->success = false;
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	
	ftruncate(fd, 0);
	(void) ST_STACK_POP (machine); // pop receiver
	handle = st_object_new(ST_HANDLE_CLASS);
	ST_HANDLE_VALUE (handle) = fd;
	ST_STACK_PUSH (machine, handle);
}

static void prim_file_stream_close(st_machine *machine) {
	st_oop handle;
	int fd;
	handle = ST_STACK_POP (machine);
	fd = ST_HANDLE_VALUE (handle);
	if (close(fd) < 0) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 1);
		return;
	}
	// leave receiver on stack
}

static void prim_file_stream_write(st_machine *machine) {
	st_oop handle;
	st_oop array;
	int fd;
	char *buffer;
	size_t total, size;
	ssize_t count;
	
	array = ST_STACK_POP (machine);
	handle = ST_STACK_POP (machine);
	if (st_object_format(array) != ST_FORMAT_BYTE_ARRAY) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 1);
		return;
	}
	if (st_object_format(handle) != ST_FORMAT_HANDLE) {
		machine->success = false;
		ST_STACK_UNPOP (machine, 2);
		return;
	}
	
	fd = ST_HANDLE_VALUE (handle);
	buffer = st_byte_array_bytes(array);
	size = st_smi_value(st_arrayed_object_size(array));
	total = 0;
	while (total < size) {
		count = write(fd, buffer + total, size - total);
		if (count < 0) {
			machine->success = false;
			ST_STACK_UNPOP (machine, 2);
			return;
		}
		total += count;
	}
	// leave receiver on stack
}

static void prim_file_stream_seek(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_file_stream_read(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}


static void prim_process_fork(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_process_yield(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_process_kill(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_thread_create(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_thread_suspend(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_thread_yield(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_thread_join(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_thread_wait(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_in_byte(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

static void prim_out_byte(st_machine *machine) {
	(void) machine;
	abort(); // not implemented yet
}

const struct st_primitive st_primitives[] = {
		{"small_int_add",                 prim_small_int_add},
		{"small_int_sub",                 prim_small_int_sub},
		{"small_int_lt",                  prim_small_int_lt},
		{"small_int_gt",                  prim_small_int_gt},
		{"small_int_le",                  prim_small_int_le},
		{"small_int_ge",                  prim_small_int_ge},
		{"small_int_eq",                  prim_small_int_eq},
		{"small_int_ne",                  prim_small_int_ne},
		{"small_int_mul",                 prim_small_int_mul},
		{"small_int_div_sel",             prim_small_int_div_sel},
		{"small_int_div",                 prim_small_int_div},
		{"small_int_mod",                 prim_small_int_mod},
		{"small_int_bitOr",               prim_small_int_bitOr},
		{"small_int_bitXor",              prim_small_int_bitXor},
		{"small_int_bitAnd",              prim_small_int_bitAnd},
		{"small_int_bitShift",            prim_small_int_bitShift},
		{"small_int_asFloat",             prim_small_int_asFloat},
		{"small_int_asLargeInt",          prim_small_int_asLargeInt},
		{"large_int_add",                 prim_large_int_add},
		{"large_int_sub",                 prim_large_int_sub},
		{"large_int_lt",                  prim_large_int_lt},
		{"large_int_gt",                  prim_large_int_gt},
		{"large_int_le",                  prim_large_int_le},
		{"large_int_ge",                  prim_large_int_ge},
		{"large_int_eq",                  prim_large_int_eq},
		{"large_int_ne",                  prim_large_int_ne},
		{"large_int_mul",                 prim_large_int_mul},
		{"large_int_div_sel",             prim_large_int_div},
		{"large_int_div",                 prim_large_int_div},
		{"large_int_mod",                 prim_large_int_mod},
		{"large_int_gcd",                 prim_large_int_gcd},
		{"large_int_lcm",                 prim_large_int_lcm},
		{"large_int_squared",             prim_large_int_squared},
		{"large_int_bitOr",               prim_large_int_bitOr},
		{"large_int_bitXor",              prim_large_int_bitXor},
		{"large_int_bitAnd",              prim_large_int_bitAnd},
		{"large_int_bitShift",            prim_large_int_bitShift},
		{"large_int_print_string_base",   prim_large_int_printStringBase},
		{"large_int_asFloat",             prim_large_int_asFloat},
		{"large_int_hash",                prim_large_int_hash},
		{"float_add",                     prim_float_add},
		{"float_sub",                     prim_float_sub},
		{"float_lt",                      prim_float_lt},
		{"float_gt",                      prim_float_gt},
		{"float_le",                      prim_float_le},
		{"float_ge",                      prim_float_ge},
		{"float_eq",                      prim_float_eq},
		{"float_ne",                      prim_float_ne},
		{"float_mul",                     prim_float_mul},
		{"float_div",                     prim_float_div},
		{"float_exp",                     prim_float_exp},
		{"float_sin",                     prim_float_sin},
		{"float_cos",                     prim_float_cos},
		{"float_tan",                     prim_float_tan},
		{"float_arc_sin",                 prim_float_arcSin},
		{"float_arc_cos",                 prim_float_arcCos},
		{"float_arc_tan",                 prim_float_arcTan},
		{"float_ln",                      prim_float_ln},
		{"float_log",                     prim_float_log},
		{"float_sqrt",                    prim_float_sqrt},
		{"float_truncated",               prim_float_truncated},
		{"float_fractionPart",            prim_float_fractionPart},
		{"float_integerPart",             prim_float_integerPart},
		{"float_hash",                    prim_float_hash},
		{"float_print_string_base",       prim_float_printStringBase},
		{"object_error",                  prim_obj_error},
		{"object_class",                  prim_obj_class},
		{"object_identity_hash",          prim_obj_identityHash},
		{"object_copy",                   prim_obj_copy},
		{"object_equivalent",             prim_obj_equivalent},
		{"object_perform",                prim_obj_perform},
		{"object_perform_with_args",      prim_obj_perform_withArguments},
		{"behavior_new",                  prim_behavior_new},
		{"behavior_new_size",             prim_behavior_newSize},
		{"behavior_compile",              prim_behavior_compile},
		{"sequenceable_collection_size",  prim_seq_collection_size},
		{"array_at",                      prim_array_at},
		{"array_at_put",                  prim_array_at_put},
		{"byte_array_at",                 prim_byteArray_at},
		{"byte_array_at_put",             prim_byteArray_at_put},
		{"byte_array_hash",               prim_byteArray_hash},
		{"byte_string_at",                prim_byteString_at},
		{"byte_string_at_put",            prim_byteString_at_put},
		{"byte_string_size",              prim_byteString_size},
		{"byte_string_compare",           prim_byteString_compare},
		{"wide_string_at",                prim_wideString_at},
		{"wide_string_at_put",            prim_wideString_at_put},
		{"word_array_at",                 prim_wordArray_at},
		{"word_array_at_put",             prim_wordArray_at_put},
		{"float_array_at",                prim_floatArray_at},
		{"float_array_at_put",            prim_floatArray_at_put},
		{"sys_exit_with_result",          prim_sys_exitWithResult},
		{"char_value",                    prim_char_value},
		{"char_for",                      prim_char_for},
		{"block_context_value",           prim_blockContext_value},
		{"block_context_value_with_args", prim_blockContext_valueWithArguments},
		{"file_stream_open",              prim_file_stream_open},
		{"file_stream_close",             prim_file_stream_close},
		{"file_stream_read",              prim_file_stream_read},
		{"file_stream_write",             prim_file_stream_write},
		{"file_stream_seek",              prim_file_stream_seek},
};

// returns 0 if there no primitive function corresponding to the given name
int st_prim_index_for_name(const char *name) {
	st_assert (name != NULL);
	for (size_t i = 0; i < ST_N_ELEMENTS (st_primitives); i++)
		if (streq (name, st_primitives[i].name))
			return i;
	return -1;
}