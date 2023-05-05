/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "types.h"
#include <tommath.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include "parser.h"
#include "compiler.h"
#include "lexer.h"
#include "utils.h"
#include "primitives.h"
#include "array.h"
#include "symbol.h"
#include "float.h"
#include "large-integer.h"
#include "universe.h"
#include "object.h"
#include "character.h"
#include "unicode.h"
#include "behavior.h"

void parse_error(Parser *parser, const char *message, Token *token) {
	if (parser->error) {
		strncpy(parser->error->message, message, 255);
		parser->error->line = token->line;
		parser->error->column = token->column;
	}

	longjmp(parser->jmploc, 0);
}

void print_parse_error(const char *message, Token *token) {
	fprintf(stderr, "error: %i: %i: %s", token->line, token->column, message);
	exit(1);
}

/* adaptor for st_lexer_next_token(). Catches lexer errors and filters out comments */
Token *next(Parser *parser) {
	Token *token;
	TokenType type;

	token = st_lexer_next_token(parser->lexer);
	type = token->type;

	if (type == TOKEN_COMMENT)
		return next(parser);
	else if (type == TOKEN_INVALID)
		parse_error(parser, st_lexer_error_message(parser->lexer), token);

	return token;
}

Token *current(Lexer *lexer) {
	return lexer->token;
}

Node *parse_block_arguments(Parser *parser) {
	Token *token;
	Node *arguments = NULL, *node;

	token = current(parser->lexer);

	while (token->type == TOKEN_COLON) {

		token = next(parser);
		if (token->type != TOKEN_IDENTIFIER)
			parse_error(parser, "expected identifier", token);

		node = st_node_new(ST_VARIABLE_NODE);
		node->line = token->line;
		node->variable.name = st_strdup(token->text);
		arguments = st_node_list_append(arguments, node);

		token = next(parser);
	}

	if (token->type != TOKEN_BINARY_SELECTOR ||
		!streq (token->text, "|"))
		parse_error(parser, "expected ':' or '|'", token);

	next(parser);

	return arguments;
}

Node *parse_block(Parser *parser) {
	Token *token;
	Node *node;
	bool nested;

	node = st_node_new(ST_BLOCK_NODE);

	// parse block arguments
	token = next(parser);

	node->line = token->line;

	if (token->type == TOKEN_COLON)
		node->block.arguments = parse_block_arguments(parser);

	token = current(parser->lexer);
	if (token->type == TOKEN_BINARY_SELECTOR
		&& streq (token->text, "|"))
		node->block.temporaries = parse_temporaries(parser);

	nested = parser->in_block;
	parser->in_block = true;

	node->block.statements = parse_statements(parser);

	if (nested == false)
		parser->in_block = false;

	token = current(parser->lexer);
	if (token->type != TOKEN_BLOCK_END)
		parse_error(parser, "expected ']'", token);
	next(parser);

	return node;
}

Node *parse_number(Parser *parser) {
	int radix;
	int sign;
	int exponent;
	char *number, *p;
	Node *node;
	Token *token;

	token = current(parser->lexer);

	sign = st_number_token_negative(token) ? -1 : 1;
	radix = st_number_token_radix(token);
	exponent = st_number_token_exponent(token);

	p = number = st_number_token_number(token);

	node = st_node_new(ST_LITERAL_NODE);
	node->line = token->line;

	/* check if there is a decimal point */
	while (*p && *p != '.')
		++p;

	if (*p == '.' || exponent != 0) {

		char *format;

		if (radix != 10)
			parse_error(parser, "only base-10 floats are supported at the moment", token);

		format = st_strdup_printf("%se%i", number, exponent);

		node->literal.value = st_float_new(sign * strtod(format, NULL));

		st_free(format);

	}
	else {

		errno = 0;
		long int integer = sign * strtol(number, NULL, radix);

		/* check for overflow */
		if (errno == ERANGE
			|| integer < INT_MIN || integer > INT_MAX) {

			mp_int value;
			mp_err result;

			result = mp_init(&value);
			if (result != MP_OKAY)
				parse_error(parser, "memory exhausted while trying parse LargeInteger", token);

			result = mp_read_radix(&value, number, radix);
			if (result != MP_OKAY)
				parse_error(parser, "memory exhausted while trying parse LargeInteger", token);

			if (sign == -1)
				result = mp_neg(&value, &value);
			if (result != MP_OKAY)
				parse_error(parser, "memory exhausted while trying parse negative LargeInteger", token);

			node->literal.value = st_large_integer_new(&value);

		}
		else {
			node->literal.value = st_smi_new(integer);
		}
	}

	next(parser);

	return node;
}

Node *parse_tuple(Parser *parser) {
	Token *token;
	Node *node;
	List *items = NULL;

	token = next(parser);
	while (true) {

		switch (token->type) {
			case TOKEN_NUMBER_CONST:
			case TOKEN_STRING_CONST:
			case TOKEN_SYMBOL_CONST:
			case TOKEN_CHARACTER_CONST:
				node = parse_primary(parser);
				items = st_list_prepend(items, (void *) node->literal.value);
				st_node_destroy(node);
				break;

			case TOKEN_LPAREN:
				node = parse_tuple(parser);
				items = st_list_prepend(items, (void *) node->literal.value);
				st_node_destroy(node);
				break;

			default:
				if (token->type == TOKEN_RPAREN) {
					goto out;
				}
				else
					parse_error(parser, "expected ')'", token);
		}

		token = current(parser->lexer);
	}

	out:
	token = next(parser);
	Oop tuple;

	items = st_list_reverse(items);

	tuple = st_object_new_arrayed(ST_ARRAY_CLASS, st_list_length(items));

	int i = 1;
	for (List *l = items; l; l = l->next)
		st_array_at_put(tuple, i++, (Oop) l->data);

	node = st_node_new(ST_LITERAL_NODE);
	node->literal.value = tuple;
	node->line = token->line;

	st_list_destroy(items);

	return node;
}

/* identifiers, literals, blocks */
Node *parse_primary(Parser *parser) {
	Node *node = NULL;
	Token *token;

	token = current(parser->lexer);

	switch (token->type) {

		case TOKEN_IDENTIFIER:

			node = st_node_new(ST_VARIABLE_NODE);
			node->line = token->line;
			node->variable.name = st_strdup(token->text);

			next(parser);
			break;

		case TOKEN_NUMBER_CONST:

			node = parse_number(parser);
			break;

		case TOKEN_STRING_CONST:

			node = st_node_new(ST_LITERAL_NODE);
			node->line = token->line;
			node->literal.value = st_string_new(token->text);

			next(parser);
			break;

		case TOKEN_SYMBOL_CONST:

			node = st_node_new(ST_LITERAL_NODE);
			node->line = token->line;
			node->literal.value = st_symbol_new(token->text);

			next(parser);
			break;

		case TOKEN_CHARACTER_CONST:

			node = st_node_new(ST_LITERAL_NODE);
			node->line = token->line;
			node->literal.value = st_character_new(st_utf8_get_unichar(token->text));

			next(parser);
			break;

		case TOKEN_BLOCK_BEGIN:

			node = parse_block(parser);
			break;

		case TOKEN_TUPLE_BEGIN:

			node = parse_tuple(parser);
			break;

		default:
			parse_error(parser, "expected expression", token);
	}

	return node;
}

Node *parse_unary_message(Parser *parser, Node *receiver) {
	Token *token;
	Node *node;

	token = current(parser->lexer);

	node = st_node_new(ST_MESSAGE_NODE);
	node->line = token->line;
	node->msg.precedence = ST_UNARY_PRECEDENCE;
	node->msg.receiver = receiver;
	node->msg.selector = st_symbol_new(token->text);
	node->msg.arguments = NULL;

	next(parser);
	return node;
}

Node *parse_binary_argument(Parser *parser, Node *receiver) {
	Node *node;
	Token *token;

	token = current(parser->lexer);
	if (token->type != TOKEN_IDENTIFIER)
		return receiver;

	node = parse_unary_message(parser, receiver);
	return parse_binary_argument(parser, node);
}

Node *parse_binary_message(Parser *parser, Node *receiver) {
	Node *node, *argument;
	Token *token;
	char *selector;

	token = current(parser->lexer);
	selector = token->text;

	/* parse the primary */
	token = next(parser);
	if (token->type == TOKEN_LPAREN)
		argument = parse_subexpression(parser);
	else
		argument = parse_primary(parser);

	argument = parse_binary_argument(parser, argument);
	node = st_node_new(ST_MESSAGE_NODE);

	node->msg.precedence = ST_BINARY_PRECEDENCE;
	node->msg.receiver = receiver;
	node->msg.selector = st_symbol_new(selector);
	node->msg.arguments = argument;

	return node;
}

Node *parse_keyword_argument(Parser *parser, Node *receiver) {
	Token *token;

	token = current(parser->lexer);

	if (receiver == NULL) {
		/* parse the primary */
		if (token->type == TOKEN_LPAREN)
			receiver = parse_subexpression(parser);
		else
			receiver = parse_primary(parser);

	}
	else if (token->type == TOKEN_IDENTIFIER) {
		receiver = parse_unary_message(parser, receiver);

	}
	else if (token->type == TOKEN_BINARY_SELECTOR && !streq (token->text, "!")) {
		receiver = parse_binary_message(parser, receiver);

	}
	else {
		return receiver;
	}

	return parse_keyword_argument(parser, receiver);
}

Node *parse_keyword_message(Parser *parser, Node *receiver) {
	Token *token;
	Node *node, *arguments = NULL, *arg;
	char *temp, *string = st_strdup("");

	token = current(parser->lexer);

	while (token->type == TOKEN_KEYWORD_SELECTOR) {

		temp = st_strconcat(string, token->text, NULL);
		st_free(string);
		string = temp;

		token = next(parser);
		arg = parse_keyword_argument(parser, NULL);
		arguments = st_node_list_append(arguments, arg);

		token = current(parser->lexer);
	}

	node = st_node_new(ST_MESSAGE_NODE);

	node->msg.precedence = ST_KEYWORD_PRECEDENCE;
	node->msg.receiver = receiver;
	node->msg.selector = st_symbol_new(string);
	node->msg.arguments = arguments;

	st_free(string);

	return node;
}

/* parses an expression from left to right, by recursively parsing subexpressions */
Node *parse_message(Parser *parser, Node *receiver) {
	Node *message = NULL;
	Token *token;
	TokenType type;

	/* Before parsing messages, check if expression is simply a variable.
	 * This is the case if token is ')' or '.' */
	token = current(parser->lexer);
	type = token->type;

	if (type == TOKEN_PERIOD || type == TOKEN_RPAREN || type == TOKEN_SEMICOLON
		|| type == TOKEN_EOF || type == TOKEN_BLOCK_END
		|| (type == TOKEN_BINARY_SELECTOR && streq (token->text, "!")))
		return receiver;

	if (type == TOKEN_IDENTIFIER)
		message = parse_unary_message(parser, receiver);

	else if (type == TOKEN_BINARY_SELECTOR)
		message = parse_binary_message(parser, receiver);

	else if (type == TOKEN_KEYWORD_SELECTOR)
		message = parse_keyword_message(parser, receiver);

	else
		parse_error(parser, "nothing more expected", token);

	return parse_message(parser, message);
}

Node *parse_assign(Parser *parser, Node *assignee) {
	Token *token;
	Node *node, *expression;

	token = next(parser);
	expression = parse_expression(parser);

	node = st_node_new(ST_ASSIGN_NODE);
	node->line = token->line;
	node->assign.assignee = assignee;
	node->assign.expression = expression;

	return node;
}

Node *parse_cascade(Parser *parser, Node *first_message) {
	Token *token;
	Node *message, *node;
	bool super_send = first_message->msg.super_send;
	token = current(parser->lexer);

	node = st_node_new(ST_CASCADE_NODE);
	node->line = token->line;

	node->cascade.receiver = first_message->msg.receiver;
	node->cascade.messages = st_list_append(node->cascade.messages, first_message);

	first_message->msg.receiver = NULL;

	while (token->type == TOKEN_SEMICOLON) {
		next(parser);
		message = parse_message(parser, NULL);

		if (message == NULL)
			parse_error(parser, "expected cascade", token);

		message->msg.super_send = super_send;

		node->cascade.messages = st_list_append(node->cascade.messages, message);
		token = current(parser->lexer);
	}

	return node;
}

Node *parse_expression(Parser *parser) {
	Node *receiver = NULL;
	Node *message;
	Token *token;
	bool super_send = false;

	token = current(parser->lexer);
	switch (token->type) {
		case TOKEN_NUMBER_CONST:
		case TOKEN_STRING_CONST:
		case TOKEN_SYMBOL_CONST:
		case TOKEN_CHARACTER_CONST:
		case TOKEN_BLOCK_BEGIN:
		case TOKEN_TUPLE_BEGIN:
			receiver = parse_primary(parser);
			break;
		case TOKEN_IDENTIFIER:
			receiver = parse_primary(parser);
			token = current(parser->lexer);
			if (token->type == TOKEN_ASSIGN)
				return parse_assign(parser, receiver);
			break;
		case TOKEN_LPAREN:
			receiver = parse_subexpression(parser);
			break;
		default:
			parse_error(parser, "expected expression", token);
	}

	/* check if receiver is pseudo-variable 'super' */
	if (receiver->type == ST_VARIABLE_NODE
		&& streq (receiver->variable.name, "super"))
		super_send = true;

	message = parse_message(parser, receiver);
	message->msg.super_send = super_send;

	token = current(parser->lexer);
	if (token->type == TOKEN_SEMICOLON)
		return parse_cascade(parser, message);
	else
		return message;
}

Node *parse_subexpression(Parser *parser) {
	Token *token;
	Node *expression;

	next(parser);
	expression = parse_expression(parser);

	token = current(parser->lexer);
	if (token->type != TOKEN_RPAREN)
		parse_error(parser, "expected ')' after expression", token);

	next(parser);
	return expression;
}

Node *parse_return(Parser *parser) {
	Node *node;
	Token *token;

	token = next(parser);
	node = st_node_new(ST_RETURN_NODE);
	node->line = token->line;
	node->retrn.expression = parse_expression(parser);
	return node;
}

Node *parse_statement(Parser *parser) {
	Token *token;

	token = current(parser->lexer);
	if (token->type == TOKEN_RETURN)
		return parse_return(parser);
	else
		return parse_expression(parser);
}

Node *parse_statements(Parser *parser) {
	Token *token;
	Node *expression = NULL, *statements = NULL;

	token = current(parser->lexer);
	while (token->type != TOKEN_EOF
		   && token->type != TOKEN_BLOCK_END) {

		/* check for unreachable statements */
		if (expression && expression->type == ST_RETURN_NODE) {
			/* first check that unreachable statement is valid ! */
			parse_statement(parser);
			parse_error(parser, "statement is unreachable", token);
		}

		expression = parse_statement(parser);
		statements = st_node_list_append(statements, expression);

		/* Consume statement delimiter ('.') if there is one.
		 *
		 * If the current token is a wrongly placed/mismatched
		 * closing token (')' or ']'), then parse_expression() will handle
		 * that.
		 */

		token = current(parser->lexer);
		if (token->type == TOKEN_PERIOD) {
			token = next(parser);
		}
	}

	for (Node *node = statements; node; node = node->next) {
		if (parser->in_block && node->type == ST_MESSAGE_NODE && node->next != NULL)
			node->msg.is_statement = true;
		else if ((!parser->in_block) && node->type == ST_MESSAGE_NODE)
			node->msg.is_statement = true;

		if (parser->in_block && node->type == ST_CASCADE_NODE && node->next != NULL)
			node->cascade.is_statement = true;
		else if ((!parser->in_block) && node->type == ST_CASCADE_NODE)
			node->cascade.is_statement = true;
	}
	return statements;
}

int parse_primitive(Parser *parser) {
	Token *token;
	int index = -1;

	token = current(parser->lexer);
	if (token->type != TOKEN_BINARY_SELECTOR
		|| !streq (token->text, "<"))
		return -1;

	token = next(parser);
	if (token->type == TOKEN_KEYWORD_SELECTOR
		&& streq (token->text, "primitive:")) {

		token = next(parser);
		if (token->type != TOKEN_STRING_CONST)
			parse_error(parser, "expected string literal", token);

		index = st_prim_index_for_name(token->text);
		if (index < 0)
			parse_error(parser, "unknown primitive", token);

		token = next(parser);
		if (token->type != TOKEN_BINARY_SELECTOR
			|| !streq (token->text, ">"))
			parse_error(parser, "expected '>'", token);

		next(parser);

	}
	else {
		parse_error(parser, "expected primitive declaration", token);
	}

	return index;
}

Node *parse_temporaries(Parser *parser) {
	Token *token;
	Node *temporaries = NULL, *temp;

	token = current(parser->lexer);

	if (token->type != TOKEN_BINARY_SELECTOR
		|| !streq (token->text, "|"))
		return NULL;

	token = next(parser);
	while (token->type == TOKEN_IDENTIFIER) {

		temp = st_node_new(ST_VARIABLE_NODE);
		temp->line = token->line;
		temp->variable.name = st_strdup(token->text);

		temporaries = st_node_list_append(temporaries, temp);

		token = next(parser);
	}

	if (token->type != TOKEN_BINARY_SELECTOR
		|| !streq (token->text, "|"))
		parse_error(parser, "expected '|'", token);

	token = next(parser);

	return temporaries;
}

void parse_message_pattern(Parser *parser, Node *method) {
	Token *token;
	TokenType type;
	Node *arguments = NULL;

	token = next(parser);
	type = token->type;

	if (type == TOKEN_IDENTIFIER) {

		method->method.selector = st_symbol_new(token->text);
		method->method.precedence = ST_UNARY_PRECEDENCE;

		next(parser);

	}
	else if (type == TOKEN_BINARY_SELECTOR) {

		method->method.selector = st_symbol_new(token->text);

		token = next(parser);
		if (token->type != TOKEN_IDENTIFIER)
			parse_error(parser, "argument name expected after binary selector", token);

		arguments = st_node_new(ST_VARIABLE_NODE);
		arguments->line = token->line;
		arguments->variable.name = st_strdup(token->text);

		method->method.precedence = ST_BINARY_PRECEDENCE;

		next(parser);

	}
	else if (type == TOKEN_KEYWORD_SELECTOR) {

		char *temp, *string = st_strdup("");
		Node *arg;

		while (token->type == TOKEN_KEYWORD_SELECTOR) {

			temp = st_strconcat(string, token->text, NULL);
			st_free(string);
			string = temp;

			token = next(parser);
			if (token->type != TOKEN_IDENTIFIER)
				parse_error(parser, "argument name expected after keyword", token);

			arg = st_node_new(ST_VARIABLE_NODE);
			arg->line = token->line;
			arg->variable.name = st_strdup(token->text);
			arguments = st_node_list_append(arguments, arg);

			token = next(parser);
		}

		method->method.selector = st_symbol_new(string);
		method->method.precedence = ST_KEYWORD_PRECEDENCE;
		st_free(string);

	}
	else {
		parse_error(parser, "invalid message pattern", token);
	}

	method->method.arguments = arguments;
}

Node *st_parse_method(Parser *parser) {
	Node *node;

	parser->in_block = false;

	node = st_node_new(ST_METHOD_NODE);

	node->method.primitive = -1;

	parse_message_pattern(parser, node);

	node->method.temporaries = parse_temporaries(parser);
	node->method.primitive = parse_primitive(parser);
	node->method.statements = parse_statements(parser);

	st_assert (node->type == ST_METHOD_NODE);

	return node;
}

bool parse_variable_names(Lexer *lexer, List **varnames) {
	Lexer *ivarlexer;
	Token *token;
	char *names;

	token = st_lexer_next_token(lexer);

	if (token->type != TOKEN_STRING_CONST)
		return false;

	names = st_strdup(token->text);
	ivarlexer = st_lexer_new(names);
	token = st_lexer_next_token(ivarlexer);

	while (token->type != TOKEN_EOF) {

		if (token->type != TOKEN_IDENTIFIER)
			print_parse_error(NULL, token);

		*varnames = st_list_append(*varnames, st_strdup(token->text));
		token = st_lexer_next_token(ivarlexer);
	}

	st_free(names);
	st_lexer_destroy(ivarlexer);

	return true;
}

void parse_class(Lexer *lexer, Token *token) {
	char *class_name = NULL;
	char *superclass_name = NULL;
	List *ivarnames = NULL;

	// 'Class' token
	if (token->type != TOKEN_IDENTIFIER || !streq (token->text, "Class"))
		print_parse_error("expected class definition", token);

	// `named:' token
	token = st_lexer_next_token(lexer);
	if (token->type != TOKEN_KEYWORD_SELECTOR || !streq (token->text, "named:"))
		print_parse_error("expected 'name:'", token);

	// class name
	token = st_lexer_next_token(lexer);
	if (token->type == TOKEN_STRING_CONST)
		class_name = st_strdup(token->text);
	else
		print_parse_error("expected string literal", token);

	// `superclass:' token
	token = st_lexer_next_token(lexer);
	if (token->type != TOKEN_KEYWORD_SELECTOR || !streq (token->text, "superclass:"))
		print_parse_error("expected 'superclass:'", token);

	// superclass name
	token = st_lexer_next_token(lexer);
	if (token->type == TOKEN_STRING_CONST)
		superclass_name = st_strdup(token->text);
	else
		print_parse_error("expected string literal", token);

	// 'instanceVariableNames:' keyword selector
	token = st_lexer_next_token(lexer);
	if (token->type == TOKEN_KEYWORD_SELECTOR &&
		streq (token->text, "instanceVariableNames:"))
		parse_variable_names(lexer, &ivarnames);
	else
		print_parse_error(NULL, token);

	token = st_lexer_next_token(lexer);
	initialize_class(class_name, superclass_name, ivarnames);
	st_list_foreach(ivarnames, st_free);
	st_list_destroy(ivarnames);
	st_free(class_name);
	st_free(superclass_name);
	return;
}

void parse_classes(const char *filename) {
	char *contents;
	Lexer *lexer;
	Token *token;

	if (!st_file_get_contents(filename, &contents)) {
		exit(1);
	}

	lexer = st_lexer_new(contents);
	st_assert (lexer != NULL);
	token = st_lexer_next_token(lexer);

	while (token->type != TOKEN_EOF) {
		while (token->type == TOKEN_COMMENT)
			token = st_lexer_next_token(lexer);
		parse_class(lexer, token);
		token = st_lexer_next_token(lexer);
	}

	st_free(contents);
	st_lexer_destroy(lexer);
}

Node *st_parser_parse(Lexer *lexer, CompilerError *error) {
	Parser *parser;
	Node *method;

	st_assert (lexer != NULL);

	parser = st_new0 (Parser);

	parser->lexer = lexer;
	parser->error = error;
	parser->in_block = false;

	if (!setjmp (parser->jmploc)) {
		method = st_parse_method(parser);
	}
	else {
		method = NULL;
	}

	st_free(parser);

	return method;
}