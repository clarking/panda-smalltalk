/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "object.h"

typedef struct ContextPart {
	ObjHeader parent;
	Oop sender;
	Oop ip;
	Oop sp;
} ContextPart;

typedef struct MethodContext {
	ContextPart parent;
	Oop method;
	Oop receiver;
	Oop stack[];
} MethodContext;

typedef struct BlockContext {
	ContextPart parent;
	Oop initial_ip;
	Oop argcount;
	Oop home;
	Oop stack[];
} BlockContext;

#define ContextPart(oop)       (( ContextPart *)   st_detag_pointer (oop))
#define MethodContext(oop)     (( MethodContext *) st_detag_pointer (oop))
#define BlockContext(oop)      (( BlockContext *)  st_detag_pointer (oop))

#define ContextPart_SENDER(oop)  (ContextPart (oop)->sender)
#define ContextPart_IP(oop)      (ContextPart (oop)->ip)
#define ContextPart_SP(oop)      (ContextPart (oop)->sp)

#define MethodContext_METHOD(oop)   (MethodContext (oop)->method)
#define MethodContext_RECEIVER(oop) (MethodContext (oop)->receiver)
#define MethodContext_STACK(oop)    (MethodContext (oop)->stack)

#define BlockContext_INITIALIP(oop) (BlockContext (oop)->initial_ip)
#define BlockContext_ARGCOUNT(oop)  (BlockContext (oop)->argcount)
#define BlockContext_HOME(oop)      (BlockContext (oop)->home)
#define BlockContext_STACK(oop)     (BlockContext (oop)->stack)

