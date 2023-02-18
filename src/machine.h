
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include <setjmp.h>
#include "st-types.h"

// cache size must be a power of 2
#define ST_METHOD_CACHE_SIZE      1024
#define ST_METHOD_CACHE_MASK      (ST_METHOD_CACHE_SIZE - 1)
#define ST_METHOD_CACHE_HASH(k, s) ((k) ^ (s))

#define ST_NUM_GLOBALS   36
#define ST_NUM_SELECTORS 24

typedef struct st_method_cache {
	st_oop class;
	st_oop selector;
	st_oop method;
} st_method_cache;

typedef struct st_machine st_machine;

struct st_machine {
	st_oop context;
	st_oop receiver;
	st_oop method;
	st_uchar *bytecode;
	st_oop *temps;
	st_oop *stack;
	st_oop lookup_class;
	st_oop message_receiver;
	st_oop message_selector;
	size_t message_argcount;
	st_oop new_method;
	bool success;
	st_uint ip;
	st_uint sp;
	jmp_buf main_loop;
	st_method_cache method_cache[ST_METHOD_CACHE_SIZE];
	st_oop globals[ST_NUM_GLOBALS];
	st_oop selectors[ST_NUM_SELECTORS];
};

extern st_machine __machine;

#define ST_STACK_POP(machine)          (machine->stack[--machine->sp])
#define ST_STACK_PUSH(machine, oop)    (machine->stack[machine->sp++] = oop)
#define ST_STACK_PEEK(machine)         (machine->stack[machine->sp-1])
#define ST_STACK_UNPOP(machine, count) (machine->sp += count)

void st_machine_main(st_machine *machine);

void st_machine_initialize(st_machine *machine);

void st_machine_set_active_context(st_machine *machine, st_oop context);

void st_machine_execute_method(st_machine *machine);

st_oop st_machine_lookup_method(st_machine *machine, st_oop class);

void st_machine_clear_caches(st_machine *machine);

