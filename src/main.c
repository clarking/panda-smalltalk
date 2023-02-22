/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>

#include "types.h"
#include "compiler.h"
#include "machine.h"
#include "array.h"

static VmOptions options;

static void init_globals(void) {
	options.filepath = "";
	options.verbose = false;
	options.repl = false;
	options.prog = "";
}

#define BUF_SIZE 2000
#define PKGNAME "Panda Smalltalk"
#define VERSION "0.0.1"

static const char version[] = PKGNAME " " VERSION "\n";
static char *str_empty = "";

static void read_compile_stdin(void) {
	
	CompilerError error;
	Oop value;
	
	char *string;
	char c;
	int i = 0;
	
	char buffer[BUF_SIZE];
	memset(buffer, 0x00, BUF_SIZE);
	
	if (options.repl) {
		while ((c = getchar()) != EOF && i < (BUF_SIZE - 1)) {
			if (c == '\n') {
				
				string = st_strconcat("doIt ^ [", buffer, "] value", NULL);
				if (!st_compile_string(ST_UNDEFINED_OBJECT_CLASS, string, &error)) {
					fprintf(stderr, "\n%i: %s\n", error.line, error.message);
					exit(1);
				}
				
				st_machine_initialize(&__machine);
				st_machine_main(&__machine);
				/* inspect the returned value on top of the stack */
				value = ST_STACK_PEEK ((&__machine));
				if (st_object_format(value) != ST_FORMAT_BYTE_ARRAY)
					abort();
				if (__machine.success)
					printf("-> %s\n", (char *) st_byte_array_bytes(value));
				
				i = 0;
				memset(buffer, 0x00, BUF_SIZE);
			}
			
			buffer[i++] = c;
			buffer[i] = '\0';
		}
	} else {
		if (!st_file_get_contents(options.filepath, &string))
			exit(1);
		
		if (!st_compile_string(ST_UNDEFINED_OBJECT_CLASS, string, &error)) {
			fprintf(stderr, "'byte_string%s\n", error.message);
			exit(1);
		}
	}
	
	st_free(string);
}

static void print_help(void) {
	printf("\n%s", version);
	printf("usage: panda [options] <file_path> \n");
	printf("  -h (--help)    :  Show help information\n");
	printf("  -d (--dir)     :  Load directory\n");
	printf("  -r (--repl)    :  Init in repl mode\n");
	printf("  -v (--version) :  Show version information\n");
	printf("  -V (--verbose) :  Show verbose output\n\n");
	exit(0);
}

static double get_elapsed_time(struct timeval before, struct timeval after) {
	return after.tv_sec - before.tv_sec + (after.tv_usec - before.tv_usec) / 1.e6;
}

static void parse_args(int argc, char *argv[]) {
	options.prog = argv[0];
	char *arg;
	char *val = NULL;
	int i;
	
	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if ((i + 1) < argc)
			val = argv[i + 1];
		
		if (strcmp(arg, "-d") == 0) {
			options.filepath = val;
		} else if (strcmp(arg, "-h") == 0) {
			print_help();
		} else if (strcmp(arg, "-r") == 0) {
			options.repl = true;
			//break;
		} else if (strcmp(arg, "-V") == 0) {
			st_set_verbose_mode(options.verbose);
		} else if (strcmp(arg, "-v") == 0) {
			printf(version);
			exit(0);
		} else {
			options.filepath = val;
		}
	}
}

int main(int argc, char *argv[]) {
	Oop value;
	init_globals();
	parse_args(argc, argv);
	bootstrap_universe();
	read_compile_stdin();
	return 0;
}

