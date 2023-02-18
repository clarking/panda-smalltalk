
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "st-types.h"

typedef struct st_input st_input;
#define ST_INPUT_EOF -1

st_input *st_input_new(const char *string);

char st_input_look_ahead(st_input *input, int i);

st_uint st_input_get_line(st_input *input);

st_uint st_input_get_column(st_input *input);

void st_input_mark(st_input *input);

void st_input_rewind(st_input *input);

void st_input_seek(st_input *input, st_uint index);

void st_input_consume(st_input *input);

st_uint st_input_size(st_input *input);

st_uint st_input_index(st_input *input);

char *st_input_range(st_input *input, st_uint start, st_uint end);

char *st_input_next_chunk(st_input *input);

void st_input_destroy(st_input *input);