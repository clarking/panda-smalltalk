
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

static inline st_oop
st_smi_new(int num) {
	return (((st_oop) num) << ST_TAG_SIZE) + ST_SMI_TAG;
}

static inline int
st_smi_value(st_oop smi) {
	return ((int) smi) >> ST_TAG_SIZE;
}

static inline st_oop
st_smi_increment(st_oop smi) {
	return st_smi_new(st_smi_value(smi) + 1);
}

static inline st_oop
st_smi_decrement(st_oop smi) {
	return st_smi_new(st_smi_value(smi) - 1);
}

static inline bool
st_smi_equal(st_oop m, st_oop n) {
	return m == n;
}

static inline int
st_smi_hash(st_oop smi) {
	return st_smi_value(smi);
}

