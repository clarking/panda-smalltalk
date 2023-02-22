
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include "types.h"

typedef struct Lexer Lexer;
typedef struct Token Token;


Lexer *st_lexer_new(const char *string);

Token *st_lexer_next_token(Lexer *lexer);

Token *st_lexer_current_token(Lexer *lexer);

void st_lexer_destroy(Lexer *lexer);

char st_lexer_error_char(Lexer *lexer);

st_uint st_lexer_error_line(Lexer *lexer);

st_uint st_lexer_error_column(Lexer *lexer);

char *st_lexer_error_message(Lexer *lexer);

void st_lexer_filter_comments(Lexer *lexer, bool filter);

TokenType st_token_get_type(Token *token);

char *st_token_get_text(Token *token);

st_uint st_token_get_line(Token *token);

st_uint st_token_get_column(Token *token);

bool st_number_token_negative(Token *token);

char *st_number_token_number(Token *token);

st_uint st_number_token_radix(Token *token);

int st_number_token_exponent(Token *token);

