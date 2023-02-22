

/*
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "utils.h"

#define DEFAULT_CODE_SIZE 20
#define CSTRING(string) ((char *) st_byte_array_bytes (string))

#define NEXT(ip)      \
    ip += sizes[*ip]; \
    break

#define FORMAT(ip) (formats[sizes[*ip]-1])

typedef void (*CodeGenerationFunc)(Generator *gt, Bytecode *code, Node *node);

typedef bool (*OptimisationMatchFunc)(Node *node);

static st_uint sizes[255] = {0,};

void check_init(void);

void generation_error(Generator *gt, const char *message, Node *node);

void bytecode_init(Bytecode *code);

void bytecode_destroy(Bytecode *code);

List *get_temporaries(Generator *gt, List *instvars, Node *arguments, Node *temporaries);

Generator *generator_new(void);

void generator_destroy(Generator *gt);

Oop create_literals_array(Generator *gt);

Oop create_bytecode_array(Bytecode *code);

void emit(Bytecode *code, st_uchar value);

int find_instvar(Generator *gt, char *name);

int find_temporary(Generator *gt, char *name);

int find_literal_const(Generator *gt, Oop literal);

int find_literal_var(Generator *gt, char *name);

void jump_offset(Bytecode *code, int offset);

void assign_temp(Bytecode *code, int index, bool pop);

void assign_instvar(Bytecode *code, int index, bool pop);

void assign_literal_var(Bytecode *code, int index, bool pop);

void push(Bytecode *code, st_uchar value, st_uchar index);

void generate_assign(Generator *gt, Bytecode *code, Node *node, bool pop);

int size_assign(Generator *gt, Node *node);

void generate_return(Generator *gt, Bytecode *code, Node *node);

int size_return(Generator *gt, Node *node);

List *get_block_temporaries(Generator *gt, Node *temporaries);

void generate_block(Generator *gt, Bytecode *code, Node *node);

int size_block(Generator *gt, Node *node);

bool match_ifTrue(Node *node);

void generate_ifTrue(Generator *gt, Bytecode *code, Node *node);

bool match_ifFalse(Node *node);

void generate_ifFalse(Generator *gt, Bytecode *code, Node *node);

bool match_ifTrueifFalse(Node *node);

void generate_ifTrueifFalse(Generator *gt, Bytecode *code, Node *node);

bool match_ifFalseifTrue(Node *node);

void generate_ifFalseifTrue(Generator *gt, Bytecode *code, Node *node);

bool match_whileTrue(Node *node);

void generate_whileTrue(Generator *gt, Bytecode *code, Node *node);

bool match_whileFalse(Node *node);

void generate_whileFalse(Generator *gt, Bytecode *code, Node *node);

bool match_whileTrueArg(Node *node);

void generate_whileTrueArg(Generator *gt, Bytecode *code, Node *node);

bool match_whileFalseArg(Node *node);

void generate_whileFalseArg(Generator *gt, Bytecode *code, Node *node);

bool match_and(Node *node);

void generate_and(Generator *gt, Bytecode *code, Node *node);

bool match_or(Node *node);

void generate_or(Generator *gt, Bytecode *code, Node *node);

int size_message(Generator *gt, Node *node);

int size_expression(Generator *gt, Node *node);

int size_statements(Generator *gt, Node *node);

void generate_expression(Generator *gt, Bytecode *code, Node *node);

void generate_statements(Generator *gt, Bytecode *code, Node *statements);

void generate_method_statements(Generator *gt, Bytecode *code, Node *statements);

List *collect_temporaries(Generator *gt, Node *node);

Oop st_generate_method(Oop class, Node *node, CompilerError *error);

void print_bytecodes(Oop literals, char *codes, int len);

void gen_print_literal(Oop lit);

void print_literals(Oop literals);

void st_print_generated_method(Oop method);

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
