
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <compiler.h>
#include <generator.h>
#include <lexer.h>
#include <node.h>
#include <universe.h>
#include <object.h>

#include <stdlib.h>
#include <stdio.h>

#define BUF_SIZE 10000

int main(int argc, char *argv[]) {
	CompilerError error;
	Lexer *lexer;
	Node *node;
	Oop method;
	char buffer[BUF_SIZE];
	char c;
	int i = 0;

	printf("Enter a Smalltalk method (for the Association class):\n");

	while ((c = getchar()) != EOF && i < (BUF_SIZE - 1))
		buffer[i++] = c;
	buffer[i] = '\0';

	bootstrap_universe();
	lexer = st_lexer_new(buffer);

	node = st_parser_parse(lexer, &error);
	if (!node) {
		fprintf(stderr, "%s:%i: %s\n", "test-generator", error.line, error.message);
		exit(1);
	}

	method = st_generate_method(st_object_class(ST_ASSOCIATION_CLASS), node, &error);
	if (method == ST_NIL) {
		fprintf(stderr, "%s:%i: %s\n", "test-generator", error.line, error.message);
		exit(1);
	}

	printf("\nGenerated Method:\n\n");
	st_print_generated_method(method);
	st_node_destroy(node);

	return 0;
}

