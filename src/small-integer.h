
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"

static inline Oop st_smi_new(int num) {
	return (((Oop) num) << ST_TAG_SIZE) + ST_SMI_TAG;
}

static inline int st_smi_value(Oop smi) {
	return ((int) smi) >> ST_TAG_SIZE;
}

static inline Oop st_smi_increment(Oop smi) {
	return st_smi_new(st_smi_value(smi) + 1);
}

static inline Oop st_smi_decrement(Oop smi) {
	return st_smi_new(st_smi_value(smi) - 1);
}

static inline bool st_smi_equal(Oop m, Oop n) {
	return m == n;
}

static inline int st_smi_hash(Oop smi) {
	return st_smi_value(smi);
}

