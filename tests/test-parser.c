

#include <st-compiler.h>
#include <st-lexer.h>
#include <st-node.h>
#include <st-universe.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define BUF_SIZE 10000

int main(int argc, char *argv[]) {
	/* read input from stdin */
	char buffer[BUF_SIZE];
	char c;
	int i = 0;
	while ((c = getchar()) != EOF && i < (BUF_SIZE - 1))
		buffer[i++] = c;
	buffer[i] = '\0';
	
	bootstrap_universe();
	Lexer *lexer = st_lexer_new(buffer);
	CompilerError error;
	Node *node = st_parser_parse(lexer, &error);
	
	if (!node) {
		fprintf(stderr, "test-parser:%i: %s\n", error.line, error.message);
		exit(1);
	}
	
	printf("-------------------\n");
	st_print_method_node(node);
	st_node_destroy(node);
	
	return 0;
}

