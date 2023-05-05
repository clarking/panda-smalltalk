
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#include <stdlib.h>
#include <setjmp.h>
#include <limits.h>
#include "types.h"
#include "compiler.h"
#include "universe.h"
#include "dictionary.h"
#include "object.h"
#include "behavior.h"
#include "context.h"
#include "primitives.h"
#include "method.h"
#include "array.h"
#include "association.h"
#include "memory.h"

VirtualMachine __machine;

static inline Oop method_context_new(VirtualMachine *machine) {

	Oop context;
	uint temp_count;
	Oop *stack;
	temp_count = st_method_get_arg_count(machine->new_method) + st_method_get_temp_count(machine->new_method);
	context = st_memory_allocate_context();

	ContextPart_SENDER (context) = machine->context;
	ContextPart_IP (context) = st_smi_new(0);
	ContextPart_SP (context) = st_smi_new((int) temp_count);
	MethodContext_RECEIVER (context) = machine->message_receiver;
	MethodContext_METHOD (context) = machine->new_method;

	// clear temporaries (and nothing above)
	stack = MethodContext_STACK (context);
	for (uint i = 0; i < temp_count; i++)
		stack[i] = ST_NIL;

	return context;
}

static Oop block_context_new(VirtualMachine *machine, uint initial_ip, uint argcount) {
	Oop home;
	Oop context;
	uint stack_size;

	stack_size = 32;
	context = st_memory_allocate(ST_SIZE_OOPS(BlockContext) + stack_size);
	if (context == 0) {
		st_memory_perform_gc();
		context = st_memory_allocate(ST_SIZE_OOPS(BlockContext) + stack_size);
		st_assert (context != 0);
	}

	st_object_initialize_header(context, ST_BLOCK_CONTEXT_CLASS);
	if (ST_OBJECT_CLASS(machine->context) == ST_BLOCK_CONTEXT_CLASS)
		home = BlockContext_HOME (machine->context);
	else
		home = machine->context;

	ContextPart_SENDER (context) = ST_NIL;
	ContextPart_IP (context) = st_smi_new(0);
	ContextPart_SP (context) = st_smi_new(0);
	BlockContext_INITIALIP (context) = st_smi_new((int) initial_ip);
	BlockContext_ARGCOUNT (context) = st_smi_new((int) argcount);
	BlockContext_HOME (context) = home;
	return context;
}

static void create_actual_message(VirtualMachine *machine) {
	Oop *elements;
	Oop message;
	Oop array;

	array = st_object_new_arrayed(ST_ARRAY_CLASS, machine->message_argcount);
	elements = st_array_elements(array);
	for (uint i = 0; i < machine->message_argcount; i++)
		elements[i] = machine->stack[machine->sp - machine->message_argcount + i];

	machine->sp -= machine->message_argcount;
	message = st_object_new(ST_MESSAGE_CLASS);
	if (message == 0) {
		st_memory_perform_gc();
		message = st_object_new(ST_MESSAGE_CLASS);
		st_assert (message != 0);
	}

	ST_OBJECT_FIELDS(message)[0] = machine->message_selector;
	ST_OBJECT_FIELDS(message)[1] = array;
	ST_STACK_PUSH (machine, message);

	machine->message_selector = ST_SELECTOR_DOESNOTUNDERSTAND;
	machine->message_argcount = 1;
}

static Oop lookup_method(VirtualMachine *machine, Oop class) {
	Oop method, dict, parent;
	uint hash;

	parent = class;
	hash = st_byte_array_hash(machine->message_selector);
	while (parent != ST_NIL) {

		Oop el;
		uint mask, i;
		dict = ST_BEHAVIOR_METHOD_DICTIONARY (parent);
		mask = st_smi_value(st_arrayed_object_size(ST_OBJECT_FIELDS(dict)[2])) - 1;
		i = (hash & mask) + 1;

		while (true) {
			el = st_array_at(ST_OBJECT_FIELDS(dict)[2], i);
			if (el == ST_NIL || el == (uintptr_t) ST_OBJECT_FIELDS(dict)[2])
				break;
			if (machine->message_selector == ST_ASSOCIATION_KEY (el))
				return ST_ASSOCIATION_VALUE (el);
			i = ((i + ST_ADVANCE_SIZE) & mask) + 1;
		}

		parent = ST_BEHAVIOR_SUPERCLASS (parent);
	}

	if (machine->message_selector == ST_SELECTOR_DOESNOTUNDERSTAND) {
		fprintf(stderr, "byte_string: no method found for #doesNotUnderstand:\n");
		exit(1);
	}

	create_actual_message(machine);
	return lookup_method(machine, class);
}

Oop st_machine_lookup_method(VirtualMachine *machine, Oop class) {
	return lookup_method(machine, class);
}

/* 
 * Creates a new method context. Parameterised by
 * @sender, @receiver, @method, and @argcount
 *
 * Message arguments are copied into the new context's temporary
 * frame. Receiver and arguments are then popped off the stack.
 *
 */
static inline void activate_method(VirtualMachine *machine) {
	Oop context = method_context_new(machine);
	Oop *arguments = MethodContext_STACK (context);
	for (uint i = 0; i < machine->message_argcount; i++)
		arguments[i] = machine->stack[machine->sp - machine->message_argcount + i];

	machine->sp -= machine->message_argcount + 1;
	st_machine_set_active_context(machine, context);
}

void st_machine_execute_method(VirtualMachine *machine) {
	uint prim_index;
	MethodFlags flags = st_method_get_flags(machine->new_method);
	if (flags == ST_METHOD_PRIMITIVE) {
		prim_index = st_method_get_prim_index(machine->new_method);
		machine->success = true;
		st_primitives[prim_index].func(machine);
		if (ST_LIKELY (machine->success))
			return;
	}

	activate_method(machine);
}

void st_machine_set_active_context(VirtualMachine *machine, Oop context) {
	Oop home;
	/* save executation state of active context */
	if (ST_UNLIKELY(machine->context != ST_NIL)) {
		ContextPart_IP (machine->context) = st_smi_new(machine->ip);
		ContextPart_SP (machine->context) = st_smi_new(machine->sp);
	}

	if (ST_OBJECT_CLASS(context) == ST_BLOCK_CONTEXT_CLASS) {
		home = BlockContext_HOME (context);
		machine->method = MethodContext_METHOD (home);
		machine->receiver = MethodContext_RECEIVER (home);
		machine->temps = MethodContext_STACK (home);
		machine->stack = BlockContext_STACK (context);
	}
	else {
		machine->method = MethodContext_METHOD (context);
		machine->receiver = MethodContext_RECEIVER (context);
		machine->temps = MethodContext_STACK (context);
		machine->stack = MethodContext_STACK (context);
	}

	machine->context = context;
	machine->sp = st_smi_value(ContextPart_SP (context));
	machine->ip = st_smi_value(ContextPart_IP (context));
	machine->bytecode = (uchar *) st_method_bytecode_bytes(machine->method);
}

#define SEND_SELECTOR(selector, argcount)            \
    machine->message_argcount = argcount;            \
    machine->message_receiver = sp[- argcount - 1];  \
    machine->message_selector = selector;            \
    goto send_common;


static inline void install_method_in_cache(VirtualMachine *machine) {
	uint index = ST_METHOD_CACHE_HASH (machine->lookup_class, machine->message_selector) & ST_METHOD_CACHE_MASK;
	machine->method_cache[index].class = machine->lookup_class;
	machine->method_cache[index].selector = machine->message_selector;
	machine->method_cache[index].method = machine->new_method;
}

static inline bool lookup_method_in_cache(VirtualMachine *machine) {
	uint index = ST_METHOD_CACHE_HASH (machine->lookup_class, machine->message_selector) & ST_METHOD_CACHE_MASK;
	if (machine->method_cache[index].class == machine->lookup_class &&
		machine->method_cache[index].selector == machine->message_selector) {
		machine->new_method = machine->method_cache[index].method;
		return true;
	}
	return false;
}

#define STACK_POP(oop)     (*--sp)
#define STACK_PUSH(oop)    (*sp++ = (oop))
#define STACK_PEEK(oop)    (*(sp-1))
#define STACK_UNPOP(count) (sp += count)

#define STORE_REGISTERS()                                             \
    machine->ip = ip - machine->bytecode;                             \
    machine->sp = sp - machine->stack;                                \
    ContextPart_IP (machine->context) = st_smi_new (machine->ip); \
    ContextPart_SP (machine->context) = st_smi_new (machine->sp)

#define LOAD_REGISTERS()                    \
    ip = machine->bytecode + machine->ip;   \
    sp = machine->stack + machine->sp

void st_machine_main(VirtualMachine *machine) {
	register const uchar *ip;
	register Oop *sp = machine->stack;

	if (setjmp (machine->main_loop))
		goto out;

	ip = machine->bytecode + machine->ip;

	start:
	switch (*ip) {
		case PUSH_TEMP: {
			STACK_PUSH (machine->temps[ip[1]]);
			ip += 2;
			goto start;
		}
		case PUSH_INSTVAR: {
			STACK_PUSH (ST_OBJECT_FIELDS(machine->receiver)[ip[1]]);
			ip += 2;
			goto start;
		}
		case STORE_POP_INSTVAR: {
			ST_OBJECT_FIELDS(machine->receiver)[ip[1]] = STACK_POP ();
			ip += 2;
			goto start;
		}
		case STORE_INSTVAR: {
			ST_OBJECT_FIELDS(machine->receiver)[ip[1]] = STACK_PEEK ();
			ip += 2;
			goto start;
		}
		case STORE_POP_TEMP: {
			machine->temps[ip[1]] = STACK_POP ();
			ip += 2;
			goto start;
		}
		case STORE_TEMP: {
			machine->temps[ip[1]] = STACK_PEEK ();
			ip += 2;
			goto start;
		}
		case STORE_LITERAL_VAR: {
			ST_ASSOCIATION_VALUE (st_array_elements(ST_METHOD_LITERALS(machine->method))[ip[1]]) = STACK_PEEK ();
			ip += 2;
			goto start;
		}
		case STORE_POP_LITERAL_VAR: {
			ST_ASSOCIATION_VALUE (st_array_elements(ST_METHOD_LITERALS(machine->method))[ip[1]]) = STACK_POP ();
			ip += 2;
			goto start;
		}
		case PUSH_SELF: {
			STACK_PUSH (machine->receiver);
			ip += 1;
			goto start;
		}
		case PUSH_TRUE: {
			STACK_PUSH (ST_TRUE);
			ip += 1;
			goto start;
		}
		case PUSH_FALSE: {
			STACK_PUSH (ST_FALSE);
			ip += 1;
			goto start;
		}
		case PUSH_NIL: {
			STACK_PUSH (ST_NIL);
			ip += 1;
			goto start;
		}
		case PUSH_INTEGER: {
			STACK_PUSH (st_smi_new((signed char) ip[1]));
			ip += 2;
			goto start;
		}
		case PUSH_ACTIVE_CONTEXT: {
			STACK_PUSH (machine->context);
			ip += 1;
			goto start;
		}
		case PUSH_LITERAL_CONST: {
			STACK_PUSH (st_array_elements(ST_METHOD_LITERALS(machine->method))[ip[1]]);
			ip += 2;
			goto start;
		}

		case PUSH_LITERAL_VAR: {
			Oop var;
			var = ST_ASSOCIATION_VALUE (st_array_elements(ST_METHOD_LITERALS(machine->method))[ip[1]]);
			STACK_PUSH (var);
			ip += 2;
			goto start;
		}

		case JUMP_TRUE: {
			if (STACK_PEEK () == ST_TRUE) {
				(void) STACK_POP ();
				ip += *((unsigned short *) (ip + 1)) + 3;
			}
			else if (ST_LIKELY (STACK_PEEK() == ST_FALSE)) {
				(void) STACK_POP ();
				ip += 3;
			}
			else {
				ip += 3;
				SEND_SELECTOR (ST_SELECTOR_MUSTBEBOOLEAN, 0);
			}
			goto start;
		}

		case JUMP_FALSE: {
			if (STACK_PEEK () == ST_FALSE) {
				(void) STACK_POP ();
				ip += *((unsigned short *) (ip + 1)) + 3;
			}
			else if (ST_LIKELY (STACK_PEEK() == ST_TRUE)) {
				(void) STACK_POP ();
				ip += 3;
			}
			else {
				ip += 3;
				SEND_SELECTOR (ST_SELECTOR_MUSTBEBOOLEAN, 0);
			}
			goto start;
		}

		case JUMP: {
			ip += *((short *) (ip + 1)) + 3;
			goto start;
		}
		case SEND_PLUS: {
			int a, b, result;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = st_smi_value(sp[-1]);
				a = st_smi_value(sp[-2]);
				result = a + b;
				if (((result << 1) ^ (result << 2)) >= 0) {
					sp -= 2;
					STACK_PUSH (st_smi_new(result));
					ip++;
					goto start;
				}
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_PLUS;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_MINUS: {
			int a, b, result;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = st_smi_value(sp[-1]);
				a = st_smi_value(sp[-2]);
				result = a - b;
				if (((result << 1) ^ (result << 2)) >= 0) {
					sp -= 2;
					STACK_PUSH (st_smi_new(result));
					ip++;
					goto start;
				}
				else
					STACK_UNPOP (2);
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_MINUS;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_MUL: {
			int a, b;
			int64_t result;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = st_smi_value(sp[-1]);
				a = st_smi_value(sp[-2]);
				result = a * b;
				if (result >= INT_MIN && result <= INT_MAX) {
					sp -= 2;
					STACK_PUSH (st_smi_new((int) result));
					ip++;
					goto start;
				}
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_MUL;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_MOD: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_new(st_smi_value(a) % st_smi_value(b)));
				ip++;
				goto start;
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_MOD;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_DIV: {
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_DIV;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_BITSHIFT: {
			Oop a, b;

			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				if (st_smi_value(b) < 0)
					STACK_PUSH (st_smi_new(st_smi_value(a) >> -st_smi_value(b)));
				else
					STACK_PUSH (st_smi_new(st_smi_value(a) << st_smi_value(b)));
				ip++;
				goto start;
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_BITSHIFT;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_BITAND: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_new(st_smi_value(a) & st_smi_value(b)));
				ip++;
				goto start;
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_BITAND;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_BITOR: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_new(st_smi_value(a) | st_smi_value(b)));
				ip++;
				goto start;
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_BITOR;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_BITXOR: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_new(st_smi_value(a) ^ st_smi_value(b)));
				ip++;
				goto start;
			}
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_BITXOR;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_LT: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_value(a) < st_smi_value(b) ? ST_TRUE : ST_FALSE);
				ip++;
				goto start;
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_LT;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_GT: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_value(a) > st_smi_value(b) ? ST_TRUE : ST_FALSE);
				ip++;
				goto start;
			}
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_GT;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_LE: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_value(a) <= st_smi_value(b) ? ST_TRUE : ST_FALSE);
				ip++;
				goto start;
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_LE;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_GE: {
			Oop a, b;
			if (ST_LIKELY (st_object_is_smi(sp[-1]) && st_object_is_smi(sp[-2]))) {
				b = STACK_POP ();
				a = STACK_POP ();
				STACK_PUSH (st_smi_value(a) >= st_smi_value(b) ? ST_TRUE : ST_FALSE);
				ip++;
				goto start;
			}

			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_GE;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_CLASS: {
			machine->message_argcount = 0;
			machine->message_selector = ST_SELECTOR_CLASS;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_SIZE: {
			machine->message_argcount = 0;
			machine->message_selector = ST_SELECTOR_SIZE;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_AT: {
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_AT;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_AT_PUT: {
			machine->message_argcount = 2;
			machine->message_selector = ST_SELECTOR_ATPUT;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_EQ: {
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_EQ;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_NE: {
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_NE;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_IDENTITY_EQ: {
			Oop a, b;
			a = STACK_POP ();
			b = STACK_POP ();
			STACK_PUSH ((a == b) ? ST_TRUE : ST_FALSE);
			ip += 1;
			goto start;
		}
		case SEND_VALUE: {
			machine->message_argcount = 0;
			machine->message_selector = ST_SELECTOR_VALUE;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_VALUE_ARG: {
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_VALUE_ARG;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_NEW: {
			machine->message_argcount = 0;
			machine->message_selector = ST_SELECTOR_NEW;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND_NEW_ARG: {
			machine->message_argcount = 1;
			machine->message_selector = ST_SELECTOR_NEW_ARG;
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 1;
			goto send_common;
		}
		case SEND: {
			uint prim_index;
			MethodFlags flags;
			Oop context;
			Oop *arguments;

			machine->message_argcount = ip[1];
			machine->message_selector = st_array_elements(ST_METHOD_LITERALS (machine->method))[ip[2]];
			machine->message_receiver = sp[-machine->message_argcount - 1];
			machine->lookup_class = st_object_class(machine->message_receiver);
			ip += 3;

			send_common:

			if (!lookup_method_in_cache(machine)) {
				STORE_REGISTERS();
				machine->new_method = lookup_method(machine, machine->lookup_class);
				LOAD_REGISTERS();
				install_method_in_cache(machine);
			}

			flags = st_method_get_flags(machine->new_method);
			if (flags == ST_METHOD_PRIMITIVE) {
				prim_index = st_method_get_prim_index(machine->new_method);

				machine->success = true;
				STORE_REGISTERS();
				st_primitives[prim_index].func(machine);
				LOAD_REGISTERS();

				if (ST_LIKELY (machine->success))
					goto start;
			}

			STORE_REGISTERS(); // store registers as a gc could occur
			context = method_context_new(machine);
			LOAD_REGISTERS();
			arguments = MethodContext_STACK (context);
			for (int i = 0; i < machine->message_argcount; i++)
				arguments[i] = sp[-machine->message_argcount + i];
			sp -= machine->message_argcount + 1;

			STORE_REGISTERS();
			st_machine_set_active_context(machine, context);
			LOAD_REGISTERS();

			/* We have to nil these fields here. Its possible that
			   that the objects they reference may be zapped by the gc.
			   Another GC invocation may try to remap these fields not knowing that
			   the references are invalid.
			   FIXME: move this nilling out of such a critical execution path
		   */

			machine->message_receiver = ST_NIL;
			machine->message_selector = ST_NIL;
			goto start;
		}
		case SEND_SUPER: {
			Oop index;
			machine->message_argcount = ip[1];
			machine->message_selector = st_array_elements(ST_METHOD_LITERALS (machine->method))[ip[2]];
			machine->message_receiver = sp[-machine->message_argcount - 1];

			index = st_smi_value(st_arrayed_object_size(ST_METHOD_LITERALS (machine->method))) - 1;
			machine->lookup_class = ST_BEHAVIOR_SUPERCLASS (
				st_array_elements(ST_METHOD_LITERALS(machine->method))[index]);

			ip += 3;
			goto send_common;
		}
		case POP_STACK_TOP: {
			(void) STACK_POP ();
			ip += 1;
			goto start;
		}
		case DUPLICATE_STACK_TOP: {
			STACK_PUSH(STACK_PEEK());
			ip += 1;
			goto start;
		}
		case BLOCK_COPY: {
			Oop block;
			Oop home;
			uint argcount = ip[1];
			uint initial_ip;

			ip += 2;

			initial_ip = ip - machine->bytecode + 3;

			STORE_REGISTERS ();
			block = block_context_new(machine, initial_ip, argcount);
			LOAD_REGISTERS ();

			STACK_PUSH (block);

			goto start;
		}
		case RETURN_STACK_TOP: {
			Oop sender;
			Oop value;
			Oop home;

			value = STACK_PEEK ();

			if (ST_OBJECT_CLASS(machine->context) == ST_BLOCK_CONTEXT_CLASS)
				sender = ContextPart_SENDER (BlockContext_HOME(machine->context));
			else {
				sender = ContextPart_SENDER (machine->context);
				st_memory_recycle_context(machine->context);
			}

			if (ST_UNLIKELY(sender == ST_NIL)) {
				STACK_PUSH (machine->context);
				STACK_PUSH (value);
				SEND_SELECTOR (ST_SELECTOR_CANNOTRETURN, 1);
				goto start;
			}

			st_machine_set_active_context(machine, sender);
			LOAD_REGISTERS ();
			STACK_PUSH (value);

			goto start;
		}
		case BLOCK_RETURN: {
			Oop caller;
			Oop value;
			Oop home;

			caller = ContextPart_SENDER (machine->context);
			value = STACK_PEEK ();

			st_machine_set_active_context(machine, caller);
			LOAD_REGISTERS ();
			STACK_PUSH (value);

			goto start;
		}
		default:
			abort();
	}
	out:
	st_log("gc", "totalPauseTime: %.6fs\n", st_timespec_to_double_seconds(&memory->total_pause_time));
}

void st_machine_clear_caches(VirtualMachine *machine) {
	memset(machine->method_cache, 0, ST_METHOD_CACHE_SIZE * 3 * sizeof(Oop));
}

void st_machine_initialize(VirtualMachine *machine) {
	Oop context;
	Oop method;

	/* clear contents */
	machine->context = ST_NIL;
	machine->receiver = ST_NIL;
	machine->method = ST_NIL;

	machine->sp = 0;
	machine->ip = 0;
	machine->stack = NULL;

	st_machine_clear_caches(machine);

	machine->message_argcount = 0;
	machine->message_receiver = ST_SMALLTALK;
	machine->message_selector = ST_SELECTOR_STARTUPSYSTEM;

	machine->new_method = lookup_method(machine, st_object_class(machine->message_receiver));
	st_assert (st_method_get_flags(machine->new_method) == ST_METHOD_NORMAL);

	context = method_context_new(machine);
	st_machine_set_active_context(machine, context);
}

