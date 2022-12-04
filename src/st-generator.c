
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <setjmp.h>

#include "st-types.h"
#include "st-generator.h"
#include "st-compiler.h"
#include "st-object.h"
#include "st-symbol.h"
#include "st-dictionary.h"
#include "st-method.h"
#include "st-array.h"
#include "st-universe.h"
#include "st-behavior.h"
#include "st-character.h"
#include "st-unicode.h"

// setup global data for compiler
void check_init(void) {
	static bool initialized = false;
	
	if (initialized)
		return;
	initialized = true;
	
	/* The size (in bytes) of each bytecode instruction */
	sizes[PUSH_TEMP] = 2;
	sizes[PUSH_INSTVAR] = 2;
	sizes[PUSH_LITERAL_CONST] = 2;
	sizes[PUSH_LITERAL_VAR] = 2;
	sizes[PUSH_SELF] = 1;
	sizes[PUSH_NIL] = 1;
	sizes[PUSH_TRUE] = 1;
	sizes[PUSH_FALSE] = 1;
	sizes[PUSH_INTEGER] = 2;
	sizes[STORE_LITERAL_VAR] = 2;
	sizes[STORE_TEMP] = 2;
	sizes[STORE_INSTVAR] = 2;
	sizes[STORE_POP_LITERAL_VAR] = 2;
	sizes[STORE_POP_TEMP] = 2;
	sizes[STORE_POP_INSTVAR] = 2;
	sizes[RETURN_STACK_TOP] = 1;
	sizes[BLOCK_RETURN] = 1;
	sizes[POP_STACK_TOP] = 1;
	sizes[DUPLICATE_STACK_TOP] = 1;
	sizes[PUSH_ACTIVE_CONTEXT] = 1;
	sizes[BLOCK_COPY] = 2;
	sizes[JUMP_TRUE] = 3;
	sizes[JUMP_FALSE] = 3;
	sizes[JUMP] = 3;
	sizes[SEND] = 3;
	sizes[SEND_SUPER] = 3;
	sizes[SEND_PLUS] = 1;
	sizes[SEND_MINUS] = 1;
	sizes[SEND_LT] = 1;
	sizes[SEND_GT] = 1;
	sizes[SEND_LE] = 1;
	sizes[SEND_GE] = 1;
	sizes[SEND_EQ] = 1;
	sizes[SEND_NE] = 1;
	sizes[SEND_MUL] = 1;
	sizes[SEND_DIV] = 1;
	sizes[SEND_MOD] = 1;
	sizes[SEND_BITSHIFT] = 1;
	sizes[SEND_BITAND] = 1;
	sizes[SEND_BITOR] = 1;
	sizes[SEND_BITXOR] = 1;
	sizes[SEND_AT] = 1;
	sizes[SEND_AT_PUT] = 1;
	sizes[SEND_SIZE] = 1;
	sizes[SEND_VALUE] = 1;
	sizes[SEND_VALUE_ARG] = 1;
	sizes[SEND_IDENTITY_EQ] = 1;
	sizes[SEND_CLASS] = 1;
	sizes[SEND_NEW] = 1;
	sizes[SEND_NEW_ARG] = 1;
}

void generation_error(st_generator *gt, const char *message, st_node *node) {
	if (gt->error) {
		strncpy(gt->error->message, message, 255);
		gt->error->line = node->line;
		gt->error->column = 0;
	}
	
	longjmp(gt->jmploc, 0);
	abort();
}

void bytecode_init(st_bytecode *code) {
	code->buffer = st_malloc(DEFAULT_CODE_SIZE);
	code->alloc = DEFAULT_CODE_SIZE;
	code->size = 0;
	code->max_stack_depth = 0;
}

void bytecode_destroy(st_bytecode *code) {
	st_free(code->buffer);
}

st_list *get_temporaries(st_generator *gt, st_list *instvars, st_node *arguments, st_node *temporaries) {
	st_list *temps = NULL;
	
	for (st_node *n = arguments; n; n = n->next) {
		for (st_list *l = instvars; l; l = l->next)
			if (streq (n->variable.name, (char *) l->data))
				generation_error(gt, "name is already defined", arguments);
		temps = st_list_prepend(temps, (st_pointer) n->variable.name);
	}
	
	for (st_node *n = temporaries; n; n = n->next) {
		for (st_list *l = instvars; l; l = l->next)
			if (streq (n->variable.name, (char *) l->data))
				generation_error(gt, "name is already defined", temporaries);
		temps = st_list_prepend(temps, (st_pointer) n->variable.name);
	}
	
	return st_list_reverse(temps);
}

st_generator *generator_new(void) {
	st_generator *gt = st_new0 (st_generator);
	gt->class = 0;
	gt->instvars = NULL;
	gt->literals = NULL;
	gt->temporaries = NULL;
	return gt;
}

void generator_destroy(st_generator *gt) {
	st_list_foreach(gt->instvars, st_free);
	st_list_destroy(gt->instvars);
	st_list_destroy(gt->temporaries);
	st_list_destroy(gt->literals);
	st_free(gt);
}

st_oop create_literals_array(st_generator *gt) {
	st_oop literals;
	st_uint i;
	
	gt->literals = st_list_append(gt->literals, (st_pointer) gt->class);
	literals = st_object_new_arrayed(ST_ARRAY_CLASS, st_list_length(gt->literals));
	
	i = 1;
	for (st_list *l = gt->literals; l; l = l->next) {
		st_array_at_put(literals, i, (st_oop) l->data);
		i++;
	}
	
	return literals;
}

st_oop create_bytecode_array(st_bytecode *code) {
	if (code->size == 0) return ST_NIL;
	st_oop array = st_object_new_arrayed(ST_BYTE_ARRAY_CLASS, code->size);
	memcpy(st_byte_array_bytes(array), code->buffer, code->size);
	return array;
}

void emit(st_bytecode *code, st_uchar value) {
	if (++code->size > code->alloc) {
		code->alloc += code->alloc;
		code->buffer = st_realloc(code->buffer, code->alloc);
	}
	
	code->buffer[code->size - 1] = value;
}

int find_instvar(st_generator *gt, char *name) {
	int i = 0;
	for (st_list *l = gt->instvars; l; l = l->next) {
		if (streq (name, (char *) l->data))
			return i;
		i++;
	}
	return -1;
}

int find_temporary(st_generator *gt, char *name) {
	int i = 0;
	
	for (st_list *l = gt->temporaries; l; l = l->next) {
		if (streq (name, (char *) l->data))
			return i;
		i++;
	}
	return -1;
}

int find_literal_const(st_generator *gt, st_oop literal) {
	int i = 0;
	for (st_list *l = gt->literals; l; l = l->next) {
		if (st_object_equal(literal, (st_oop) l->data))
			return i;
		i++;
	}
	gt->literals = st_list_append(gt->literals, (void *) literal);
	return i;
}

int find_literal_var(st_generator *gt, char *name) {
	
	st_oop assoc = st_dictionary_association_at(ST_GLOBALS, st_symbol_new(name));
	if (assoc == ST_NIL)
		return -1;
	
	int i = 0;
	for (st_list *l = gt->literals; l; l = l->next) {
		if (st_object_equal(assoc, (st_oop) l->data))
			return i;
		i++;
	}
	gt->literals = st_list_append(gt->literals, (st_pointer) assoc);
	return i;
}

void jump_offset(st_bytecode *code, int offset) {
	st_assert (offset <= INT16_MAX);
	emit(code, JUMP);
	emit(code, offset & 0xFF);          // push low byte
	emit(code, (offset >> 8) & 0xFF);   // push high byte
}

void assign_temp(st_bytecode *code, int index, bool pop) {
	st_assert (index <= 255);
	if (pop) emit(code, STORE_POP_TEMP);
	else 	 emit(code, STORE_TEMP);
	emit(code, (st_uchar) index);
}

void assign_instvar(st_bytecode *code, int index, bool pop) {
	st_assert (index <= 255);
	if (pop) emit(code, STORE_POP_INSTVAR);
	else 	 emit(code, STORE_INSTVAR);
	emit(code, (st_uchar) index);
}

void assign_literal_var(st_bytecode *code, int index, bool pop) {
	st_assert (index <= 255);
	if (pop)
		emit(code, STORE_POP_LITERAL_VAR);
	else
		emit(code, STORE_LITERAL_VAR);
	emit(code, (st_uchar) index);
}

void push(st_bytecode *code, st_uchar value, st_uchar index) {
	emit(code, value);
	emit(code, index);
	code->max_stack_depth++;
}

void push_special(st_bytecode *code, st_uchar value) {
	emit(code, value);
	code->max_stack_depth++;
}

void generate_assign(st_generator *gt, st_bytecode *code, st_node *node, bool pop) {
	int index;
	
	generate_expression(gt, code, node->assign.expression);
	index = find_temporary(gt, node->assign.assignee->variable.name);
	if (index >= 0) {
		assign_temp(code, index, pop);
		return;
	}
	
	index = find_instvar(gt, node->assign.assignee->variable.name);
	if (index >= 0) {
		assign_instvar(code, index, pop);
		return;
	}
	
	index = find_literal_var(gt, node->assign.assignee->variable.name);
	if (index >= 0) {
		assign_literal_var(code, index, pop);
		return;
	}
	
	generation_error(gt, "unknown variable", node);
}

int size_assign(st_generator *gt, st_node *node) {
	st_bytecode code;
	bytecode_init(&code);
	generate_assign(gt, &code, node, true);
	bytecode_destroy(&code);
	return code.size;
}

void generate_return(st_generator *gt, st_bytecode *code, st_node *node) {
	generate_expression(gt, code, node->retrn.expression);
	emit(code, RETURN_STACK_TOP);
}

int size_return(st_generator *gt, st_node *node) {
	st_bytecode code;
	bytecode_init(&code);
	generate_return(gt, &code, node);
	bytecode_destroy(&code);
	return code.size;
}

st_list *get_block_temporaries(st_generator *gt, st_node *temporaries) {
	st_list *temps = NULL;
	for (st_node *node = temporaries; node; node = node->next) {
		for (st_list *l = gt->instvars; l; l = l->next)
			if (streq (node->variable.name, (char *) l->data))
				generation_error(gt, "name is already defined", node);
		
		for (st_list *l = gt->temporaries; l; l = l->next)
			if (streq (node->variable.name, (char *) l->data))
				generation_error(gt, "name already used in method", node);
	
		temps = st_list_prepend(temps, (st_pointer) node->variable.name);
	}
	
	return st_list_reverse(temps);
}

void generate_block(st_generator *gt, st_bytecode *code, st_node *node) {
	int index, size = 0;
	st_uint i, argcount;
	st_node *l;
	
	argcount = st_node_list_length(node->block.arguments);
	
	emit(code, BLOCK_COPY);
	emit(code, argcount);
	
	// get size of block code and then jump around that code
	size = 2 * argcount + size_statements(gt, node->block.statements) + sizes[BLOCK_RETURN];
	jump_offset(code, size);
	
	/* Store all block arguments into the temporary frame.
	   Note that upon a block activation, the stack pointer sits
	   above the last argument to to the block */
	i = st_node_list_length(node->block.arguments);
	for (; i > 0; --i) {
		l = st_node_list_at(node->block.arguments, i - 1);
		index = find_temporary(gt, l->variable.name);
		st_assert (index >= 0);
		assign_temp(code, index, true);
	}
	
	generate_statements(gt, code, node->block.statements);
	emit(code, BLOCK_RETURN);
}

int size_block(st_generator *gt, st_node *node) {
	st_bytecode code;
	bytecode_init(&code);
	generate_block(gt, &code, node);
	bytecode_destroy(&code);
	return code.size;
}

// #ifTrue:
bool match_ifTrue(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "ifTrue:") != 0)
		return false;
	
	block = node->msg.arguments;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_ifTrue(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *block;
	int size;
	
	block = node->msg.arguments;
	
	generate_expression(gt, code, node->msg.receiver);
	size = size_statements(gt, block->block.statements);
	size += node->msg.is_statement ? sizes[POP_STACK_TOP] : sizes[JUMP];
	emit(code, JUMP_FALSE);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	generate_statements(gt, code, block->block.statements);
	
	if (node->msg.is_statement) {
		emit(code, POP_STACK_TOP);
	} else {
		emit(code, JUMP);
		emit(code, 1);
		emit(code, 0);
		emit(code, PUSH_NIL);
	}
}

// #ifFalse:
bool match_ifFalse(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "ifFalse:") != 0)
		return false;
	
	block = node->msg.arguments;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_ifFalse(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *block;
	int size;
	
	block = node->msg.arguments;
	
	generate_expression(gt, code, node->msg.receiver);
	size = size_statements(gt, block->block.statements);
	size += node->msg.is_statement ? sizes[POP_STACK_TOP] : sizes[JUMP];
	emit(code, JUMP_TRUE);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	generate_statements(gt, code, block->block.statements);
	
	if (node->msg.is_statement) {
		emit(code, POP_STACK_TOP);
	} else {
		emit(code, JUMP);
		emit(code, 1);
		emit(code, 0);
		emit(code, PUSH_NIL);
	}
}

// #ifTrueifFalse:
bool match_ifTrueifFalse(st_node *node) {
	st_node *true_block, *false_block;
	
	if (strcmp(CSTRING (node->msg.selector), "ifTrue:ifFalse:") != 0)
		return false;
	
	true_block = node->msg.arguments;
	if (true_block->type != ST_BLOCK_NODE || true_block->block.arguments != NULL)
		return false;
	
	false_block = node->msg.arguments->next;
	if (false_block->type != ST_BLOCK_NODE || false_block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_ifTrueifFalse(st_generator *gt, st_bytecode *code, st_node *node) {
	int size;
	
	st_node *true_block, *false_block;
	
	true_block = node->msg.arguments;
	false_block = node->msg.arguments->next;
	
	generate_expression(gt, code, node->msg.receiver);
	size = size_statements(gt, true_block->block.statements) + sizes[JUMP];
	emit(code, JUMP_FALSE);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	generate_statements(gt, code, true_block->block.statements);
	size = size_statements(gt, false_block->block.statements);
	emit(code, JUMP);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	generate_statements(gt, code, false_block->block.statements);
	
	if (node->msg.is_statement)
		emit(code, POP_STACK_TOP);
}

// #ifFalseifTrue:
bool match_ifFalseifTrue(st_node *node) {
	st_node *true_block, *false_block;
	
	if (strcmp(CSTRING (node->msg.selector), "ifFalse:ifTrue:") != 0)
		return false;
	
	true_block = node->msg.arguments;
	if (true_block->type != ST_BLOCK_NODE || true_block->block.arguments != NULL)
		return false;
	
	false_block = node->msg.arguments->next;
	if (false_block->type != ST_BLOCK_NODE || false_block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_ifFalseifTrue(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *true_block, *false_block;
	int size;
	
	true_block = node->msg.arguments;
	false_block = node->msg.arguments->next;
	
	generate_expression(gt, code, node->msg.receiver);
	size = size_statements(gt, true_block->block.statements) + sizes[JUMP];
	emit(code, JUMP_TRUE);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	generate_statements(gt, code, true_block->block.statements);
	size = size_statements(gt, false_block->block.statements);
	emit(code, JUMP);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	generate_statements(gt, code, false_block->block.statements);
	
	if (node->msg.is_statement)
		emit(code, POP_STACK_TOP);
}

// #whileTrue
bool match_whileTrue(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "whileTrue") != 0)
		return false;
	
	block = node->msg.receiver;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_whileTrue(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *block;
	int size;
	
	block = node->msg.receiver;
	
	generate_statements(gt, code, block->block.statements);
	
	emit(code, JUMP_FALSE);
	emit(code, 3);
	emit(code, 0);
	size = size_statements(gt, block->block.statements);
	size += sizes[JUMP_FALSE];
	size = -(size + 3);
	emit(code, JUMP);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	if (!node->msg.is_statement)
		emit(code, PUSH_NIL);
}

// #whileFalse
bool match_whileFalse(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "whileFalse") != 0)
		return false;
	
	block = node->msg.receiver;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_whileFalse(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *block;
	int size;
	
	block = node->msg.receiver;
	generate_statements(gt, code, block->block.statements);
	
	emit(code, JUMP_TRUE);
	emit(code, 3);
	emit(code, 0);
	size = size_statements(gt, block->block.statements);
	size += sizes[JUMP_FALSE];
	size = -(size + 3);
	emit(code, JUMP);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	if (!node->msg.is_statement)
		emit(code, PUSH_NIL);
}

// #whileTrueArg
bool match_whileTrueArg(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "whileTrue:") != 0)
		return false;
	
	block = node->msg.receiver;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	block = node->msg.arguments;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_whileTrueArg(st_generator *gt, st_bytecode *code, st_node *node) {
	
	int size;
	
	generate_statements(gt, code, node->msg.receiver->block.statements);
	emit(code, JUMP_FALSE);
	size = size_statements(gt, node->msg.arguments->block.statements);
	size += sizes[POP_STACK_TOP] + sizes[JUMP];
	
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	generate_statements(gt, code, node->msg.arguments->block.statements);
	
	size += size_statements(gt, node->msg.receiver->block.statements);
	size = -(size + 3);
	emit(code, POP_STACK_TOP);
	emit(code, JUMP);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	if (!node->msg.is_statement)
		emit(code, PUSH_NIL);
}

// #whileFalseArg
bool match_whileFalseArg(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "whileFalse:") != 0)
		return false;
	
	block = node->msg.receiver;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	block = node->msg.arguments;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_whileFalseArg(st_generator *gt, st_bytecode *code, st_node *node) {
	
	int size;
	
	generate_statements(gt, code, node->msg.receiver->block.statements);
	emit(code, JUMP_TRUE);
	size = size_statements(gt, node->msg.arguments->block.statements);
	size += sizes[POP_STACK_TOP] + sizes[JUMP];
	
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	generate_statements(gt, code, node->msg.arguments->block.statements);
	
	size += size_statements(gt, node->msg.receiver->block.statements);
	size = -(size + 3);
	
	emit(code, POP_STACK_TOP);
	emit(code, JUMP);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	if (!node->msg.is_statement)
		emit(code, PUSH_NIL);
}

// #and:
bool match_and(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "and:") != 0)
		return false;
	
	block = node->msg.arguments;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_and(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *block;
	int size;
	
	block = node->msg.arguments;
	generate_expression(gt, code, node->msg.receiver);
	size = size_statements(gt, block->block.statements) + sizes[JUMP];
	
	emit(code, JUMP_FALSE);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	generate_statements(gt, code, block->block.statements);
	
	emit(code, JUMP);
	emit(code, 1);
	emit(code, 0);
	emit(code, PUSH_FALSE);
	
	if (node->msg.is_statement)
		emit(code, POP_STACK_TOP);
}

// #or:
bool match_or(st_node *node) {
	st_node *block;
	
	if (strcmp(CSTRING (node->msg.selector), "or:") != 0)
		return false;
	
	block = node->msg.arguments;
	if (block->type != ST_BLOCK_NODE || block->block.arguments != NULL)
		return false;
	
	return true;
}

void generate_or(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *block;
	int size;
	
	block = node->msg.arguments;
	generate_expression(gt, code, node->msg.receiver);
	size = size_statements(gt, block->block.statements) + sizes[JUMP];
	
	emit(code, JUMP_TRUE);
	emit(code, size & 0xFF);
	emit(code, (size >> 8) & 0xFF);
	
	generate_statements(gt, code, block->block.statements);
	
	emit(code, JUMP);
	emit(code, 1);
	emit(code, 0);
	emit(code, PUSH_TRUE);
	if (node->msg.is_statement)
		emit(code, POP_STACK_TOP);
}

void generate_message_send(st_generator *gt, st_bytecode *code, st_node *node) {
	st_node *args;
	st_uint argcount = 0;
	int index;
	
	/* generate arguments */
	args = node->msg.arguments;
	for (; args; args = args->next) {
		generate_expression(gt, code, args);
		argcount++;
	}
	
	/* check if message is a special */
	for (size_t i = 0; i < ST_N_ELEMENTS (__machine.selectors); i++) {
		if (!node->msg.super_send && node->msg.selector == __machine.selectors[i]) {
			emit(code, SEND_PLUS + i);
			goto out;
		}
	}
	
	if (node->msg.super_send)
		emit(code, SEND_SUPER);
	else
		emit(code, SEND);
	
	index = find_literal_const(gt, node->msg.selector);
	
	emit(code, (st_uchar) argcount);
	emit(code, (st_uchar) index);
	
	out:
	if (node->msg.is_statement)
		emit(code, POP_STACK_TOP);
}

int size_message_send(st_generator *gt, st_node *node) {
	st_bytecode code;
	
	bytecode_init(&code);
	generate_message_send(gt, &code, node);
	bytecode_destroy(&code);
	
	return code.size;
}

void generate_cascade(st_generator *gt, st_bytecode *code, st_node *node) {
	
	
	generate_expression(gt, code, node->cascade.receiver);
	emit(code, DUPLICATE_STACK_TOP);
	
	for (st_list *l = node->cascade.messages; l; l = l->next) {
		
		generate_message_send(gt, code, (st_node *) l->data);
		
		if (l->next || node->cascade.is_statement)
			emit(code, POP_STACK_TOP);
		
		if (l->next && l->next->next)
			emit(code, DUPLICATE_STACK_TOP);
	}
}

int size_cascade(st_generator *gt, st_node *node) {
	st_bytecode code;
	
	bytecode_init(&code);
	generate_cascade(gt, &code, node);
	bytecode_destroy(&code);
	
	return code.size;
}

void generate_message(st_generator *gt, st_bytecode *code, st_node *node) {
	st_uint i;
	
	for (i = 0; i < ST_N_ELEMENTS (optimisers); i++) {
		if (optimisers[i].match_func(node)) {
			optimisers[i].generation_func(gt, code, node);
			return;
		}
	}
	
	generate_expression(gt, code, node->msg.receiver);
	generate_message_send(gt, code, node);
}

int size_message(st_generator *gt, st_node *node) {
	st_bytecode code;
	
	bytecode_init(&code);
	generate_message(gt, &code, node);
	bytecode_destroy(&code);
	
	return code.size;
}

void generate_expression(st_generator *gt, st_bytecode *code, st_node *node) {
	int index;
	
	switch (node->type) {
		case ST_VARIABLE_NODE: {
			const char *name = node->variable.name;
			
			if (streq (name, "self")) {
				push_special(code, PUSH_SELF);
				break;
			} else if (streq (name, "super")) {
				push_special(code, PUSH_SELF);
				break;
			} else if (streq (name, "true")) {
				push_special(code, PUSH_TRUE);
				break;
			} else if (streq (name, "false")) {
				push_special(code, PUSH_FALSE);
				break;
			} else if (streq (name, "nil")) {
				push_special(code, PUSH_NIL);
				break;
			} else if (streq (name, "thisContext")) {
				push_special(code, PUSH_ACTIVE_CONTEXT);
				break;
			}
			
			index = find_temporary(gt, node->variable.name);
			if (index >= 0) {
				push(code, PUSH_TEMP, index);
				break;
			}
			index = find_instvar(gt, node->variable.name);
			if (index >= 0) {
				push(code, PUSH_INSTVAR, index);
				break;
			}
			index = find_literal_var(gt, node->variable.name);
			if (index >= 0) {
				push(code, PUSH_LITERAL_VAR, index);
				break;
			}
			generation_error(gt, "unknown variable", node);
			break;
			
		}
		case ST_LITERAL_NODE:
			
			/* use optimized PUSH_INTEGER for smis in the range of -127..127 */
			if (st_object_is_smi(node->literal.value) &&
			    ((st_smi_value(node->literal.value) >= -127) &&
			     (st_smi_value(node->literal.value) <= 127))) {
				emit(code, PUSH_INTEGER);
				emit(code, st_smi_value(node->literal.value));
			} else {
				index = find_literal_const(gt, node->literal.value);
				push(code, PUSH_LITERAL_CONST, index);
			}
			break;
		
		case ST_ASSIGN_NODE:
			generate_assign(gt, code, node, false);
			break;
		
		case ST_BLOCK_NODE:
			generate_block(gt, code, node);
			break;
		
		case ST_MESSAGE_NODE:
			generate_message(gt, code, node);
			break;
		
		case ST_CASCADE_NODE:
			generate_cascade(gt, code, node);
			break;
		
		default:
			st_assert_not_reached ();
	}
}

int size_expression(st_generator *gt, st_node *node) {
	st_bytecode code;
	
	bytecode_init(&code);
	generate_expression(gt, &code, node);
	bytecode_destroy(&code);
	
	return code.size;
}

void generate_statements(st_generator *gt, st_bytecode *code, st_node *statements) {
	if (statements == NULL) {
		emit(code, PUSH_NIL);
	}
	
	for (st_node *node = statements; node; node = node->next) {
		
		switch (node->type) {
			
			case ST_VARIABLE_NODE:
			case ST_LITERAL_NODE:
			case ST_BLOCK_NODE:
				/*
				 * don't generate anything in this case since we would end up with a constant
				 * expression with no side-effects.
					 *
					 * However, in a block with no explicit return, the value of the last statement is the implicit
					 * value of the block.
				 */
				if (node->next == NULL)
					generate_expression(gt, code, node);
				break;
			
			case ST_ASSIGN_NODE:
				
				/* don't use STORE_POP if this is the last statement */
				if (node->next == NULL)
					generate_assign(gt, code, node, false);
				else
					generate_assign(gt, code, node, true);
				break;
			
			case ST_RETURN_NODE:
				
				st_assert (node->next == NULL);
				generate_return(gt, code, node);
				return;
			
			case ST_MESSAGE_NODE:
				
				generate_message(gt, code, node);
				break;
			
			case ST_CASCADE_NODE:
				
				generate_cascade(gt, code, node);
				break;
			
			default:
				st_assert_not_reached ();
		}
	}
}

int size_statements(st_generator *gt, st_node *statements) {
	st_bytecode code;
	
	bytecode_init(&code);
	generate_statements(gt, &code, statements);
	bytecode_destroy(&code);
	
	return code.size;
}

void generate_method_statements(st_generator *gt, st_bytecode *code, st_node *statements) {
	for (st_node *node = statements; node; node = node->next) {
		
		switch (node->type) {
			
			case ST_VARIABLE_NODE:
			case ST_LITERAL_NODE:
			case ST_BLOCK_NODE:
				break;
			
			case ST_ASSIGN_NODE:
				
				generate_assign(gt, code, node, true);
				break;
			
			case ST_RETURN_NODE:
				
				st_assert (node->next == NULL);
				generate_return(gt, code, node);
				return;
			
			case ST_MESSAGE_NODE:
				
				generate_message(gt, code, node);
				break;
			
			case ST_CASCADE_NODE:
				
				generate_cascade(gt, code, node);
				break;
			
			default:
				st_assert_not_reached ();
		}
	}
	
	emit(code, PUSH_SELF);
	emit(code, RETURN_STACK_TOP);
}

st_list *collect_temporaries(st_generator *gt, st_node *node) {
	st_list *temps = NULL;
	
	if (node == NULL)
		return NULL;
	
	if (node->type == ST_BLOCK_NODE) {
		temps = st_list_concat(get_block_temporaries(gt, node->block.arguments),
				get_block_temporaries(gt, node->block.temporaries));
	}
	
	switch (node->type) {
		case ST_BLOCK_NODE:
			temps = st_list_concat(temps, collect_temporaries(gt, node->block.statements));
			break;
		case ST_ASSIGN_NODE:
			temps = st_list_concat(temps, collect_temporaries(gt, node->assign.expression));
			break;
		case ST_RETURN_NODE:
			temps = st_list_concat(temps, collect_temporaries(gt, node->retrn.expression));
			break;
		case ST_MESSAGE_NODE:
			temps = st_list_concat(temps, collect_temporaries(gt, node->msg.receiver));
			temps = st_list_concat(temps, collect_temporaries(gt, node->msg.arguments));
			break;
			break;
		case ST_CASCADE_NODE:
			temps = st_list_concat(temps, collect_temporaries(gt, node->cascade.receiver));
			for (st_list *l = node->cascade.messages; l; l = l->next)
				temps = st_list_concat(temps, collect_temporaries(gt, (st_node *) l->data));
			
			break;
		
		case ST_METHOD_NODE:
		case ST_VARIABLE_NODE:
		case ST_LITERAL_NODE:
			break;
	}
	
	temps = st_list_concat(temps, collect_temporaries(gt, node->next));
	
	return temps;
}

st_oop st_generate_method(st_oop class, st_node *node, st_compiler_error *error) {
	st_generator *gt;
	st_oop method;
	st_uint argcount;
	st_uint tempcount;
	st_bytecode code;
	
	st_assert (class != ST_NIL);
	st_assert (node != NULL && node->type == ST_METHOD_NODE);
	
	check_init();
	
	gt = generator_new();
	gt->error = error;
	
	if (setjmp (gt->jmploc)) {
		generator_destroy(gt);
		return ST_NIL;
	}
	
	gt->class = class;
	gt->instvars = st_behavior_all_instance_variables(class);
	gt->temporaries = get_temporaries(gt, gt->instvars, node->method.arguments, node->method.temporaries);
	
	/* collect all block-level temporaries */
	gt->temporaries = st_list_concat(gt->temporaries, collect_temporaries(gt, node->method.statements));
	
	bytecode_init(&code);
	generate_method_statements(gt, &code, node->method.statements);
	method = st_object_new(ST_COMPILED_METHOD_CLASS);
	
	argcount = st_node_list_length(node->method.arguments);
	tempcount = st_list_length(gt->temporaries) - st_node_list_length(node->method.arguments);
	
	ST_METHOD_HEADER (method) = st_smi_new(0);
	st_method_set_arg_count(method, argcount);
	st_method_set_temp_count(method, tempcount);
	st_method_set_prim_index(method, node->method.primitive);
	
	if (node->method.primitive >= 0) {
		st_method_set_flags(method, ST_METHOD_PRIMITIVE);
	} else {
		st_method_set_flags(method, ST_METHOD_NORMAL);
	}
	
	ST_METHOD_LITERALS (method) = create_literals_array(gt);
	ST_METHOD_BYTECODE (method) = create_bytecode_array(&code);
	ST_METHOD_SELECTOR (method) = node->method.selector;
	
	generator_destroy(gt);
	bytecode_destroy(&code);
	
	return method;
}

void print_bytecodes(st_oop literals, char* codes, int len) {
	char *ip;
	
	static const char *const formats[] = {
			"<%02x>       ",
			"<%02x %02x>    ",
			"<%02x %02x %02x> ",
	};
	
	ip = codes;
	while ((ip - codes) < len) {
		
		printf("%3li ", ip - codes);
		switch ((Code) *ip) {
			case PUSH_TEMP:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("pushTemp: %i", ip[1]);
				NEXT (ip);
			case PUSH_INSTVAR:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("pushInstvar: %i", ip[1]);
				NEXT (ip);
			case PUSH_LITERAL_CONST:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("pushConst: %i", ip[1]);
				NEXT (ip);
			case PUSH_LITERAL_VAR:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("pushLit: %i", ip[1]);
				NEXT (ip);
			case PUSH_SELF:
				printf(FORMAT (ip), ip[0]);
				printf("push: self");
				NEXT (ip);
			case PUSH_TRUE:
				printf(FORMAT (ip), ip[0]);
				printf("pushConst: true");
				NEXT (ip);
			case PUSH_FALSE:
				printf(FORMAT (ip), ip[0]);
				printf("pushConst: false");
				NEXT (ip);
			case PUSH_NIL:
				printf(FORMAT (ip), ip[0]);
				printf("pushConst: nil");
				NEXT (ip);
			case PUSH_INTEGER:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("pushConst: %i", (signed char) ip[1]);
				NEXT (ip);
			case PUSH_ACTIVE_CONTEXT:
				printf(FORMAT (ip), ip[0]);
				printf("push: thisContext");
				NEXT (ip);
			case STORE_TEMP:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("storeTemp: %i", ip[1]);
				NEXT (ip);
			case STORE_INSTVAR:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("storeInstvar: %i", ip[1]);
				NEXT (ip);
			case STORE_LITERAL_VAR:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("storeLiteral: %i", ip[1]);
				NEXT (ip);
			case STORE_POP_TEMP:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("popIntoTemp: %i", ip[1]);
				NEXT (ip);
			case STORE_POP_INSTVAR:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("popIntoInstvar: %i", ip[1]);
				NEXT (ip);
			case STORE_POP_LITERAL_VAR:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("popIntoLiteral: %i", ip[1]);
				NEXT (ip);
			case BLOCK_COPY:
				printf(FORMAT (ip), ip[0], ip[1]);
				printf("blockCopy: %i", ip[1]);
				NEXT (ip);
			case JUMP:
				printf(FORMAT (ip), ip[0], ip[1], ip[2]);
				short offset = *((short *) (ip + 1));
				printf("jump: %li", 3 + (ip - codes) + offset);
				NEXT (ip);
			case JUMP_TRUE:
				printf(FORMAT (ip), ip[0], ip[1], ip[2]);
				printf("jumpTrue: %li", 3 + (ip - codes) + *((short *) (ip + 1)));
				NEXT (ip);
			case JUMP_FALSE:
				printf(FORMAT (ip), ip[0], ip[1], ip[2]);
				printf("jumpFalse: %li", 3 + (ip - codes) + *((short *) (ip + 1)));
				NEXT (ip);
			case POP_STACK_TOP:
				printf(FORMAT (ip), ip[0]);
				printf("pop");
				NEXT (ip);
			case DUPLICATE_STACK_TOP:
				printf(FORMAT (ip), ip[0]);
				printf("dup");
				NEXT (ip);
			case RETURN_STACK_TOP:
				printf(FORMAT (ip), ip[0]);
				printf("returnTop");
				ip += 1;
				break;
				NEXT (ip);
			case BLOCK_RETURN:
				printf(FORMAT (ip), ip[0]);
				printf("blockReturn");
				NEXT (ip);
			case SEND: {
				st_oop selector;
				
				selector = st_array_at(literals, ip[2] + 1);
				
				printf(FORMAT (ip), ip[0], ip[1], ip[2]);
				
				printf("send: #%s", (char *) st_byte_array_bytes(selector));
				
				NEXT (ip);
			}
			case SEND_SUPER: {
				st_oop selector;
				
				selector = st_array_at(literals, ip[2] + 1);
				
				printf(FORMAT (ip), ip[0], ip[1], ip[2]);
				
				printf("sendSuper: #%s", (char *) st_byte_array_bytes(selector));
				
				NEXT (ip);
			}
			case SEND_PLUS:
			case SEND_MINUS:
			case SEND_LT:
			case SEND_GT:
			case SEND_LE:
			case SEND_GE:
			case SEND_EQ:
			case SEND_NE:
			case SEND_MUL:
			case SEND_DIV:
			case SEND_MOD:
			case SEND_BITSHIFT:
			case SEND_BITAND:
			case SEND_BITOR:
			case SEND_BITXOR:
			case SEND_AT:
			case SEND_AT_PUT:
			case SEND_SIZE:
			case SEND_VALUE:
			case SEND_VALUE_ARG:
			case SEND_IDENTITY_EQ:
			case SEND_CLASS:
			case SEND_NEW:
			case SEND_NEW_ARG:
				
				printf(FORMAT (ip), ip[0]);
				printf("sendSpecial: #%s", st_byte_array_bytes(__machine.selectors[ip[0] - SEND_PLUS]));
				
				NEXT (ip);
			
		}
		printf("\n");
	}
	
}

void gen_print_literal(st_oop lit) {
	if (st_object_is_smi(lit)) {
		
		printf("%i", st_smi_value(lit));
		
	} else if (st_object_is_symbol(lit)) {
		
		printf("#%s", (char *) st_byte_array_bytes(lit));
		
	} else if (st_object_class(lit) == ST_STRING_CLASS) {
		
		printf("'%s'", (char *) st_byte_array_bytes(lit));
		
	} else if (st_object_class(lit) == ST_CHARACTER_CLASS) {
		
		char outbuf[6] = {0};
		st_unichar_to_utf8(st_character_value(lit), outbuf);
		printf("$%s", outbuf);
	}
}

void print_literals(st_oop literals) {
	if (literals == ST_NIL)
		return;
	
	printf("literals: ");
	
	for (int i = 1; i <= st_smi_value(ST_ARRAYED_OBJECT (literals)->size); i++) {
		
		st_oop lit = st_array_at(literals, i);
		
		gen_print_literal(lit);
		
		printf(" ");
	}
	
	printf("\n");
}

void st_print_generated_method(st_oop method) {
	st_oop literals;
	char *bytecodes;
	int size;
	
	printf("flags: %i; ", st_method_get_flags(method));
	printf("arg-count: %i; ", st_method_get_arg_count(method));
	printf("temp-count: %i; ", st_method_get_temp_count(method));
	printf("primitive: %i;\n", st_method_get_prim_index(method));
	
	printf("\n");
	
	literals = ST_METHOD_LITERALS (method);
	bytecodes = st_method_bytecode_bytes(method);
	size = st_smi_value(st_arrayed_object_size(ST_METHOD_BYTECODE (method)));
	
	print_bytecodes(literals, bytecodes, size);
	
	print_literals(literals);
}