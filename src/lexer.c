
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "types.h"
#include "lexer.h"
#include "input.h"
#include "utils.h"
#include "unicode.h"

#define lookahead(self, k)   ((char) st_input_look_ahead (self->input, k))
#define consume(self)        (st_input_consume (self->input))
#define mark(self)           (st_input_mark (self->input))
#define rewind(self)         (st_input_rewind (self->input))

static void make_token(Lexer *lexer, TokenType type, char *text) {
	Token *token;
	
	token = st_new0 (Token);
	token->type = type;
	token->text = text ? text : st_strdup("");
	token->type = type;
	token->line = lexer->line;
	token->column = lexer->column;
	
	lexer->token = token;
	lexer->token_matched = true;
	lexer->allocated_tokens = st_list_prepend(lexer->allocated_tokens, token);
}

static void make_number_token(Lexer *lexer, int radix, int exponent, char *number, bool negative) {
	Token *token;
	
	token = st_new0 (Token);
	token->type = TOKEN_NUMBER_CONST;
	token->line = lexer->line;
	token->column = lexer->column;
	token->negative = negative;
	token->number = number;
	token->radix = radix;
	token->exponent = exponent;
	
	lexer->token = token;
	lexer->token_matched = true;
	lexer->allocated_tokens = st_list_prepend(lexer->allocated_tokens, token);
}

static void raise_error(Lexer *lexer, ErrCode error_code, char error_char) {
	lexer->failed = true;
	lexer->error_code = error_code;
	lexer->error_char = error_char;
	lexer->error_line = lexer->line;
	lexer->error_column = lexer->column;
	
	make_token(lexer, TOKEN_INVALID, NULL);
	consume (lexer); // hopefully recover after consuming char
	longjmp(lexer->main_loop, 0); // go back to main loop
}

static void match_range(Lexer *lexer, char a, char b) {
	if (lookahead (lexer, 1) < a || lookahead (lexer, 1) > b)
		raise_error(lexer, ERROR_MISMATCHED_CHAR, lookahead (lexer, 1));
	consume (lexer);
}

static void match(Lexer *lexer, char c) {
	if (lookahead (lexer, 1) != c) {
		// mismatch error
		raise_error(lexer, ERROR_MISMATCHED_CHAR, lookahead (lexer, 1));
	}
	consume (lexer);
}

static bool is_special_char(char c) {
	switch (c) {
		
		case '+':
		case '/':
		case '\\':
		case '*':
		case '~':
		case '<':
		case '>':
		case '=':
		case '@':
		case '%':
		case '|':
		case '&':
		case '?':
		case '!':
		case ',':
			return true;
		
		default:
			return false;
		
	}
}

/* check if a char is valid numeral identifier for a given radix
 * 
 * for example, 2r1010301 is an invalid number since the '3' is not within the radix.
 * 
 **/
static bool is_radix_numeral(uint radix, char c) {
	st_assert (radix >= 2 && radix <= 36);
	
	if (radix > 10)
		return (c >= '0' && c <= '9') || (c >= 'A' && c <= ('A' - 1 + (radix - 10)));
	else
		return c >= '0' && c <= ('0' - 1 + radix);
}

/* Numbers. We do just do basic matching here. Actual parsing and conversion can
 * be done in the parser. 
 */
static void match_number(Lexer *lexer) {
	// We don't match any leading '-'.
	// The parser will resolve whether a '-' specifies a negative number or a binary selector
	
	bool negative = false;
	long radix = 10;
	long exponent = 0;
	int k, j, l;
	char *string;
	
	if (lookahead (lexer, 1) == '-') {
		negative = true;
		consume (lexer);
	}
	
	k = st_input_index(lexer->input);
	
	do {
		match_range(lexer, '0', '9');
	}
	while (isdigit (lookahead(lexer, 1)));
	
	if (lookahead (lexer, 1) != 'r') {
		j = st_input_index(lexer->input);
		goto out1;
	}
	else {
		string = st_input_range(lexer->input, k, st_input_index(lexer->input));
		radix = strtol(string, NULL, 10);
		st_free(string);
		if (radix < 2 || radix > 36)
			raise_error(lexer, ERROR_INVALID_RADIX, lookahead (lexer, 1));
		consume (lexer);
	}
	
	k = st_input_index(lexer->input);
	if (lookahead (lexer, 1) == '-')
		raise_error(lexer, ERROR_NO_VIABLE_ALT_FOR_CHAR, lookahead (lexer, 1));
	
	out1:
	while (is_radix_numeral(radix, lookahead (lexer, 1)))
		consume (lexer);
	
	if (lookahead (lexer, 1) == '.' && is_radix_numeral(radix, lookahead (lexer, 2))) {
		consume (lexer);
		do {
			consume (lexer);
		}
		while (is_radix_numeral(radix, lookahead (lexer, 1)));
	}
	
	j = st_input_index(lexer->input);
	if (lookahead (lexer, 1) == 'e') {
		
		consume (lexer);
		l = st_input_index(lexer->input);
		
		if (lookahead (lexer, 1) == '-' && isdigit (lookahead(lexer, 2)))
			consume (lexer);
		
		while (isdigit (lookahead(lexer, 1)))
			consume (lexer);
		
		if (l == st_input_index(lexer->input))
			goto out2;
		
		string = st_input_range(lexer->input, l, st_input_index(lexer->input));
		exponent = strtol(string, NULL, 10);
		st_free(string);
	}
	
	out2:
	make_number_token(lexer, radix, exponent, st_input_range(lexer->input, k, j), negative);
}

static void match_identifier(Lexer *lexer, bool create_token) {
	if (isalpha (lookahead(lexer, 1)))
		consume (lexer);
	else {
		raise_error(lexer, ERROR_NO_VIABLE_ALT_FOR_CHAR, lookahead (lexer, 1));
	}
	
	while (true) {
		if (isalpha (lookahead(lexer, 1)))
			consume (lexer);
		else if (lookahead (lexer, 1) >= '0' && lookahead (lexer, 1) <= '9')
			consume (lexer);
		else if (lookahead (lexer, 1) == '_')
			consume (lexer);
		else
			break;
	}
	
	if (create_token)
		make_token(lexer, TOKEN_IDENTIFIER, st_input_range(lexer->input, lexer->start, st_input_index(lexer->input)));
}

static void match_keyword_or_identifier(Lexer *lexer, bool create_token) {
	if (isalpha (lookahead(lexer, 1)))
		consume (lexer);
	else
		raise_error(lexer, ERROR_NO_VIABLE_ALT_FOR_CHAR, lookahead (lexer, 1));
	
	while (true) {
		if (isalpha (lookahead(lexer, 1)))
			consume (lexer);
		else if (lookahead (lexer, 1) >= '0' && lookahead (lexer, 1) <= '9')
			consume (lexer);
		else if (lookahead (lexer, 1) == '_')
			consume (lexer);
		else
			break;
	}
	
	TokenType token_type;
	if (lookahead (lexer, 1) == ':' && lookahead (lexer, 2) != '=') {
		consume (lexer);
		token_type = TOKEN_KEYWORD_SELECTOR;
	}
	else
		token_type = TOKEN_IDENTIFIER;
	
	if (create_token) {
		char *text;
		if (token_type == TOKEN_KEYWORD_SELECTOR)
			text = st_input_range(lexer->input, lexer->start, st_input_index(lexer->input));
		else
			text = st_input_range(lexer->input, lexer->start, st_input_index(lexer->input));
		make_token(lexer, token_type, text);
	}
}

static void match_string_constant(Lexer *lexer) {
	mark (lexer);
	match(lexer, '\'');
	while (lookahead (lexer, 1) != '\'') {
		consume (lexer);
		if (lookahead (lexer, 1) == ST_INPUT_EOF) {
			rewind (lexer);
			raise_error(lexer, ERROR_UNTERMINATED_STRING_LITERAL, lookahead (lexer, 1));
		}
	}
	
	match(lexer, '\'');
	char *string;
	string = st_input_range(lexer->input, lexer->start + 1, st_input_index(lexer->input) - 1);
	make_token(lexer, TOKEN_STRING_CONST, string);
}

static void match_comment(Lexer *lexer) {
	mark (lexer);
	match(lexer, '"');
	while (lookahead (lexer, 1) != '"') {
		consume (lexer);
		if (lookahead (lexer, 1) == ST_INPUT_EOF) {
			rewind (lexer);
			raise_error(lexer, ERROR_UNTERMINATED_COMMENT, lookahead (lexer, 1));
		}
	}
	
	match(lexer, '"');
	if (!lexer->filter_comments) {
		char *comment;
		comment = st_input_range(lexer->input, lexer->start + 1, st_input_index(lexer->input) - 1);
		make_token(lexer, TOKEN_COMMENT, comment);
	}
}

static void match_tuple_begin(Lexer *lexer) {
	match(lexer, '#');
	match(lexer, '(');
	make_token(lexer, TOKEN_TUPLE_BEGIN, st_strdup("#("));
}

static void match_binary_selector(Lexer *lexer, bool create_token) {
	if (lookahead (lexer, 1) == '-') {
		match(lexer, '-');
		if (is_special_char(lookahead (lexer, 1)))
			match(lexer, lookahead (lexer, 1));
		
	}
	else if (is_special_char(lookahead (lexer, 1))) {
		match(lexer, lookahead (lexer, 1));
		if (is_special_char(lookahead (lexer, 1)))
			match(lexer, lookahead (lexer, 1));
		
	}
	else
		raise_error(lexer, ERROR_NO_VIABLE_ALT_FOR_CHAR, lookahead (lexer, 1));
	
	if (create_token)
		make_token(lexer, TOKEN_BINARY_SELECTOR,
		           st_input_range(lexer->input, lexer->start, st_input_index(lexer->input)));
}

static void match_symbol_constant(Lexer *lexer) {
	match(lexer, '#');
	
	if (isalpha (lookahead(lexer, 1))) {
		do {
			match_keyword_or_identifier(lexer, false);
		}
		while (isalpha (lookahead(lexer, 1)));
		
	}
	else if (lookahead (lexer, 1) == '-' || is_special_char(lookahead (lexer, 1))) {
		match_binary_selector(lexer, false);
	}
	else
		raise_error(lexer, ERROR_NO_ALT_FOR_POUND, lookahead (lexer, 1));
	
	// discard #
	char *symbol_text = st_input_range(lexer->input, lexer->start + 1, st_input_index(lexer->input));
	make_token(lexer, TOKEN_SYMBOL_CONST, symbol_text);
}

static void match_block_begin(Lexer *lexer) {
	match(lexer, '[');
	make_token(lexer, TOKEN_BLOCK_BEGIN, NULL);
}

static void match_block_end(Lexer *lexer) {
	match(lexer, ']');
	make_token(lexer, TOKEN_BLOCK_END, NULL);
}

static void match_lparen(Lexer *lexer) {
	match(lexer, '(');
	make_token(lexer, TOKEN_LPAREN, NULL);
}

static void match_rparen(Lexer *lexer) {
	match(lexer, ')');
	make_token(lexer, TOKEN_RPAREN, NULL);
}

static void match_char_constant(Lexer *lexer) {
	char ch = 0;
	match(lexer, '$');
	if (lookahead (lexer, 1) == '\\') {
		if (lookahead (lexer, 2) == 't') {
			ch = '\t';
			consume (lexer);
			consume (lexer);
		}
		else if (lookahead (lexer, 2) == 'f') {
			ch = '\f';
			consume (lexer);
			consume (lexer);
		}
		else if (lookahead (lexer, 2) == 'n') {
			ch = '\n';
			consume (lexer);
			consume (lexer);
		}
		else if (lookahead (lexer, 2) == 'r') {
			ch = '\r';
			consume (lexer);
			consume (lexer);
		}
		else if (isxdigit (lookahead(lexer, 2))) {
			consume (lexer);
			int start = st_input_index(lexer->input);
			do {
				consume (lexer);
			}
			while (isxdigit (lookahead(lexer, 1)));
			
			char *string = st_input_range(lexer->input, start, st_input_index(lexer->input));
			ch = strtol(string, NULL, 16);
			st_free(string);
		}
		else {
			ch = '\\';  // just match a single '\' char
			consume (lexer);
		}
		
	}
	else if (isgraph (lookahead(lexer, 1))) {
		ch = lookahead (lexer, 1);
		consume (lexer);
	}
	else
		raise_error(lexer, ERROR_INVALID_CHAR_CONST, lookahead (lexer, 1));
	
	char outbuf[6];
	st_unichar_to_utf8(ch, outbuf);
	make_token(lexer, TOKEN_CHARACTER_CONST, st_strdup(outbuf));
}

static void match_eof(Lexer *lexer) {
	match(lexer, ST_INPUT_EOF);
	make_token(lexer, TOKEN_EOF, NULL);
}

static void match_white_space(Lexer *lexer) {
	// gobble up white space
	while (true) {
		switch (lookahead (lexer, 1)) {
			case ' ':
			case '\r':
			case '\n':
			case '\t':
			case '\f':
				consume (lexer);
				break;
			default:
				return;
		}
	}
}

static void match_colon(Lexer *lexer) {
	match(lexer, ':');
	make_token(lexer, TOKEN_COLON, NULL);
}

static void match_semicolon(Lexer *lexer) {
	match(lexer, ';');
	make_token(lexer, TOKEN_SEMICOLON, NULL);
}

static void match_assign(Lexer *lexer) {
	match(lexer, ':');
	match(lexer, '=');
	make_token(lexer, TOKEN_ASSIGN, NULL);
}

static void match_period(Lexer *lexer) {
	match(lexer, '.');
	make_token(lexer, TOKEN_PERIOD, NULL);
}

static void match_return(Lexer *lexer) {
	match(lexer, '^');
	make_token(lexer, TOKEN_RETURN, NULL);
}

/* st_lexer_next_token:
 * lexer: a Lexer
 *
 * Returns the next matched token from the input stream. Caller takes
 * ownership of returned token.
 *
 * If the end of the input stream is reached, tokens of type ST_TOKEN_EOF
 * will be returned. Similarly, if there are matching errors, then tokens
 * of type ST_TOKEN_INVALID will be returned;
 *
 */

Token *st_lexer_next_token(Lexer *lexer) {
	st_assert (lexer != NULL);
	
	while (true) {
		// reset token and error state
		lexer->failed = false;
		lexer->token_matched = false;
		lexer->line = st_input_get_line(lexer->input);
		lexer->column = st_input_get_column(lexer->input);
		lexer->start = st_input_index(lexer->input);
		
		// we return here on match errors and then goto out
		if (setjmp (lexer->main_loop))
			goto out;
		
		switch (lookahead (lexer, 1)) {
			case ' ':
			case '\n':
			case '\r':
			case '\t':
			case '\f':
				match_white_space(lexer);
				break;
			case '(':
				match_lparen(lexer);
				break;
			case ')':
				match_rparen(lexer);
				break;
			case '[':
				match_block_begin(lexer);
				break;
			case ']':
				match_block_end(lexer);
				break;
			case '^':
				match_return(lexer);
				break;
			case '.':
				match_period(lexer);
				break;
			case ';':
				match_semicolon(lexer);
				break;
			case '+':
			case '/':
			case '\\':
			case '*':
			case '<':
			case '>':
			case '=':
			case '@':
			case '%':
			case '|':
			case '&':
			case '?':
			case '!':
			case '~':
			case ',':
				match_binary_selector(lexer, true);
				break;
			case '$':
				match_char_constant(lexer);
				break;
			case '"':
				match_comment(lexer);
				break;
			case '\'':
				match_string_constant(lexer);
				break;
			case ST_INPUT_EOF:
				match_eof(lexer);
				break;
			default:
				if (isalpha (lookahead(lexer, 1)))
					match_keyword_or_identifier(lexer, true);
				else if (lookahead (lexer, 1) == '-' && isdigit (lookahead(lexer, 2)))
					match_number(lexer);
				else if (isdigit (lookahead(lexer, 1)))
					match_number(lexer);
				else if (lookahead (lexer, 1) == '-')
					match_binary_selector(lexer, true);
				else if (lookahead (lexer, 1) == '#' && lookahead (lexer, 2) == '(')
					match_tuple_begin(lexer);
				else if (lookahead (lexer, 1) == '#')
					match_symbol_constant(lexer);
					// match assign or colon
				else if (lookahead (lexer, 1) == ':' && lookahead (lexer, 2) == '=')
					match_assign(lexer);
				else if (lookahead (lexer, 1) == ':')
					match_colon(lexer);
				else
					raise_error(lexer, ERROR_ILLEGAL_CHAR, lookahead (lexer, 1));
		}
		
		out:
		// we return the matched token or an invalid token on error
		if (lexer->token_matched || lexer->failed)
			return lexer->token;
		else
			continue;
	}
}

static void lexer_initialize(Lexer *lexer, LexInput *input) {
	lexer->input = input;
	lexer->token = NULL;
	lexer->line = 1;
	lexer->column = 1;
	lexer->start = -1;
	lexer->error_code = 0;
	lexer->failed = false;
	lexer->filter_comments = true;
	lexer->allocated_tokens = NULL;
}

Lexer *st_lexer_new(const char *string) {
	Lexer *lexer;
	LexInput *input;
	
	st_assert (string != NULL);
	lexer = st_new0 (Lexer);
	input = st_input_new(string);
	if (!input)
		return NULL;
	
	lexer_initialize(lexer, input);
	return lexer;
}

void destroy_token(Token *token) {
	if (token->type != TOKEN_NUMBER_CONST)
		st_free(token->text);
	else
		st_free(token->number);
	
	st_free(token);
}

void st_lexer_destroy(Lexer *lexer) {
	st_assert (lexer != NULL);
	st_input_destroy(lexer->input);
	st_list_foreach(lexer->allocated_tokens, (st_list_foreach_func) destroy_token);
	st_list_destroy(lexer->allocated_tokens);
	st_free(lexer);
}

TokenType st_token_get_type(Token *token) {
	st_assert (token != NULL);
	return token->type;
}

char *st_token_get_text(Token *token) {
	st_assert (token != NULL);
	return token->text;
}

uint st_token_get_line(Token *token) {
	st_assert (token != NULL);
	return token->line;
}

uint st_token_get_column(Token *token) {
	st_assert (token != NULL);
	return token->column;
}

uint st_lexer_error_line(Lexer *lexer) {
	st_assert (lexer != NULL);
	return lexer->error_line;
}

uint st_lexer_error_column(Lexer *lexer) {
	st_assert (lexer != NULL);
	return lexer->error_column;
}

char st_lexer_error_char(Lexer *lexer) {
	st_assert (lexer != NULL);
	return lexer->error_char;
}

char *st_lexer_error_message(Lexer *lexer) {
	st_assert (lexer != NULL);
	static const char *msgformats[] = {
		"mismatched character \\%04X",
		"no viable alternative for character \\%04X",
		"illegal character \\%04X",
		"unterminated comment",
		"unterminated string literal",
		"invalid radix for number",
		"non-whitespace character expected after '$'",
		"expected '(' after '#'",
	};
	
	switch (lexer->error_code) {
		case ERROR_UNTERMINATED_COMMENT:
		case ERROR_UNTERMINATED_STRING_LITERAL:
		case ERROR_INVALID_RADIX:
		case ERROR_INVALID_CHAR_CONST:
		case ERROR_NO_ALT_FOR_POUND:
			return st_strdup_printf("%s", msgformats[lexer->error_code]);
		case ERROR_MISMATCHED_CHAR:
		case ERROR_NO_VIABLE_ALT_FOR_CHAR:
		case ERROR_ILLEGAL_CHAR:
			return st_strdup_printf(msgformats[lexer->error_code], lexer->error_char);
		default:
			return NULL;
	}
}

Token *st_lexer_current_token(Lexer *lexer) {
	return lexer->token;
}

void st_lexer_filter_comments(Lexer *lexer, bool filter) {
	lexer->filter_comments = filter;
}

bool st_number_token_negative(Token *token) {
	return token->negative;
}

char *st_number_token_number(Token *token) {
	return token->number;
}

uint st_number_token_radix(Token *token) {
	return token->radix;
}

int st_number_token_exponent(Token *token) {
	return token->exponent;
}
