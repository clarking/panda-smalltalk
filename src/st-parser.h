
/*
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include "st-types.h"

void parse_error(st_parser *parser, const char *message, st_token *token);

void print_parse_error(const char *message, st_token *token);

/* adaptor for st_lexer_next_token(). Catches lexer errors and filters out comments */
st_token *next(st_parser *parser);

st_token *current(st_lexer *lexer);

st_node *parse_statements(st_parser *parser);

st_node *parse_temporaries(st_parser *parser);

st_node *parse_subexpression(st_parser *parser);

st_node *parse_expression(st_parser *parser);

int parse_primitive(st_parser *parser);

st_node *parse_block_arguments(st_parser *parser);

st_node *parse_block(st_parser *parser);

st_node *parse_number(st_parser *parser);

st_node *parse_primary(st_parser *parser);

st_node *parse_tuple(st_parser *parser);

/* identifiers, literals, blocks */
st_node *parse_primary(st_parser *parser);

st_node *parse_unary_message(st_parser *parser, st_node *receiver);

st_node *parse_binary_argument(st_parser *parser, st_node *receiver);

st_node *parse_binary_message(st_parser *parser, st_node *receiver);

st_node *parse_keyword_argument(st_parser *parser, st_node *receiver);

st_node *parse_keyword_message(st_parser *parser, st_node *receiver);

/* parses an expression from left to right, by recursively parsing subexpressions */
st_node *parse_message(st_parser *parser, st_node *receiver);

st_node *parse_assign(st_parser *parser, st_node *assignee);

st_node *parse_cascade(st_parser *parser, st_node *first_message);

st_node *parse_expression(st_parser *parser);

st_node *parse_subexpression(st_parser *parser);

st_node *parse_return(st_parser *parser);

st_node *parse_statement(st_parser *parser);

st_node *parse_statements(st_parser *parser);

int parse_primitive(st_parser *parser);

st_node *parse_temporaries(st_parser *parser);

void parse_message_pattern(st_parser *parser, st_node *method);

st_node *st_parse_method(st_parser *parser);

bool parse_variable_names(st_lexer *lexer, st_list **varnames);

void parse_class(st_lexer *lexer, st_token *token);

void parse_classes(const char *filename);

st_node *st_parser_parse(st_lexer *lexer, st_compiler_error *error);



