
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "utils.h"

void print_variable(Node *node);

void print_object(Oop object);

void print_expression(Node *expression);

void print_tuple(Oop tuple);

void node_print_literal(Node *node);

void print_return(Node *node);

void print_assign(Node *node);

char **extract_keywords(char *selector);

void print_method_node(Node *node);

void print_block(Node *node);

void print_message(Node *node);

Node *st_node_new(NodeType type);

Node *st_node_list_append(Node *list, Node *node);

Node *st_node_list_at(Node *list, uint index);

uint st_node_list_length(Node *list);

void st_print_method_node(Node *method);

void st_node_destroy(Node *node);
