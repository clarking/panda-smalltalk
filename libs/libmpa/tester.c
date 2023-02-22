/* **************************************************************************
 * Copyright (c) 2007, David Waite
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are
 *  met:
 * 
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice
 *   this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * * The name(s) of contributors may not be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ************************************************************************* */
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "PtrArray.h"

int main(int argc, char **argv);

void test_new() {
	PtrArray p1 = PtrArray_new(10);
	assert(p1 != NULL);
	assert(p1->size >= 10);
	assert(p1->array != NULL);
	assert(p1->length == 0);
	PtrArray_free(p1);
}

void test_new_oom() {
	fprintf(stderr, "Your malloc implementation may report an (ignorable) "
	                "error for this test\n");
	PtrArray p1 = PtrArray_new(1000000000);
	assert (p1 == NULL);
}

void test_free() {
	PtrArray p1 = PtrArray_new(1);
	PtrArray_free(p1);
	PtrArray p2 = NULL;
	PtrArray_free(p2);
}

void test_new_zero() {
	PtrArray p1 = PtrArray_new(0);
	assert(p1 != NULL);
	assert(p1->size == 0);
	assert(p1->array == NULL);
	assert(p1->length == 0);
	PtrArray_free(p1);
}

static PtrArray setup_data() {
	PtrArray p1 = PtrArray_new(4);
	PtrArray_append(p1, &malloc);
	PtrArray_append(p1, &realloc);
	PtrArray_append(p1, &free);
	PtrArray_append(p1, &main);
	return p1;
}

void test_remove_ordered() {
	PtrArray p1 = setup_data();
	void *value =
			PtrArray_remove_ordered(p1, &realloc);
	assert(value == &realloc);
	assert(PtrArray_get_index(p1, 0) == &malloc);
	assert(PtrArray_get_index(p1, 1) == &free);
	assert(PtrArray_get_index(p1, 2) == &main);
	assert(PtrArray_length(p1) == 3);
	
	value =
			PtrArray_remove_ordered(p1, &malloc);
	assert(value == &malloc);
	assert(PtrArray_get_index(p1, 0) == &free);
	assert(PtrArray_get_index(p1, 1) == &main);
	assert(PtrArray_length(p1) == 2);
	
	value =
			PtrArray_remove_ordered(p1, &main);
	assert(value == &main);
	assert(PtrArray_get_index(p1, 0) == &free);
	assert(PtrArray_length(p1) == 1);
	
	value =
			PtrArray_remove_ordered(p1, &free);
	assert (value == &free);
	assert(PtrArray_length(p1) == 0);
}

void test_remove_ordered_nonexistant() {
	PtrArray p1 = setup_data();
	void *value = PtrArray_remove_ordered(p1, "some random data");
	assert(value == NULL);
}

void test_remove_index_ordered() {
	PtrArray p1 = setup_data();
	void *value =
			PtrArray_remove_index_ordered(p1, 1);
	assert(value == &realloc);
	assert(PtrArray_get_index(p1, 0) == &malloc);
	assert(PtrArray_get_index(p1, 1) == &free);
	assert(PtrArray_get_index(p1, 2) == &main);
	assert(PtrArray_length(p1) == 3);
	
	value =
			PtrArray_remove_index_ordered(p1, 0);
	assert(value == &malloc);
	assert(PtrArray_get_index(p1, 0) == &free);
	assert(PtrArray_get_index(p1, 1) == &main);
	assert(PtrArray_length(p1) == 2);
	
	value =
			PtrArray_remove_index_ordered(p1, 1);
	assert(value == &main);
	assert(PtrArray_get_index(p1, 0) == &free);
	assert(PtrArray_length(p1) == 1);
	
	value =
			PtrArray_remove_index_ordered(p1, 0);
	assert (value == &free);
	assert(PtrArray_length(p1) == 0);
}

void test_remove_fast() {
	PtrArray p1 = setup_data();
	void *value =
			PtrArray_remove_fast(p1, &realloc);
	assert(value == &realloc);
	assert(PtrArray_contains(p1, &malloc));
	assert(PtrArray_contains(p1, &free));
	assert(PtrArray_contains(p1, &main));
	assert(PtrArray_length(p1) == 3);
	
	value =
			PtrArray_remove_fast(p1, &malloc);
	assert(value == &malloc);
	assert(PtrArray_contains(p1, &free));
	assert(PtrArray_contains(p1, &main));
	assert(PtrArray_length(p1) == 2);
	
	value =
			PtrArray_remove_fast(p1, &main);
	assert(value == &main);
	assert(PtrArray_contains(p1, &free));
	assert(PtrArray_length(p1) == 1);
	
	value =
			PtrArray_remove_fast(p1, &free);
	assert (value == &free);
	assert(PtrArray_length(p1) == 0);
}

void test_remove_fast_nonexistant() {
	PtrArray p1 = setup_data();
	void *value = PtrArray_remove_fast(p1, "some random data");
	assert(value == NULL);
}

void test_remove_index_fast() {
	PtrArray p1 = setup_data();
	void *value =
			PtrArray_remove_index_fast(p1, 1);
	assert(value == &realloc);
	assert(PtrArray_contains(p1, &malloc));
	assert(PtrArray_contains(p1, &free));
	assert(PtrArray_contains(p1, &main));
	assert(PtrArray_length(p1) == 3);
	
	value =
			PtrArray_remove_index_fast(p1, 0);
	assert(value == &malloc);
	assert(PtrArray_contains(p1, &free));
	assert(PtrArray_contains(p1, &main));
	assert(PtrArray_length(p1) == 2);
	
	value =
			PtrArray_remove_index_fast(p1, 1);
	assert(value == &main);
	assert(PtrArray_contains(p1, &free));
	assert(PtrArray_length(p1) == 1);
	
	value =
			PtrArray_remove_index_fast(p1, 0);
	assert (value == &free);
	assert(PtrArray_length(p1) == 0);
}

void test_get_set_index() {
	PtrArray p1 = setup_data();
	assert(PtrArray_get_index(p1, 0) == &malloc);
	PtrArray_set_index(p1, 0, &realloc);
	assert(PtrArray_get_index(p1, 0) == &realloc);
}

void test_array_contains() {
	PtrArray p1 = setup_data();
	assert(PtrArray_contains(p1, &malloc));
	assert(!PtrArray_contains(p1, &test_array_contains));
}

void test_array_length() {
	PtrArray p1 = setup_data();
	assert(PtrArray_length(p1) == 4);
	PtrArray_remove_index_fast(p1, 0);
	assert(PtrArray_length(p1) == 3);
}

void test_array_append() {
	int i;
	PtrArray p1 = PtrArray_new(0);
	for (i = 0; i < 1000; i++)
		PtrArray_append(p1, &test_array_append);
	assert(PtrArray_length(p1) == 1000);
	
	for (i = 0; i < 1000; i++)
		assert(p1->array[i] == &test_array_append);
}

void test_array_clear() {
	PtrArray p1 = setup_data();
	PtrArray_clear(p1);
	assert(PtrArray_length(p1) == 0);
	PtrArray_append(p1, 0);
	assert(PtrArray_length(p1) == 1);
}

int main(int argc, char **argv) {
	test_new();
	test_new_oom();
	test_new_zero();
	
	test_remove_ordered();
	test_remove_ordered_nonexistant();
	test_remove_index_ordered();
	
	test_remove_fast();
	test_remove_fast_nonexistant();
	test_remove_index_fast();
	
	test_get_set_index();
	test_array_contains();
	test_array_length();
	
	test_array_append();
	test_array_clear();
	return 0;
}
