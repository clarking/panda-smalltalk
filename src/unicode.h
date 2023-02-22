
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <unistd.h>
#include "types.h"

void st_unicode_init(void);

void st_unicode_canonical_decomposition(const unichar *in, int inlen, unichar **out, int *outlen);

#define st_utf8_skip(c) (((0xE5000000 >> (((c) >> 3) & 0xFE)) & 3) + 1)
#define st_utf8_next_char(p) (const char *)((p) + st_utf8_skip (*(const char *)(p)))

int st_utf8_strlen(const char *string);

unichar st_utf8_get_unichar(const char *p);

bool st_utf8_validate(const char *string, ssize_t max_len);

int st_unichar_to_utf8(unichar ch, char *outbuf);

const char *st_utf8_offset_to_pointer(const char *string, uint offset);

unichar *st_utf8_to_ucs4(const char *string);

