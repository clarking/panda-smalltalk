
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "st-identity-hashtable.h"
#include "st-utils.h"
#include "st-dictionary.h"
#include "st-universe.h"

// power of 2
#define INITIAL_CAPACITY 256

st_identity_hashtable *st_identity_hashtable_new(void) {
	
	st_identity_hashtable *ht;
	ht = st_new (st_identity_hashtable);
	ht->table = st_malloc0(sizeof(struct st_cell) * INITIAL_CAPACITY);
	ht->alloc = INITIAL_CAPACITY;
	ht->size = 0;
	ht->deleted = 0;
	ht->current_hash = 0;
	return ht;
}

// find an object which may already be stored somewhere in table
static st_uint identity_hashtable_find(st_identity_hashtable *ht, st_oop object) {
	st_uint mask, i;
	mask = ht->alloc - 1;
	i = (st_detag_pointer(object) - memory->start) & mask;
	
	while (true) {
		if (ht->table[i].object == 0 || object == ht->table[i].object)
			return i;
		i = (i + ST_ADVANCE_SIZE) & mask;
	}
}

// find a place to insert object
static st_uint identity_hashtable_find_available_cell(st_identity_hashtable *ht, st_oop object) {
	st_uint mask, i;
	
	mask = ht->alloc - 1;
	i = (st_detag_pointer(object) - memory->start) & mask;
	
	while (true) {
		if (ht->table[i].object == 0 || ht->table[i].object == (st_oop) ht)
			return i;
		i = (i + ST_ADVANCE_SIZE) & mask;
	}
}

static void
identity_hashtable_check_grow(st_identity_hashtable *ht) {
	st_uint alloc, index;
	struct st_cell *table;
	
	// ensure table is at least half-full
	if ((ht->size + ht->deleted) * 2 <= ht->alloc)
		return;
	
	alloc = ht->alloc;
	table = ht->table;
	ht->alloc *= 2;
	ht->deleted = 0;
	ht->table = st_malloc0(sizeof(struct st_cell) * ht->alloc);
	
	for (st_uint i = 0; i <= alloc; i++) {
		if (table[i].object != 0 && (table[i].object != (st_oop) ht)) {
			index = identity_hashtable_find_available_cell(ht, table[i].object);
			ht->table[index].object = table[i].object;
			ht->table[index].hash = table[i].hash;
		}
	}
	
	st_free(table);
}

void st_identity_hashtable_remove(st_identity_hashtable *ht, st_oop object) {
	
	st_uint index;
	index = identity_hashtable_find(ht, object);
	if (ht->table[index].object != 0) {
		ht->table[index].object = (st_oop) ht;
		ht->table[index].hash = 0;
		ht->size--;
		ht->deleted++;
		return;
	}
	
	st_assert_not_reached ();
}

st_uint st_identity_hashtable_hash(st_identity_hashtable *ht, st_oop object) {
	// assigns an identity hash for an object
	st_uint index;
	index = identity_hashtable_find(ht, object);
	if (ht->table[index].object == 0) {
		ht->size++;
		ht->table[index].object = object;
		ht->table[index].hash = ht->current_hash++;
		identity_hashtable_check_grow(ht);
	}
	
	return ht->table[index].hash;
}

void st_identity_hashtable_rehash_object(st_identity_hashtable *ht, st_oop old, st_oop new) {
	st_uint hash, index;
	
	index = identity_hashtable_find(ht, old);
	st_assert (ht->table[index].object != 0);
	
	hash = ht->table[index].hash;
	ht->table[index].object = (st_oop) ht;
	ht->table[index].hash = 0;
	ht->deleted++;
	
	index = identity_hashtable_find_available_cell(ht, new);
	if (ht->table[index].object == (st_oop) ht)
		ht->deleted--;
	
	ht->table[index].object = new;
	ht->table[index].hash = hash;
}
