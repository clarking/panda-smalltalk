
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>
#include <ptr_array.h>
#include <tommath.h>
#include <time.h>

#ifdef __GNUC__
#define HAVE_COMPUTED_GOTO
#endif

// Check host data model. We only support LP64 at the moment.
#ifndef __LP64__
	#define ST_HOST32             1
	#define ST_HOST64             0
	#define ST_BITS_PER_WORD     32
	#define ST_BITS_PER_INTEGER  32
#else
	#define ST_HOST32            0
	#define ST_HOST64            1
	#define ST_BITS_PER_WORD     64
	#define ST_BITS_PER_INTEGER  32
#endif

#define ST_SMALL_INTEGER_MIN  (-ST_SMALL_INTEGER_MAX - 1)
#define ST_SMALL_INTEGER_MAX  536870911

// threshold is 8 Mb or 16 Mb depending on whether system is 32 or 64 bits
#define ST_COLLECTION_THRESHOLD (sizeof (st_oop) * 2 * 1024 * 1024)

#define ST_TAG_SIZE 2

// bit utilities
#define ST_NTH_BIT(n)         (1 << (n))
#define ST_NTH_MASK(n)        (ST_NTH_BIT(n) - 1)
#define NIL_SIZE_OOPS (sizeof (struct st_header) / sizeof (st_oop))

enum {
	ST_SMI_TAG,
	ST_POINTER_TAG,
	ST_CHARACTER_TAG,
	ST_MARK_TAG,
};

enum {
	_ST_OBJECT_UNUSED_BITS = 15,
	_ST_OBJECT_HASH_BITS = 1,
	_ST_OBJECT_SIZE_BITS = 8,
	_ST_OBJECT_FORMAT_BITS = 6,
	_ST_OBJECT_FORMAT_SHIFT = ST_TAG_SIZE,
	_ST_OBJECT_SIZE_SHIFT = _ST_OBJECT_FORMAT_BITS + _ST_OBJECT_FORMAT_SHIFT,
	_ST_OBJECT_HASH_SHIFT = _ST_OBJECT_SIZE_BITS + _ST_OBJECT_SIZE_SHIFT,
	_ST_OBJECT_UNUSED_SHIFT = _ST_OBJECT_HASH_BITS + _ST_OBJECT_HASH_SHIFT,
	_ST_OBJECT_FORMAT_MASK = ST_NTH_MASK (_ST_OBJECT_FORMAT_BITS),
	_ST_OBJECT_SIZE_MASK = ST_NTH_MASK (_ST_OBJECT_SIZE_BITS),
	_ST_OBJECT_HASH_MASK = ST_NTH_MASK (_ST_OBJECT_HASH_BITS),
	_ST_OBJECT_UNUSED_MASK = ST_NTH_MASK (_ST_OBJECT_UNUSED_BITS),
};

typedef enum st_format {
	ST_FORMAT_OBJECT,
	ST_FORMAT_FLOAT,
	ST_FORMAT_LARGE_INTEGER,
	ST_FORMAT_HANDLE,
	ST_FORMAT_ARRAY,
	ST_FORMAT_BYTE_ARRAY,
	ST_FORMAT_FLOAT_ARRAY,
	ST_FORMAT_INTEGER_ARRAY,
	ST_FORMAT_WORD_ARRAY,
	ST_FORMAT_CONTEXT,
	ST_NUM_FORMATS
} st_format;

/* basic oop pointer:
 * integral type wide enough to hold a C pointer.
 * Can either point to a heap object or contain a smi or Character immediate.
 */
typedef uintptr_t st_oop;

typedef unsigned char st_uchar;
typedef unsigned short st_ushort;
typedef unsigned long st_ulong;
typedef unsigned int st_uint;
typedef void *st_pointer;
typedef st_uint st_unichar;

static inline st_oop
st_tag_pointer(st_pointer p) {
	return ((st_oop) p) + ST_POINTER_TAG;
}

static inline st_oop *
st_detag_pointer(st_oop oop) {
	return (st_oop *) (oop - ST_POINTER_TAG);
}

struct _global {
	int width, maxhelppos, indent;
	const char *helppfx;
	const char *filepath;
	char sf[3]; // Initialised to 0 from here on.
	const char *prog, *usage, *message, *helplf;
	char helpsf, **argv;
	struct opt_spec *opts, *curr;
	int opts1st, helppos;
	bool verbose;
	bool repl;
};

typedef struct _global global;


/*
 * Every heap-allocated object starts with this header word
 *
 * format of mark oop
 * [ unused: 16 | instance-size: 8 | format: 6 | tag: 2 ]
 *
 * format:      object format
 * mark:        object contains a forwarding pointer
 * unused: 	not used yet (haven't implemented GC)
 *
 */
typedef struct st_header {
	st_oop mark;
	st_oop class;
	st_oop fields[];
} st_header;

typedef struct st_arrayed_object {
	st_header parent;
	st_oop size;
} st_arrayed_object;

typedef struct st_array {
	st_arrayed_object parent;
	st_oop elements[];
} st_array;

typedef struct st_word_array {
	st_arrayed_object parent;
	st_uint elements[];
} st_word_array;

typedef struct st_float_array {
	st_arrayed_object parent;
	double elements[];
} st_float_array;

typedef struct st_byte_array {
	st_arrayed_object parent;
	st_uchar bytes[];
} st_byte_array;

typedef struct st_association {
	st_header parent;
	st_oop key;
	st_oop value;
} st_association;

typedef struct st_hashed_collection {
	struct st_header header;
	st_oop size;
	st_oop deleted;
	st_oop array;
} st_hashed_collection;

typedef struct st_behavior {
	st_header header;
	st_oop format;
	st_oop superclass;
	st_oop instance_size;
	st_oop method_dictionary;
	st_oop instance_variables;
} st_behavior;

typedef struct st_class {
	st_behavior parent;
	st_oop name;
} st_class;

typedef struct st_metaclass {
	st_behavior parent;
	st_oop instance_class;
} st_metaclass;

typedef struct st_list st_list;
typedef struct st_list {
	st_pointer data;
	st_list *next;
} st_list;

typedef struct marker {
	int p;
	int line;
	int column;
} marker;

typedef struct st_input {
	char *text;
	st_uint p;        /* current index into text */
	st_uint n;        /* total number of chars in text */
	st_uint line;     /* current line number, starting from 1 */
	st_uint column;   /* current column number, starting from 1 */
	marker marker;
} st_input;

typedef struct st_filein {
	const char *filename;
	st_input *input;
	int line;
} st_filein;

typedef struct st_compiler_error {
	char message[255];
	st_uint line;
	st_uint column;
} st_compiler_error;

typedef enum {
	TOKEN_INVALID,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_BLOCK_BEGIN,
	TOKEN_BLOCK_END,
	TOKEN_COMMA,
	TOKEN_SEMICOLON,
	TOKEN_PERIOD,
	TOKEN_RETURN,
	TOKEN_COLON,
	TOKEN_ASSIGN,
	TOKEN_TUPLE_BEGIN,
	TOKEN_IDENTIFIER,
	TOKEN_CHARACTER_CONST,
	TOKEN_STRING_CONST,
	TOKEN_NUMBER_CONST,
	TOKEN_SYMBOL_CONST,
	TOKEN_COMMENT,
	TOKEN_BINARY_SELECTOR,
	TOKEN_KEYWORD_SELECTOR,
	TOKEN_EOF
} st_token_type;

typedef struct st_token {
	st_token_type type;
	int line;
	int column;
	union {
		struct {
			char *text;
		};
		struct { // Number Token
			bool negative;
			char *number;
			int radix;
			int exponent;
		};
	};
} st_token;

typedef enum {
	ERROR_MISMATCHED_CHAR,
	ERROR_NO_VIABLE_ALT_FOR_CHAR,
	ERROR_ILLEGAL_CHAR,
	ERROR_UNTERMINATED_COMMENT,
	ERROR_UNTERMINATED_STRING_LITERAL,
	ERROR_INVALID_RADIX,
	ERROR_INVALID_CHAR_CONST,
	ERROR_NO_ALT_FOR_POUND,
	
} err_code;

typedef struct st_lexer {
	st_input *input;
	bool filter_comments;
	bool token_matched;
	
	// data for next token
	st_uint line;
	st_uint column;
	st_uint start;
	st_token *token;
	
	// error control
	bool failed;
	jmp_buf main_loop;
	
	// last error information
	err_code error_code;
	st_uint error_line;
	st_uint error_column;
	char error_char;
	
	st_list *allocated_tokens; // delayed deallocation
} st_lexer;


typedef struct st_parser {
	st_lexer *lexer;
	bool in_block;
	st_compiler_error *error;
	jmp_buf jmploc;
} st_parser;

typedef enum {
	ST_METHOD_NODE,
	ST_BLOCK_NODE,
	ST_VARIABLE_NODE,
	ST_ASSIGN_NODE,
	ST_RETURN_NODE,
	ST_MESSAGE_NODE,
	ST_CASCADE_NODE,
	ST_LITERAL_NODE,
} st_node_type;

typedef enum {
	ST_UNARY_PRECEDENCE,
	ST_BINARY_PRECEDENCE,
	ST_KEYWORD_PRECEDENCE,
} st_msg_precedence;

typedef struct st_node st_node;
typedef struct st_node {
	st_node_type type;
	int line;
	st_node *next;
	union {
		struct {
			st_msg_precedence precedence;
			int primitive;
			st_oop selector;
			st_node *statements;
			st_node *temporaries;
			st_node *arguments;
			
		} method;
		
		struct {
			st_msg_precedence precedence;
			bool is_statement;
			st_oop selector;
			st_node *receiver;
			st_node *arguments;
			bool super_send;
		} msg;
		
		struct {
			char *name;
		} variable;
		
		struct {
			st_oop value;
		} literal;
		
		struct {
			st_node *assignee;
			st_node *expression;
		} assign;
		
		struct {
			st_node *expression;
		} retrn;
		
		struct {
			st_node *statements;
			st_node *temporaries;
			st_node *arguments;
		} block;
		
		struct {
			st_node *receiver;
			st_list *messages;
			bool is_statement;
		} cascade;
	};
	
} st_node;

typedef struct st_handle {
	st_header header;
	uintptr_t value;
} st_handle;

typedef struct st_generator {
	st_oop class;
	jmp_buf jmploc;
	st_compiler_error *error;
	st_list *temporaries; // names of temps, in order of appearance
	st_list *instvars;    // names of instvars, in order they were defined
	st_list *literals;    // literal frame for the compiled code
} st_generator;

typedef struct st_bytecode {
	st_uchar *buffer;
	st_uint size;
	st_uint alloc;
	st_uint max_stack_depth;
} st_bytecode;

typedef struct st_cell {
	st_oop object;
	st_uint hash;
} st_cell;

typedef struct st_identity_hashtable {
	st_cell *table;
	st_uint alloc;
	st_uint size;
	st_uint deleted;
	st_uint current_hash;
	
} st_identity_hashtable;

typedef struct st_heap {
	st_uchar *start;        // start of reserved address space
	st_uchar *p;            // end of committed address space (`start' to `p' is thus writeable memory)
	st_uchar *end;          // end of reserved address space
} st_heap;

typedef struct st_memory {
	st_heap *heap;
	st_oop *start, *end;
	st_oop *p;
	st_oop *mark_stack;
	st_uint mark_stack_size;
	st_uchar *mark_bits;
	st_uchar *alloc_bits;
	st_uint bits_size;                   // in bytes
	st_oop **offsets;
	st_uint offsets_size;                // in bytes
	ptr_array roots;
	st_uint counter;
	st_oop free_context;                 // free context pool
	struct timespec total_pause_time;    // total accumulated pause time
	st_ulong bytes_allocated;            // current number of allocated bytes
	st_ulong bytes_collected;            // number of bytes collected in last compaction
	
	st_identity_hashtable *ht;
	
} st_memory;


typedef struct st_large_integer {
	struct st_header parent;
	mp_int value;
} st_large_integer;
