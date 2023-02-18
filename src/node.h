
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"
#include "st-utils.h"

void print_variable(st_node *node);

void print_object(st_oop object);

void print_expression(st_node *expression);

void print_tuple(st_oop tuple);

void node_print_literal(st_node *node);

void print_return(st_node *node);

void print_assign(st_node *node);

char **extract_keywords(char *selector);

void print_method_node(st_node *node);

void print_block(st_node *node);

void print_message(st_node *node);

st_node *st_node_new(st_node_type type);

st_node *st_node_list_append(st_node *list, st_node *node);

st_node *st_node_list_at(st_node *list, st_uint index);

st_uint st_node_list_length(st_node *list);

void st_print_method_node(st_node *method);

void st_node_destroy(st_node *node);
