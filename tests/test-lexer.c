

#include <st-lexer.h>
#include <stdio.h>
#include <stdbool.h>

static const char *const token_names[] = {
		"TOKEN_INVALID",
		"TOKEN_LPAREN",
		"TOKEN_RPAREN",
		"TOKEN_BLOCK_BEGIN",
		"TOKEN_BLOCK_END",
		"TOKEN_COMMA",
		"TOKEN_SEMICOLON",
		"TOKEN_PERIOD",
		"TOKEN_RETURN",
		"TOKEN_COLON",
		"TOKEN_ASSIGN",
		"TOKEN_TUPLE_BEGIN",
		"TOKEN_IDENTIFIER",
		"TOKEN_CHARACTER_CONST",
		"TOKEN_STRING_CONST",
		"TOKEN_NUMBER_CONST",
		"TOKEN_SYMBOL_CONST",
		"TOKEN_COMMENT",
		"TOKEN_BINARY_SELECTOR",
		"TOKEN_KEYWORD_SELECTOR",
		"TOKEN_EOF",
};

static void print_token(st_lexer *lexer, st_token *token) {
	st_token_type type;
	char *string;
	type = st_token_get_type(token);
	switch (type) {
		// tokens string values
		case TOKEN_COMMENT:
		case TOKEN_IDENTIFIER:
		case TOKEN_STRING_CONST:
		case TOKEN_SYMBOL_CONST:
		case TOKEN_NUMBER_CONST:
		case TOKEN_KEYWORD_SELECTOR:
		case TOKEN_BINARY_SELECTOR:
		case TOKEN_CHARACTER_CONST:
			
			string = st_token_get_text(token);
			
			printf("%s (%i:%i: \"%s\")\n", token_names[type],
					st_token_get_line(token), st_token_get_column(token), string);
			break;
			
			// Invalid Token;
		case TOKEN_INVALID:
			printf("%s\n", st_lexer_error_message(lexer));
			break;
		
		default:
			printf("%s (%i:%i)\n", token_names[type],
					st_token_get_line(token), st_token_get_column(token));
			break;
	}
}

#define BUF_SIZE 10000

int main(int argc, char *argv[]) {
	st_lexer *lexer;
	st_token *token;
	st_token_type type;
	
	printf("Enter or pipe some Smalltalk code on stdin:\n\n");
	
	/* read input from stdin */
	char buffer[BUF_SIZE];
	char c;
	int i = 0;
	while ((c = getchar()) != EOF && i < (BUF_SIZE - 1))
		buffer[i++] = c;
	buffer[i] = '\0';
	
	lexer = st_lexer_new(buffer);
	
	while (type != TOKEN_EOF) {
		token = st_lexer_next_token(lexer);
		type = st_token_get_type(token);
		print_token(lexer, token);
	};
	
	return 0;
}
