
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#include "association.h"
#include "behavior.h"

uint st_association_hash(Oop object) {
	return st_object_hash(ST_ASSOCIATION (object)->key) ^ st_object_hash(ST_ASSOCIATION (object)->value);
}

bool st_association_equal(Oop object, Oop other) {
	Association *m, *n;

	if (st_object_class(other) != ST_ASSOCIATION_CLASS)
		return false;

	m = ST_ASSOCIATION (object);
	n = ST_ASSOCIATION (other);
	return st_object_equal(m->key, n->key) && st_object_equal(m->value, n->value);
}

Oop st_association_new(Oop key, Oop value) {
	Oop assoc = st_object_new(ST_ASSOCIATION_CLASS);

	ST_ASSOCIATION_KEY (assoc) = key;
	ST_ASSOCIATION_VALUE (assoc) = value;
	return assoc;
}
