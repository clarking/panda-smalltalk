/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"
#include "st-object.h"

struct st_context_part {
	struct st_header parent;
	st_oop sender;
	st_oop ip;
	st_oop sp;
	
};

struct st_method_context {
	struct st_context_part parent;
	st_oop method;
	st_oop receiver;
	st_oop stack[];
};

struct st_block_context {
	struct st_context_part parent;
	st_oop initial_ip;
	st_oop argcount;
	st_oop home;
	st_oop stack[];
};

#define ST_CONTEXT_PART(oop)       ((struct st_context_part *)   st_detag_pointer (oop))
#define ST_METHOD_CONTEXT(oop)     ((struct st_method_context *) st_detag_pointer (oop))
#define ST_BLOCK_CONTEXT(oop)      ((struct st_block_context *)  st_detag_pointer (oop))

#define ST_CONTEXT_PART_SENDER(oop)  (ST_CONTEXT_PART (oop)->sender)
#define ST_CONTEXT_PART_IP(oop)      (ST_CONTEXT_PART (oop)->ip)
#define ST_CONTEXT_PART_SP(oop)      (ST_CONTEXT_PART (oop)->sp)

#define ST_METHOD_CONTEXT_METHOD(oop)   (ST_METHOD_CONTEXT (oop)->method)
#define ST_METHOD_CONTEXT_RECEIVER(oop) (ST_METHOD_CONTEXT (oop)->receiver)
#define ST_METHOD_CONTEXT_STACK(oop)    (ST_METHOD_CONTEXT (oop)->stack)

#define ST_BLOCK_CONTEXT_INITIALIP(oop) (ST_BLOCK_CONTEXT (oop)->initial_ip)
#define ST_BLOCK_CONTEXT_ARGCOUNT(oop)  (ST_BLOCK_CONTEXT (oop)->argcount)
#define ST_BLOCK_CONTEXT_HOME(oop)      (ST_BLOCK_CONTEXT (oop)->home)
#define ST_BLOCK_CONTEXT_STACK(oop)     (ST_BLOCK_CONTEXT (oop)->stack)

