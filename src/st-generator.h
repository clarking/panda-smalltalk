

/*
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"
#include "st-utils.h"

#define DEFAULT_CODE_SIZE 20
#define CSTRING(string) ((char *) st_byte_array_bytes (string))

#define NEXT(ip)      \
    ip += sizes[*ip]; \
    break

#define FORMAT(ip) (formats[sizes[*ip]-1])

typedef void (*CodeGenerationFunc)(st_generator *gt, st_bytecode *code, st_node *node);

typedef bool (*OptimisationMatchFunc)(st_node *node);

static st_uint sizes[255] = {0,};

void check_init(void);

void generation_error(st_generator *gt, const char *message, st_node *node);

void bytecode_init(st_bytecode *code);

void bytecode_destroy(st_bytecode *code);

st_list *get_temporaries(st_generator *gt, st_list *instvars, st_node *arguments, st_node *temporaries);

st_generator *generator_new(void);

void generator_destroy(st_generator *gt);

st_oop create_literals_array(st_generator *gt);

st_oop create_bytecode_array(st_bytecode *code);

void emit(st_bytecode *code, st_uchar value);

int find_instvar(st_generator *gt, char *name);

int find_temporary(st_generator *gt, char *name);

int find_literal_const(st_generator *gt, st_oop literal);

int find_literal_var(st_generator *gt, char *name);

void jump_offset(st_bytecode *code, int offset);

void assign_temp(st_bytecode *code, int index, bool pop);

void assign_instvar(st_bytecode *code, int index, bool pop);

void assign_literal_var(st_bytecode *code, int index, bool pop);

void push(st_bytecode *code, st_uchar value, st_uchar index);

void generate_assign(st_generator *gt, st_bytecode *code, st_node *node, bool pop);

int size_assign(st_generator *gt, st_node *node);

void generate_return(st_generator *gt, st_bytecode *code, st_node *node);

int size_return(st_generator *gt, st_node *node);

st_list *get_block_temporaries(st_generator *gt, st_node *temporaries);

void generate_block(st_generator *gt, st_bytecode *code, st_node *node);

int size_block(st_generator *gt, st_node *node);

bool match_ifTrue(st_node *node);

void generate_ifTrue(st_generator *gt, st_bytecode *code, st_node *node);

bool match_ifFalse(st_node *node);

void generate_ifFalse(st_generator *gt, st_bytecode *code, st_node *node);

bool match_ifTrueifFalse(st_node *node);

void generate_ifTrueifFalse(st_generator *gt, st_bytecode *code, st_node *node);

bool match_ifFalseifTrue(st_node *node);

void generate_ifFalseifTrue(st_generator *gt, st_bytecode *code, st_node *node);

bool match_whileTrue(st_node *node);

void generate_whileTrue(st_generator *gt, st_bytecode *code, st_node *node);

bool match_whileFalse(st_node *node);

void generate_whileFalse(st_generator *gt, st_bytecode *code, st_node *node);

bool match_whileTrueArg(st_node *node);

void generate_whileTrueArg(st_generator *gt, st_bytecode *code, st_node *node);

bool match_whileFalseArg(st_node *node);

void generate_whileFalseArg(st_generator *gt, st_bytecode *code, st_node *node);

bool match_and(st_node *node);

void generate_and(st_generator *gt, st_bytecode *code, st_node *node);

bool match_or(st_node *node);

void generate_or(st_generator *gt, st_bytecode *code, st_node *node);

int size_message(st_generator *gt, st_node *node);

int size_expression(st_generator *gt, st_node *node);

int size_statements(st_generator *gt, st_node *node);

void generate_expression(st_generator *gt, st_bytecode *code, st_node *node);

void generate_statements(st_generator *gt, st_bytecode *code, st_node *statements);

void generate_method_statements(st_generator *gt, st_bytecode *code, st_node *statements);

st_list *collect_temporaries(st_generator *gt, st_node *node);

st_oop st_generate_method(st_oop class, st_node *node, st_compiler_error *error);

void print_bytecodes(st_oop literals, char *codes, int len);

void gen_print_literal(st_oop lit);

void print_literals(st_oop literals);

void st_print_generated_method(st_oop method);

const struct optimisers {
	CodeGenerationFunc generation_func;
	OptimisationMatchFunc match_func;
	
} optimisers[] = {
		{generate_ifTrue,        match_ifTrue},
		{generate_ifFalse,       match_ifFalse},
		{generate_ifTrueifFalse, match_ifTrueifFalse},
		{generate_ifFalseifTrue, match_ifFalseifTrue},
		{generate_whileTrue,     match_whileTrue},
		{generate_whileFalse,    match_whileFalse},
		{generate_whileTrueArg,  match_whileTrueArg},
		{generate_whileFalseArg, match_whileFalseArg},
		{generate_and,           match_and},
		{generate_or,            match_or}
};
