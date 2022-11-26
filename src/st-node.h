
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"
#include "st-utils.h"

typedef enum {
	ST_METHOD_NODE,
	ST_BLOCK_NODE,
	ST_VARIABLE_NODE,
	ST_ASSIGN_NODE,
	ST_RETURN_NODE,
	ST_MESSAGE_NODE,
	ST_CASCADE_NODE,
	ST_LITERAL_NODE,
	
} st_node_type;

typedef enum {
	ST_UNARY_PRECEDENCE,
	ST_BINARY_PRECEDENCE,
	ST_KEYWORD_PRECEDENCE,
	
} STMessagePrecedence;

typedef struct st_node st_node;

struct st_node {
	st_node_type type;
	int line;
	st_node *next;
	
	union {
		
		struct {
			STMessagePrecedence precedence;
			int primitive;
			st_oop selector;
			st_node *statements;
			st_node *temporaries;
			st_node *arguments;
			
		} method;
		
		struct {
			STMessagePrecedence precedence;
			bool is_statement;
			st_oop selector;
			st_node *receiver;
			st_node *arguments;
			
			bool super_send;
			
		} msg;
		
		struct {
			char *name;
			
		} variable;
		
		struct {
			st_oop value;
			
		} literal;
		
		struct {
			st_node *assignee;
			st_node *expression;
			
		} assign;
		
		struct {
			st_node *expression;
			
		} retrn;
		
		struct {
			st_node *statements;
			st_node *temporaries;
			st_node *arguments;
			
		} block;
		
		struct {
			st_node *receiver;
			st_list *messages;
			bool is_statement;
			
		} cascade;
		
	};
	
};

st_node *st_node_new(st_node_type type);

st_node *st_node_list_append(st_node *list, st_node *node);

st_node *st_node_list_at(st_node *list, st_uint index);

st_uint st_node_list_length(st_node *list);

void st_print_method_node(st_node *method);

void st_node_destroy(st_node *node);
