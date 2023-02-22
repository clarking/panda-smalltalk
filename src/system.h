
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"

uint st_system_pagesize(void);

void *st_system_reserve_memory(void *addr, uint size);

void *st_system_commit_memory(void *addr, uint size);

void *st_system_decommit_memory(void *addr, uint size);

void st_system_release_memory(void *addr, uint size);

