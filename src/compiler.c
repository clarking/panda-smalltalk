
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "universe.h"
#include "dictionary.h"
#include "behavior.h"
#include "input.h"
#include "node.h"
#include "lexer.h"
#include "array.h"

/*
 * st_compile_string:
 * @class: The class for which the compiled method will be bound.
 * @string: Source code for the method
 * @error: return location for errors
 *
 * This function will compile a source string into a new CompiledMethod,
 * and place the method in the methodDictionary of the given class.
 */
bool st_compile_string(Oop class, const char *string, CompilerError *error) {
	Node *node;
	Oop method;
	Lexer *lexer;

	st_assert (class != ST_NIL);

	lexer = st_lexer_new(string);
	if (!lexer)
		return false;

	node = st_parser_parse(lexer, error);
	st_lexer_destroy(lexer);

	if (!node)
		return false;

	method = st_generate_method(class, node, error);
	if (method == ST_NIL) {
		st_node_destroy(node);
		return false;
	}

	st_dictionary_at_put(ST_BEHAVIOR (class)->method_dictionary, node->method.selector, method);
	st_node_destroy(node);
	return true;
}

void filein_error(FileIn *parser, Token *token, const char *message) {
	fprintf(stderr, "%s: %i: %s\n", parser->filename, parser->line + ((token) ? token->line : -90),
			message);
	exit(1);
}

Token *next_token(FileIn *parser, Lexer *lexer) {
	Token *token;
	token = st_lexer_next_token(lexer);
	if (token->type == TOKEN_COMMENT)
		return next_token(parser, lexer);
	else if (token->type == TOKEN_INVALID)
		filein_error(parser, token, st_lexer_error_message(lexer));

	return token;
}

Lexer *next_chunk(FileIn *parser) {
	Lexer *lexer;
	char *chunk;

	parser->line = st_input_get_line(parser->input);
	chunk = st_input_next_chunk(parser->input);
	if (!chunk)
		return NULL;

	lexer = st_lexer_new(chunk);
	st_free(chunk);
	return lexer;
}

void compile_method(FileIn *parser, Lexer *lexer, char *class_name, bool class_method) {
	Token *token = NULL;
	Oop class;
	CompilerError error;

	st_lexer_destroy(lexer);

	// get class or metaclass
	class = st_global_get(class_name);
	if (class == ST_NIL)
		filein_error(parser, token, "undefined class");

	if (class_method)
		class = st_object_class(class);

	// parse method chunk
	lexer = next_chunk(parser);
	if (!lexer)
		filein_error(parser, token, "expected method definition");

	Node *node;
	Oop method;

	node = st_parser_parse(lexer, &error);
	if (node == NULL)
		goto error;
	if (node->type != ST_METHOD_NODE)
		printf("%i\n", node->type);

	method = st_generate_method(class, node, &error);
	if (method == ST_NIL)
		goto error;

	st_dictionary_at_put(ST_BEHAVIOR (class)->method_dictionary, node->method.selector, method);
	st_node_destroy(node);
	st_lexer_destroy(lexer);
	st_free(class_name);

	return;

	error:
	st_node_destroy(node);
	fprintf(stderr, "%s:%i: %s\n", parser->filename, parser->line + error.line - 1, error.message);
	exit(1);
}

void compile_class(FileIn *parser, Lexer *lexer, char *name) {
	printf("%s", "TODO: Not implemented yet");
}

void compile_chunk(FileIn *parser, Lexer *lexer) {
	Token *token;
	char *name;
	token = next_token(parser, lexer);
	if (token->type == TOKEN_IDENTIFIER) {
		name = st_strdup(token->text);
		token = next_token(parser, lexer);

		if (token->type == TOKEN_IDENTIFIER && (streq (token->text, "method")))
			compile_method(parser, lexer, name, false);
		else if (token->type == TOKEN_IDENTIFIER || streq (token->text, "classMethod"))
			compile_method(parser, lexer, name, true);
		else if (token->type == TOKEN_KEYWORD_SELECTOR && streq (token->text, "subclass:"))
			compile_class(parser, lexer, name);
		else
			goto error;
	}
	else
		goto error;

	return;
	error:
	filein_error(parser, token, "unrecognised syntax");
}

void compile_chunks(FileIn *parser) {
	Lexer *lexer;
	while (st_input_look_ahead(parser->input, 1) != ST_INPUT_EOF) {
		lexer = next_chunk(parser);
		if (!lexer)
			continue;
		compile_chunk(parser, lexer);
	}
}

void compile_file_in(const char *filename) {
	char *buffer;
	FileIn *parser;

	st_assert (filename != NULL);
	if (!st_file_get_contents(filename, &buffer))
		return;

	parser = st_new0 (FileIn);
	parser->input = st_input_new(buffer);

	if (!parser->input) {
		fprintf(stderr, "could not validate input file '%s'", filename);
		return;
	}

	parser->filename = basename(filename);
	parser->line = 1;

	compile_chunks(parser);
	st_free(buffer);
	st_input_destroy(parser->input);
	st_free(parser);
}