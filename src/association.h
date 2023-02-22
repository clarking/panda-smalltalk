
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include "types.h"
#include "object.h"


#define ST_ASSOCIATION(oop)       ((Association *) st_detag_pointer (oop))
#define ST_ASSOCIATION_KEY(oop)   (ST_ASSOCIATION(oop)->key)
#define ST_ASSOCIATION_VALUE(oop) (ST_ASSOCIATION(oop)->value)

Oop st_association_new(Oop key, Oop value);

st_uint st_association_hash(Oop object);

bool st_association_equal(Oop object, Oop other);

