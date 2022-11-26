
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include "st-types.h"

typedef struct st_lexer st_lexer;
typedef struct st_token st_token;

typedef enum {
	ST_TOKEN_INVALID,
	ST_TOKEN_LPAREN,
	ST_TOKEN_RPAREN,
	ST_TOKEN_BLOCK_BEGIN,
	ST_TOKEN_BLOCK_END,
	ST_TOKEN_COMMA,
	ST_TOKEN_SEMICOLON,
	ST_TOKEN_PERIOD,
	ST_TOKEN_RETURN,
	ST_TOKEN_COLON,
	ST_TOKEN_ASSIGN,
	ST_TOKEN_TUPLE_BEGIN,
	ST_TOKEN_IDENTIFIER,
	ST_TOKEN_CHARACTER_CONST,
	ST_TOKEN_STRING_CONST,
	ST_TOKEN_NUMBER_CONST,
	ST_TOKEN_SYMBOL_CONST,
	ST_TOKEN_COMMENT,
	ST_TOKEN_BINARY_SELECTOR,
	ST_TOKEN_KEYWORD_SELECTOR,
	ST_TOKEN_EOF
	
} st_token_type;

st_lexer *st_lexer_new(const char *string);

st_token *st_lexer_next_token(st_lexer *lexer);

st_token *st_lexer_current_token(st_lexer *lexer);

void st_lexer_destroy(st_lexer *lexer);

char st_lexer_error_char(st_lexer *lexer);

st_uint st_lexer_error_line(st_lexer *lexer);

st_uint st_lexer_error_column(st_lexer *lexer);

char *st_lexer_error_message(st_lexer *lexer);

void st_lexer_filter_comments(st_lexer *lexer, bool filter);

st_token_type st_token_get_type(st_token *token);

char *st_token_get_text(st_token *token);

st_uint st_token_get_line(st_token *token);

st_uint st_token_get_column(st_token *token);

bool st_number_token_negative(st_token *token);

char *st_number_token_number(st_token *token);

st_uint st_number_token_radix(st_token *token);

int st_number_token_exponent(st_token *token);

