
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"
#include "object.h"
#include "memory.h"
#include "small-integer.h"
#include "array.h"
#include "float.h"
#include "large-integer.h"

#define ST_BEHAVIOR(oop)  ((Behavior *)  st_detag_pointer (oop))
#define ST_CLASS(oop)     ((Class *)     st_detag_pointer (oop))
#define ST_METACLASS(oop) ((MetaClass *) st_detag_pointer (oop))

#define ST_BEHAVIOR_FORMAT(oop)             (ST_BEHAVIOR (oop)->format)
#define ST_BEHAVIOR_SUPERCLASS(oop)         (ST_BEHAVIOR (oop)->superclass)
#define ST_BEHAVIOR_INSTANCE_SIZE(oop)      (ST_BEHAVIOR (oop)->instance_size)
#define ST_BEHAVIOR_METHOD_DICTIONARY(oop)  (ST_BEHAVIOR (oop)->method_dictionary)
#define ST_BEHAVIOR_INSTANCE_VARIABLES(oop) (ST_BEHAVIOR (oop)->instance_variables)
#define ST_CLASS_NAME(oop)                  (ST_CLASS (oop)->name)
#define ST_METACLASS_INSTANCE_CLASS(oop)    (ST_METACLASS (oop)->instance_class)

Oop st_object_new(Oop class);

Oop st_object_new_arrayed(Oop class, uint size);

List *st_behavior_all_instance_variables(Oop class);
