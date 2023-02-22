
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include "input.h"
#include "utils.h"

static char *filter_double_bangs(const char *chunk) {
	uint size, i = 0, count = 0;
	const char *p = chunk;
	char *buf;
	
	size = strlen(chunk);
	if (size < 2)
		return st_strdup(chunk);
	
	// count number of redundant bangs
	while (p[0] && p[1]) {
		if (ST_UNLIKELY(p[0] == '!' && p[1] == '!'))
			count++;
		p++;
	}
	
	buf = st_malloc(size - count + 1);
	
	// copy over text skipping over redundant bangs
	p = chunk;
	while (*p) {
		if (*p == '!')
			p++;
		buf[i++] = *p;
		p++;
	}
	
	buf[i] = 0;
	return buf;
}

char *st_input_next_chunk(LexInput *input) {
	char *chunk_filtered, *chunk = NULL;
	uint start;
	
	start = st_input_index(input);
	while (st_input_look_ahead(input, 1) != ST_INPUT_EOF) {
		
		if (st_input_look_ahead(input, 1) != '!') {
			st_input_consume(input);
			continue;
		}
		
		// skip past doubled bangs
		if (st_input_look_ahead(input, 1) == '!'
		    && st_input_look_ahead(input, 2) == '!') {
			st_input_consume(input);
			st_input_consume(input);
			continue;
		}
		
		chunk = st_input_range(input, start, st_input_index(input));
		chunk_filtered = filter_double_bangs(chunk);
		st_input_consume(input);
		st_free(chunk);
		return chunk_filtered;
	}
	return NULL;
}

void st_input_destroy(LexInput *input) {
	st_assert (input != NULL);
	st_free(input->text);
	st_free(input);
}

uint st_input_get_line(LexInput *input) {
	st_assert (input != NULL);
	return input->line;
}

uint st_input_get_column(LexInput *input) {
	st_assert (input != NULL);
	return input->column;
}

char st_input_look_ahead(LexInput *input, int i) {
	st_assert (input != NULL);
	if (ST_UNLIKELY(i == 0))
		return 0x0000;
	
	if (ST_UNLIKELY(i < 0)) {
		i++;
		if ((input->p + i - 1) < 0)
			return ST_INPUT_EOF;
	}
	
	if ((input->p + i - 1) >= input->n)
		return ST_INPUT_EOF;
	
	return input->text[input->p + i - 1];
}

void st_input_mark(LexInput *input) {
	st_assert (input != NULL);
	input->InputMarker.p = input->p;
	input->InputMarker.line = input->line;
	input->InputMarker.column = input->column;
}

void st_input_rewind(LexInput *input) {
	st_assert (input != NULL);
	st_input_seek(input, input->InputMarker.p);
	input->line = input->InputMarker.line;
	input->column = input->InputMarker.column;
}

void st_input_seek(LexInput *input, uint index) {
	st_assert (input != NULL);
	if (index <= input->p)
		input->p = index;
	
	while (input->p < index)
		st_input_consume(input);
}

void st_input_consume(LexInput *input) {
	st_assert (input != NULL);
	if (input->p < input->n) {
		input->column++;
		
		/* 0x000A is newline */
		if (input->text[input->p] == 0x000A) {
			input->line++;
			input->column = 1;
		}
		input->p++;
	}
}

uint st_input_size(LexInput *input) {
	st_assert (input != NULL);
	return input->n;
}

char *st_input_range(LexInput *input, uint start, uint end) {
	char *buf;
	uint len;
	
	st_assert ((end - start) >= 0);
	len = end - start;
	buf = st_malloc(len + 1);
	memcpy(buf, input->text + start, len);
	buf[len] = 0;
	return buf;
}

uint st_input_index(LexInput *input) {
	st_assert (input != NULL);
	return input->p;
}

static void initialize_state(LexInput *input, const char *string) {
	input->text = (char *) string;
	input->n = strlen(string);
	input->line = 1;
	input->column = 1;
	input->InputMarker.p = 0;
	input->InputMarker.line = 0;
	input->InputMarker.column = 0;
}

LexInput *st_input_new(const char *string) {
	LexInput *input;
	st_assert (string != NULL);
	input = st_new0 (LexInput);
	initialize_state(input, strdup(string));
	return input;
}