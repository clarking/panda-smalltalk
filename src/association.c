
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#include "st-association.h"
#include "st-behavior.h"

st_uint st_association_hash(st_oop object) {
	return st_object_hash(ST_ASSOCIATION (object)->key) ^ st_object_hash(ST_ASSOCIATION (object)->value);
}

bool st_association_equal(st_oop object, st_oop other) {
	struct st_association *m, *n;
	
	if (st_object_class(other) != ST_ASSOCIATION_CLASS)
		return false;
	
	m = ST_ASSOCIATION (object);
	n = ST_ASSOCIATION (other);
	
	return st_object_equal(m->key, n->key) && st_object_equal(m->value, n->value);
}

st_oop st_association_new(st_oop key, st_oop value) {
	st_oop assoc = st_object_new(ST_ASSOCIATION_CLASS);
	
	ST_ASSOCIATION_KEY (assoc) = key;
	ST_ASSOCIATION_VALUE (assoc) = value;
	
	return assoc;
}
