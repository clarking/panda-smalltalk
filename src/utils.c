
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include "utils.h"

int vasprintf(char **strp, const char *fmt, va_list ap);

extern char *program_invocation_short_name;

extern bool st_get_verbose_mode(void) ST_GNUC_PURE;

void *st_malloc(size_t size) {
	void *ptr;

	ptr = malloc(size);
	if (ST_UNLIKELY(ptr == NULL))
		abort();

	return ptr;
}

void *st_malloc0(size_t size) {
	void *ptr;

	ptr = calloc(1, size);
	if (ST_UNLIKELY(ptr == NULL))
		abort();

	return ptr;
}

void *st_realloc(void *mem, size_t size) {
	void *ptr;

	ptr = realloc(mem, size);
	if (ST_UNLIKELY(ptr == NULL))
		abort();

	return ptr;
}

void st_free(void *mem) {
	if (ST_UNLIKELY(mem != NULL))
		free(mem);
}

bool st_file_get_contents(const char *filename, char **buffer) {

	struct stat info;
	long int total;
	ssize_t count;
	char *temp;
	int fd;

	st_assert (filename != NULL);
	*buffer = NULL;

	if (stat(filename, &info) != 0) {
		if (strlen(filename) > 0)
			fprintf(stderr, "%s: error: %s: %s\n", program_invocation_short_name, filename, strerror(errno));
		else
			fprintf(stderr, "%s: %s\n", program_invocation_short_name, strerror(errno));
		return false;
	}

	if (!S_ISREG (info.st_mode)) {
		fprintf(stderr, "%s: error: %s: Not a regular file\n", program_invocation_short_name, filename);
		return false;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: error: %s: %s\n", program_invocation_short_name, filename, strerror(errno));
		return false;
	}

	total = 0;
	temp = st_malloc(info.st_size + 1);
	while (total < info.st_size) {
		count = read(fd, temp + total, info.st_size - total);
		if (count < 0) {
			fprintf(stderr, "%s: error: %s: %s\n", program_invocation_short_name, filename, strerror(errno));
			st_free(temp);
			close(fd);
			return false;
		}
		total += count;
	}

	close(fd);

	temp[info.st_size] = 0;
	*buffer = temp;

	return true;
}

/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc. */
char *st_strdup(const char *string) {
	size_t size;
	char *copy;

	st_assert (string != NULL);

	size = strlen(string);
	copy = st_malloc(size + 1);
	strcpy(copy, string);

	return copy;
}

/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc. */
char *st_strdup_vprintf(const char *format, va_list args) {
	int n;
	char *ret;

	n = vasprintf(&ret, format, args);
	if (n == -1)
		return NULL;

	return ret;
}

/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc. */
char *st_strdup_printf(const char *format, ...) {
	char *ret;
	va_list args;
	int n;

	va_start (args, format);
	n = vasprintf(&ret, format, args);
	va_end (args);
	if (n == -1)
		return NULL;

	return ret;
}

/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc. */
char *st_strconcat(const char *first, ...) {
	va_list args;
	size_t total = 0;
	char *s, *ret;

	total += strlen(first);
	va_start (args, first);
	for (s = va_arg (args, char *); s != NULL; s = va_arg(args, char *)) {
		total += strlen(s);
	}
	va_end (args);

	ret = st_malloc(total + 1);
	if (ret == NULL)
		return NULL;

	ret[total] = 0;
	strcpy(ret, first);
	va_start (args, first);
	for (s = va_arg (args, char *); s != NULL; s = va_arg(args, char *)) {
		strcat(ret, s);
	}
	va_end (args);

	return ret;
}

double st_timespec_to_double_seconds(struct timespec *interval) {
	/* print in seconds */
	return interval->tv_sec + (interval->tv_nsec / 1.e9);
}

void st_timespec_difference(struct timespec *start, struct timespec *end, struct timespec *diff) {
	if ((end->tv_nsec - start->tv_nsec) < 0) {
		diff->tv_sec = end->tv_sec - start->tv_sec - 1;
		diff->tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
	}
	else {
		diff->tv_sec = end->tv_sec - start->tv_sec;
		diff->tv_nsec = end->tv_nsec - start->tv_nsec;
	}
}

void st_timespec_add(struct timespec *t1, struct timespec *t2, struct timespec *result) {
	if ((t1->tv_nsec + t2->tv_nsec) >= 1000000000) {
		result->tv_sec = t1->tv_sec + t2->tv_sec + 1;
		result->tv_nsec = t1->tv_nsec + t2->tv_nsec - 1000000000;
	}
	else {
		result->tv_sec = t1->tv_sec + t2->tv_sec;
		result->tv_nsec = t1->tv_nsec + t2->tv_nsec;
	}
}

List *st_list_append(List *list, void *data) {
	List *new_list, *l;

	new_list = st_new (List);
	new_list->data = data;
	new_list->next = NULL;

	if (list == NULL)
		return new_list;

	l = list;
	while (l->next)
		l = l->next;

	l->next = new_list;

	return list;
}

List *st_list_prepend(List *list, void *data) {
	List *new_list;

	new_list = st_new (List);
	new_list->data = data;
	new_list->next = list;

	return new_list;
}

List *st_list_reverse(List *list) {
	List *next, *prev = NULL;

	while (list) {
		next = list->next;
		list->next = prev;
		prev = list;
		list = next;
	}

	return prev;
}

List *st_list_concat(List *list1, List *list2) {
	List *l;

	if (list1 == NULL)
		return list2;

	if (list2 == NULL)
		return list1;

	l = list1;
	while (l->next)
		l = l->next;

	l->next = list2;
	return list1;
}

uint st_list_length(List *list) {
	List *l = list;
	uint len = 0;

	for (; l; l = l->next)
		++len;

	return len;
}

void st_list_foreach(List *list, st_list_foreach_func func) {
	for (List *l = list; l; l = l->next)
		func(l->data);
}

void st_list_destroy(List *list) {
	List *next, *current;

	current = list;

	while (current) {
		next = current->next;
		st_free(current);
		current = next;
	}
}

/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc.  */
static void add_to_vector(char ***vector, int size, char *token) {
	*vector = *vector == NULL ?
			  (char **) st_malloc(2 * sizeof(*vector)) :
			  (char **) st_realloc(*vector, (size + 1) * sizeof(*vector));

	(*vector)[size - 1] = token;
}

/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc. */
char **st_strsplit(const char *string, const char *delimiter, int max_tokens) {
	const char *c;
	char *token, **vector;
	int size = 1;

	if (strncmp(string, delimiter, strlen(delimiter)) == 0) {
		vector = (char **) st_malloc(2 * sizeof(vector));
		vector[0] = st_strdup("");
		size++;
		string += strlen(delimiter);
	}
	else {
		vector = NULL;
	}

	while (*string && !(max_tokens > 0 && size >= max_tokens)) {
		c = string;
		if (strncmp(string, delimiter, strlen(delimiter)) == 0) {
			token = st_strdup("");
			string += strlen(delimiter);
		}
		else {
			while (*string && strncmp(string, delimiter, strlen(delimiter)) != 0) {
				string++;
			}

			if (*string) {
				size_t toklen = (string - c);
				token = st_strndup(c, toklen);

				/* Need to leave a trailing empty
				 * token if the delimiter is the last
				 * part of the string
				 */
				if (strcmp(string, delimiter) != 0) {
					string += strlen(delimiter);
				}
			}
			else {
				token = st_strdup(c);
			}
		}

		add_to_vector(&vector, size, token);
		size++;
	}

	if (*string) {
		/* Add the rest of the string as the last element */
		add_to_vector(&vector, size, st_strdup(string));
		size++;
	}

	if (vector == NULL) {
		vector = (char **) st_malloc(2 * sizeof(vector));
		vector[0] = NULL;
	}
	else if (size > 0) {
		vector[size - 1] = NULL;
	}

	return vector;
}

/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc.*/
void st_strfreev(char **str_array) {
	char **orig = str_array;
	if (str_array == NULL)
		return;
	while (*str_array != NULL) {
		st_free(*str_array);
		str_array++;
	}
	st_free(orig);
}


/* Derived from eglib (part of Mono) Copyright (C) 2006 Novell, Inc.*/
/* This is not a macro, because I dont want to put _GNU_SOURCE in the glib.h header */
char *st_strndup(const char *str, size_t n) {
#ifdef HAVE_STRNDUP
	return strndup (str, n);
#else
	if (str) {
		char *retval = malloc(n + 1);
		if (retval) {
			strncpy(retval, str, n)[n] = 0;
		}
		return retval;
	}
	return NULL;
#endif
}

uint st_string_hash(const char *string) {
	const signed char *p = (signed const char *) string;
	uint h = *p;

	if (*p == 0)
		return h;

	while (*p) {
		h = (h << 5) - h + *p++;
	}

	return h;
}

void st_log(const char *restrict domain, const char *restrict format, ...) {
	char *fmt;
	va_list args;

	st_assert (domain != NULL);
	if (!st_get_verbose_mode())
		return;

	fmt = st_strconcat("** ", domain, ": ", format, "\n", NULL);

	va_start (args, format);
	vfprintf(stderr, fmt, args);
	va_end (args);
	st_free(fmt);
}
