
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "types.h"

typedef struct LexInput LexInput;
#define ST_INPUT_EOF (-1)

LexInput *st_input_new(const char *string);

char st_input_look_ahead(LexInput *input, int i);

uint st_input_get_line(LexInput *input);

uint st_input_get_column(LexInput *input);

void st_input_mark(LexInput *input);

void st_input_rewind(LexInput *input);

void st_input_seek(LexInput *input, uint index);

void st_input_consume(LexInput *input);

uint st_input_size(LexInput *input);

uint st_input_index(LexInput *input);

char *st_input_range(LexInput *input, uint start, uint end);

char *st_input_next_chunk(LexInput *input);

void st_input_destroy(LexInput *input);