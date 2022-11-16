/*
 * st-input.h
 *
 * Copyright (C) 2008 Vincent Geddes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#ifndef __ST_INPUT_H__
#define __ST_INPUT_H__

#include <st-types.h>

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
#endif /* __ST_INPUT_H__ */