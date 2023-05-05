
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
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

void st_lexer_destroy(Lexer *lexer);

char *st_lexer_error_message(Lexer *lexer);

bool st_number_token_negative(Token *token);

char *st_number_token_number(Token *token);

uint st_number_token_radix(Token *token);

int st_number_token_exponent(Token *token);

