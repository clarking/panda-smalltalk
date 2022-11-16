/*
 * st-small-integer.h
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

#ifndef __ST_SMALL_INTEGER_H__
#define __ST_SMALL_INTEGER_H__

#include <st-types.h>

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

#endif /* __ST_SMALL_INTEGER_H__ */
