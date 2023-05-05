/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
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

#define PKGNAME "Panda Smalltalk"
#define VERSION "0.0.1"

static void init_globals(void) {
	options.src_dir = "../st";
	options.script = "";
	options.verbose = false;
	options.mode = VM_MODE_EXPR;
}

static const char version[] = PKGNAME " " VERSION "\n";

static void do_it(char *buffer) {
	CompilerError error;
	char *string = st_strconcat("doIt ^ [", buffer, "] value", NULL);
	if (!st_compile_string(ST_UNDEFINED_OBJECT_CLASS, string, &error)) {
		fprintf(stderr, "\n%i: %s\n", error.line, error.message);
		if (options.mode != VM_MODE_REPL)
			exit(1);
	}
	else {
		st_machine_initialize(&__machine);
		st_machine_main(&__machine);
		/* inspect the returned value on top of the stack */
		Oop value = ST_STACK_PEEK ((&__machine));
		if (st_object_format(value) != ST_FORMAT_BYTE_ARRAY)
			abort();
		if (__machine.success)
			printf("-> %s\n", (char *) st_byte_array_bytes(value));
	}

	st_free(string);
}

static void read_compile_stdin(void) {

	char c;
	int i = 0;
	char buffer[INPUT_BUF_SIZE];
	memset(buffer, 0x00, INPUT_BUF_SIZE);

	switch (options.mode) {
		case VM_MODE_EXPR:
			do_it(st_strconcat(options.script, NULL));
			break;
		case VM_MODE_REPL:
			while ((c = getchar()) != EOF && i < (INPUT_BUF_SIZE - 1)) {
				if (c == '\n') {
					do_it(buffer);
					i = 0;
					memset(buffer, 0x00, INPUT_BUF_SIZE);
				}

				buffer[i++] = c;
				buffer[i] = '\0';
			}
			break;
		case VM_MODE_FILE:
			compile_file_in(options.script);
			break;
		default:
			break;
	}
}

static void print_help(void) {
	printf("\n%s", version);
	printf("usage: panda [options] <file_path> \n");
	printf("  -h (--help)    :  Show help information\n");
	printf("  -d (--dir)     :  Load directory\n");
	printf("  -s (--script)  :  Load script file\n");
	printf("  -r (--repl)    :  Init in repl mode\n");
	printf("  -e (--eval)    :  Evaluate expression\n");
	printf("  -v (--version) :  Print version\n");
	printf("  -V (--verbose) :  Verbose output\n\n");
	exit(0);
}

static double get_elapsed_time(struct timeval before, struct timeval after) {
	return after.tv_sec - before.tv_sec + (after.tv_usec - before.tv_usec) / 1.e6;
}

static void parse_args(int argc, char *argv[]) {

	char *arg;
	char *val = NULL;
	int i;

	if (argc == 1)
		print_help();

	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if ((i + 1) < argc)
			val = argv[i + 1];

		if (strcmp(arg, "-d") == 0)
			options.src_dir = val;
		if (strcmp(arg, "-s") == 0) {
			options.mode = VM_MODE_FILE;
			options.script = val;
		}
		else if (strcmp(arg, "-h") == 0)
			print_help();
		else if (strcmp(arg, "-r") == 0) {
			options.mode = VM_MODE_REPL;
		}
		else if (strcmp(arg, "-e") == 0) {
			options.mode = VM_MODE_EXPR;
			options.script = val;
		}
		else if (strcmp(arg, "-V") == 0) {
			st_set_verbose_mode(options.verbose);
		}
		else if (strcmp(arg, "-v") == 0) {
			printf(version);
			exit(0);
		}
	}
}

int main(int argc, char *argv[]) {
	init_globals();
	parse_args(argc, argv);
	bootstrap_universe();
	read_compile_stdin();
	return 0;
}

