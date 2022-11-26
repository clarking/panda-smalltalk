
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"
#include "st-object.h"
#include "st-memory.h"
#include "st-small-integer.h"
#include "st-array.h"
#include "st-float.h"
#include "st-large-integer.h"

#define ST_BEHAVIOR(oop)  ((struct st_behavior *)  st_detag_pointer (oop))
#define ST_CLASS(oop)     ((struct st_class *)     st_detag_pointer (oop))
#define ST_METACLASS(oop) ((struct st_metaclass *) st_detag_pointer (oop))

struct st_behavior {
	struct st_header header;
	st_oop format;
	st_oop superclass;
	st_oop instance_size;
	st_oop method_dictionary;
	st_oop instance_variables;
};

struct st_class {
	struct st_behavior parent;
	st_oop name;
};

struct st_metaclass {
	struct st_behavior parent;
	
	st_oop instance_class;
};

#define ST_BEHAVIOR_FORMAT(oop)             (ST_BEHAVIOR (oop)->format)
#define ST_BEHAVIOR_SUPERCLASS(oop)         (ST_BEHAVIOR (oop)->superclass)
#define ST_BEHAVIOR_INSTANCE_SIZE(oop)      (ST_BEHAVIOR (oop)->instance_size)
#define ST_BEHAVIOR_METHOD_DICTIONARY(oop)  (ST_BEHAVIOR (oop)->method_dictionary)
#define ST_BEHAVIOR_INSTANCE_VARIABLES(oop) (ST_BEHAVIOR (oop)->instance_variables)
#define ST_CLASS_NAME(oop)                  (ST_CLASS (oop)->name)
#define ST_METACLASS_INSTANCE_CLASS(oop)    (ST_METACLASS (oop)->instance_class)

st_oop st_object_new(st_oop class);

st_oop st_object_new_arrayed(st_oop class, int size);

st_list *st_behavior_all_instance_variables(st_oop class);

