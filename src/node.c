
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "node.h"
#include "array.h"
#include "types.h"
#include "object.h"
#include "float.h"
#include "character.h"
#include "unicode.h"

void print_variable(Node *node) {
	st_assert (node->type == ST_VARIABLE_NODE);
	printf("%s", node->variable.name);
}

void print_object(Oop object) {
	if (st_object_is_smi(object))
		printf("%i", st_smi_value(object));
	else if (st_object_is_symbol(object))
		printf("#%s", st_byte_array_bytes(object));
	else if (st_object_class(object) == ST_STRING_CLASS)
		printf("%s", st_byte_array_bytes(object));
	else if (st_object_is_float(object))
		printf("%f", st_float_value(object));
	else if (st_object_class(object) == ST_CHARACTER_CLASS) {
		char outbuf[6] = {0};
		st_unichar_to_utf8(st_character_value(object), outbuf);
		printf("$%s", outbuf);
	}
	else if (st_object_class(object) == ST_ARRAY_CLASS)
		print_tuple(object);
}

void print_tuple(Oop tuple) {

	int size;
	size = st_smi_value(st_arrayed_object_size(tuple));
	printf("#(");
	for (int i = 1; i <= size; i++) {
		print_object(st_array_at(tuple, i));
		if (i < size)
			printf(" ");
	}
	printf(")");
}

void node_print_literal(Node *node) {
	st_assert (node->type == ST_LITERAL_NODE);
	print_object(node->literal.value);
}

void print_return(Node *node) {
	st_assert (node->type == ST_RETURN_NODE);
	printf("^ ");
	print_expression(node->retrn.expression);
}

void print_assign(Node *node) {
	st_assert (node->type == ST_ASSIGN_NODE);
	print_variable(node->assign.assignee);
	printf(" := ");
	print_expression(node->assign.expression);
}

char **extract_keywords(char *selector) {
	int len = strlen(selector);
	/* hide trailing ':' */
	selector[len - 1] = 0;
	char **keywords = st_strsplit(selector, ":", 0);
	selector[len - 1] = ':';
	return keywords;
}

void print_method_node(Node *node) {
	st_assert (node->type == ST_METHOD_NODE);
	if (node->method.precedence == ST_KEYWORD_PRECEDENCE) {
		char *selector = (char *) st_byte_array_bytes(node->method.selector);
		char **keywords = extract_keywords(selector);
		Node *arguments = node->method.arguments;
		for (char **keyword = keywords; *keyword; keyword++) {
			printf("%s: ", *keyword);
			print_variable(arguments);
			printf(" ");
			arguments = arguments->next;
		}
		st_strfreev(keywords);
	}
	else if (node->method.precedence == ST_BINARY_PRECEDENCE) {
		printf("%s", (char *) st_byte_array_bytes(node->method.selector));
		printf(" ");
		print_variable(node->method.arguments);
	}
	else
		printf("%s", (char *) st_byte_array_bytes(node->method.selector));

	printf("\n");

	if (node->method.temporaries != NULL) {
		printf("| ");
		Node *temp = node->method.temporaries;
		for (; temp; temp = temp->next) {
			print_variable(temp);
			printf(" ");
		}
		printf("|\n");
	}

	if (node->method.primitive >= 0)
		printf("<primitive: %i>\n", node->method.primitive);

	Node *stm = node->method.statements;
	for (; stm; stm = stm->next) {
		if (stm->type == ST_RETURN_NODE)
			print_return(stm);
		else
			print_expression(stm);
		printf(".\n");
	}
}

void print_block(Node *node) {
	st_assert (node->type == ST_BLOCK_NODE);
	printf("[ ");
	if (node->block.arguments != NULL) {
		Node *arg = node->block.arguments;
		for (; arg; arg = arg->next) {
			printf(":");
			print_variable(arg);
			printf(" ");
		}
		printf("|");
		printf(" ");
	}

	if (node->block.temporaries != NULL) {
		printf("| ");
		Node *temp = node->block.temporaries;
		for (; temp; temp = temp->next) {
			print_variable(temp);
			printf(" ");
		}
		printf("|");
		printf(" ");
	}

	Node *stm = node->block.statements;
	for (; stm; stm = stm->next) {
		if (stm->type == ST_RETURN_NODE)
			print_return(stm);
		else
			print_expression(stm);

		if (stm->next != NULL)
			printf(". ");
	}

	printf(" ]");
}

void print_message(Node *node) {

	if (node->msg.precedence == ST_UNARY_PRECEDENCE) {
		if (node->msg.receiver->type == ST_MESSAGE_NODE &&
			(node->msg.receiver->msg.precedence == ST_BINARY_PRECEDENCE ||
			 node->msg.receiver->msg.precedence == ST_KEYWORD_PRECEDENCE))
			printf("(");

		print_expression(node->msg.receiver);
		if (node->msg.receiver->type == ST_MESSAGE_NODE &&
			(node->msg.receiver->msg.precedence == ST_BINARY_PRECEDENCE ||
			 node->msg.receiver->msg.precedence == ST_KEYWORD_PRECEDENCE))
			printf("(");

		printf(" ");
		printf("%s", (char *) st_byte_array_bytes(node->msg.selector));

	}
	else if (node->msg.precedence == ST_BINARY_PRECEDENCE) {

		if (node->msg.receiver->type == ST_MESSAGE_NODE && node->msg.receiver->msg.precedence == ST_KEYWORD_PRECEDENCE)
			printf("(");

		print_expression(node->msg.receiver);
		if (node->msg.receiver->type == ST_MESSAGE_NODE && node->msg.receiver->msg.precedence == ST_KEYWORD_PRECEDENCE)
			printf(")");

		printf(" ");
		printf("%s", (char *) st_byte_array_bytes(node->msg.selector));
		printf(" ");

		if (node->msg.arguments->type == ST_MESSAGE_NODE &&
			(node->msg.arguments->msg.precedence == ST_BINARY_PRECEDENCE ||
			 node->msg.arguments->msg.precedence == ST_KEYWORD_PRECEDENCE))
			printf("(");

		print_expression(node->msg.arguments);
		if (node->msg.arguments->type == ST_MESSAGE_NODE &&
			(node->msg.arguments->msg.precedence == ST_BINARY_PRECEDENCE ||
			 node->msg.arguments->msg.precedence == ST_KEYWORD_PRECEDENCE))
			printf(")");

	}
	else if (node->msg.precedence == ST_KEYWORD_PRECEDENCE) {

		char *selector = (char *) st_byte_array_bytes(node->msg.selector);
		char **keywords = extract_keywords(selector);
		Node *arguments = node->msg.arguments;

		if (node->msg.receiver->type == ST_MESSAGE_NODE && node->msg.receiver->msg.precedence == ST_KEYWORD_PRECEDENCE)
			printf("(");

		print_expression(node->msg.receiver);
		if (node->msg.receiver->type == ST_MESSAGE_NODE &&
			node->msg.receiver->msg.precedence == ST_KEYWORD_PRECEDENCE)
			printf(")");

		printf(" ");
		for (char **keyword = keywords; *keyword; keyword++) {
			printf("%s: ", *keyword);
			if (node->msg.arguments->type == ST_MESSAGE_NODE &&
				node->msg.arguments->msg.precedence == ST_KEYWORD_PRECEDENCE)
				printf("(");
			print_expression(arguments);
			if (node->msg.arguments->type == ST_MESSAGE_NODE &&
				node->msg.arguments->msg.precedence == ST_KEYWORD_PRECEDENCE)
				printf(")");
			printf(" ");
			arguments = arguments->next;
		}
		st_strfreev(keywords);
	}

	if (node->next) {
		printf(". ");
		print_message(node->next);
	}
}

void print_expression(Node *node) {
	switch (node->type) {
		case ST_LITERAL_NODE:
			node_print_literal(node);
			break;
		case ST_VARIABLE_NODE:
			print_variable(node);
			break;
		case ST_ASSIGN_NODE:
			print_assign(node);
			break;
		case ST_BLOCK_NODE:
			print_block(node);
			break;
		case ST_MESSAGE_NODE:
			print_message(node);
			break;
		case ST_METHOD_NODE:
			break;
		case ST_RETURN_NODE:
			break;
		case ST_CASCADE_NODE:
			break;
	}
}

void st_print_method_node(Node *node) {
	st_assert (node && node->type == ST_METHOD_NODE);
	print_method_node(node);
}

Node *st_node_new(NodeType type) {
	Node *node = st_new0 (Node);
	node->type = type;

	if (node->type == ST_MESSAGE_NODE)
		node->msg.is_statement = false;

	if (node->type == ST_CASCADE_NODE)
		node->cascade.is_statement = false;

	return node;
}

Node *st_node_list_append(Node *list, Node *node) {
	Node *l = list;
	if (list == NULL)
		return node;
	while (l->next)
		l = l->next;
	l->next = node;
	return list;
}

uint st_node_list_length(Node *list) {
	Node *l = list;
	int len = 0;
	for (; l; l = l->next)
		++len;
	return len;
}

Node *st_node_list_at(Node *list, uint index) {
	uint i = 0;
	Node *l;
	for (l = list; i < index; i++)
		l = l->next;
	return l;
}

void st_node_destroy(Node *node) {
	if (node == NULL)
		return;

	switch (node->type) {
		case ST_METHOD_NODE:
			st_node_destroy(node->method.arguments);
			st_node_destroy(node->method.temporaries);
			st_node_destroy(node->method.statements);
			break;
		case ST_BLOCK_NODE:
			st_node_destroy(node->block.arguments);
			st_node_destroy(node->block.temporaries);
			st_node_destroy(node->block.statements);
			break;
		case ST_ASSIGN_NODE:
			st_node_destroy(node->assign.assignee);
			st_node_destroy(node->assign.expression);
			break;
		case ST_RETURN_NODE:
			st_node_destroy(node->retrn.expression);
			break;
		case ST_MESSAGE_NODE:
			st_node_destroy(node->msg.receiver);
			st_node_destroy(node->msg.arguments);
			break;
		case ST_CASCADE_NODE:
			st_node_destroy(node->cascade.receiver);
			st_list_foreach(node->cascade.messages, (st_list_foreach_func) st_node_destroy);
			st_list_destroy(node->cascade.messages);
			break;
		case ST_VARIABLE_NODE:
			st_free(node->variable.name);
			break;
		default:
			break;
	}

	st_node_destroy(node->next);
	st_free(node);
}



