
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#pragma once

#include <stdio.h>
#include "types.h"

uint st_identity_hashtable_hash(IdentityHashTable *ht, Oop object);

IdentityHashTable *st_identity_hashtable_new(void);

void st_identity_hashtable_remove(IdentityHashTable *ht, Oop object);

void st_identity_hashtable_rehash_object(IdentityHashTable *ht, Oop old, Oop new);
