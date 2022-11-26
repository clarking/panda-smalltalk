
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include "st-object.h"
#include "st-types.h"

struct st_association {
	struct st_header parent;
	st_oop key;
	st_oop value;
};

st_oop st_association_new(st_oop key, st_oop value);

st_uint st_association_hash(st_oop object);

bool st_association_equal(st_oop object, st_oop other);

#define ST_ASSOCIATION(oop)       ((struct st_association *) st_detag_pointer (oop))
#define ST_ASSOCIATION_KEY(oop)   (ST_ASSOCIATION(oop)->key)
#define ST_ASSOCIATION_VALUE(oop) (ST_ASSOCIATION(oop)->value)

