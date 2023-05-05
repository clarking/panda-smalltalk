
/*
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include "types.h"

void parse_error(Parser *parser, const char *message, Token *token);

void print_parse_error(const char *message, Token *token);

/* adaptor for st_lexer_next_token(). Catches lexer errors and filters out comments */
Token *next(Parser *parser);

Token *current(Lexer *lexer);

Node *parse_statements(Parser *parser);

Node *parse_temporaries(Parser *parser);

Node *parse_subexpression(Parser *parser);

Node *parse_expression(Parser *parser);

int parse_primitive(Parser *parser);

Node *parse_block_arguments(Parser *parser);

Node *parse_block(Parser *parser);

Node *parse_number(Parser *parser);

Node *parse_primary(Parser *parser);

Node *parse_tuple(Parser *parser);

/* identifiers, literals, blocks */

Node *parse_unary_message(Parser *parser, Node *receiver);

Node *parse_binary_argument(Parser *parser, Node *receiver);

Node *parse_binary_message(Parser *parser, Node *receiver);

Node *parse_keyword_argument(Parser *parser, Node *receiver);

Node *parse_keyword_message(Parser *parser, Node *receiver);

/* parses an expression from left to right, by recursively parsing subexpressions */
Node *parse_message(Parser *parser, Node *receiver);

Node *parse_assign(Parser *parser, Node *assignee);

Node *parse_cascade(Parser *parser, Node *first_message);

Node *parse_return(Parser *parser);

Node *parse_statement(Parser *parser);

void parse_message_pattern(Parser *parser, Node *method);

Node *st_parse_method(Parser *parser);

bool parse_variable_names(Lexer *lexer, List **varnames);

void parse_class(Lexer *lexer, Token *token);

void parse_classes(const char *filename);

Node *st_parser_parse(Lexer *lexer, CompilerError *error);



