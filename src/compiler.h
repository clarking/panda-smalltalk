
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "utils.h"
#include "lexer.h"
#include "node.h"

/* isn't declared in glibc string.h */
char *basename(const char *FILENAME);

bool st_compile_string(Oop class, const char *string, CompilerError *error);

void filein_error(FileIn *parser, Token *token, const char *message);

Token *next_token(FileIn *parser, Lexer *lexer);

Lexer *next_chunk(FileIn *parser);

void compile_method(FileIn *parser, Lexer *lexer, char *class_name, bool class_method);

void compile_class(FileIn *parser, Lexer *lexer, char *name);

void compile_chunk(FileIn *parser, Lexer *lexer);

void compile_chunks(FileIn *parser);

Node *st_parser_parse(Lexer *lexer, CompilerError *error);

Oop st_generate_method(Oop class, Node *node, CompilerError *error);

void compile_file_in(const char *filename);

/* bytecodes */
typedef enum {
	PUSH_TEMP = 0,
	PUSH_INSTVAR,
	PUSH_LITERAL_CONST,
	PUSH_LITERAL_VAR,
	STORE_LITERAL_VAR,
	STORE_TEMP,
	STORE_INSTVAR,
	STORE_POP_LITERAL_VAR,
	STORE_POP_TEMP,
	STORE_POP_INSTVAR,
	PUSH_SELF,
	PUSH_NIL,
	PUSH_TRUE,
	PUSH_FALSE,
	PUSH_INTEGER,
	RETURN_STACK_TOP,
	BLOCK_RETURN,
	POP_STACK_TOP,
	DUPLICATE_STACK_TOP,
	PUSH_ACTIVE_CONTEXT,
	BLOCK_COPY,
	JUMP_TRUE,
	JUMP_FALSE,
	JUMP,
	SEND,        /* B, B (arg count), B (selector index) */
	SEND_SUPER,
	SEND_PLUS,
	SEND_MINUS,
	SEND_LT,
	SEND_GT,
	SEND_LE,
	SEND_GE,
	SEND_EQ,
	SEND_NE,
	SEND_MUL,
	SEND_DIV,
	SEND_MOD,
	SEND_BITSHIFT,
	SEND_BITAND,
	SEND_BITOR,
	SEND_BITXOR,
	SEND_AT,
	SEND_AT_PUT,
	SEND_SIZE,
	SEND_VALUE,
	SEND_VALUE_ARG,
	SEND_IDENTITY_EQ,
	SEND_CLASS,
	SEND_NEW,
	SEND_NEW_ARG,
} Code;

