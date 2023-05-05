
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include "identity-hashtable.h"
#include "utils.h"
#include "dictionary.h"
#include "universe.h"

// power of 2
#define INITIAL_CAPACITY 256

IdentityHashTable *st_identity_hashtable_new(void) {

	IdentityHashTable *ht;
	ht = st_new (IdentityHashTable);
	ht->table = st_malloc0(sizeof(Cell) * INITIAL_CAPACITY);
	ht->alloc = INITIAL_CAPACITY;
	ht->size = 0;
	ht->deleted = 0;
	ht->current_hash = 0;
	return ht;
}

// find an object which may already be stored somewhere in table
static uint identity_hashtable_find(IdentityHashTable *ht, Oop object) {
	uint mask, i;
	mask = ht->alloc - 1;
	i = (st_detag_pointer(object) - memory->start) & mask;

	while (true) {
		if (ht->table[i].object == 0 || object == ht->table[i].object)
			return i;
		i = (i + ST_ADVANCE_SIZE) & mask;
	}
}

// find a place to insert object
static uint identity_hashtable_find_available_cell(IdentityHashTable *ht, Oop object) {
	uint mask, i;

	mask = ht->alloc - 1;
	i = (st_detag_pointer(object) - memory->start) & mask;

	while (true) {
		if (ht->table[i].object == 0 || ht->table[i].object == (Oop) ht)
			return i;
		i = (i + ST_ADVANCE_SIZE) & mask;
	}
}

static void identity_hashtable_check_grow(IdentityHashTable *ht) {
	uint alloc, index;
	struct Cell *table;

	// ensure table is at least half-full
	if ((ht->size + ht->deleted) * 2 <= ht->alloc)
		return;

	alloc = ht->alloc;
	table = ht->table;
	ht->alloc *= 2;
	ht->deleted = 0;
	ht->table = st_malloc0(sizeof(Cell) * ht->alloc);

	for (uint i = 0; i <= alloc; i++) {
		if (table[i].object != 0 && (table[i].object != (Oop) ht)) {
			index = identity_hashtable_find_available_cell(ht, table[i].object);
			ht->table[index].object = table[i].object;
			ht->table[index].hash = table[i].hash;
		}
	}

	st_free(table);
}

void st_identity_hashtable_remove(IdentityHashTable *ht, Oop object) {

	uint index;
	index = identity_hashtable_find(ht, object);
	if (ht->table[index].object != 0) {
		ht->table[index].object = (Oop) ht;
		ht->table[index].hash = 0;
		ht->size--;
		ht->deleted++;
		return;
	}

	st_assert_not_reached ();
}

uint st_identity_hashtable_hash(IdentityHashTable *ht, Oop object) {
	// assigns an identity hash for an object
	uint index;
	index = identity_hashtable_find(ht, object);
	if (ht->table[index].object == 0) {
		ht->size++;
		ht->table[index].object = object;
		ht->table[index].hash = ht->current_hash++;
		identity_hashtable_check_grow(ht);
	}

	return ht->table[index].hash;
}

void st_identity_hashtable_rehash_object(IdentityHashTable *ht, Oop old, Oop new) {
	uint hash, index;

	index = identity_hashtable_find(ht, old);
	st_assert (ht->table[index].object != 0);

	hash = ht->table[index].hash;
	ht->table[index].object = (Oop) ht;
	ht->table[index].hash = 0;
	ht->deleted++;

	index = identity_hashtable_find_available_cell(ht, new);
	if (ht->table[index].object == (Oop) ht)
		ht->deleted--;

	ht->table[index].object = new;
	ht->table[index].hash = hash;
}
