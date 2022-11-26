
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

st_uint st_system_pagesize(void);

st_pointer st_system_reserve_memory(st_pointer addr, st_uint size);

st_pointer st_system_commit_memory(st_pointer addr, st_uint size);

st_pointer st_system_decommit_memory(st_pointer addr, st_uint size);

void st_system_release_memory(st_pointer addr, st_uint size);

