
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include <stdio.h>
#include "st-types.h"

struct cell {
	st_oop object;
	st_uint hash;
};

typedef struct st_identity_hashtable {
	struct cell *table;
	st_uint alloc;
	st_uint size;
	st_uint deleted;
	st_uint current_hash;
	
} st_identity_hashtable;

st_uint st_identity_hashtable_hash(st_identity_hashtable *ht, st_oop object);

st_identity_hashtable *st_identity_hashtable_new(void);

void st_identity_hashtable_remove(st_identity_hashtable *ht, st_oop object);

void st_identity_hashtable_rehash_object(st_identity_hashtable *ht, st_oop old, st_oop new);
