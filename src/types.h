
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
#define ST_COLLECTION_THRESHOLD (sizeof (Oop) * 2 * 1024 * 1024)

#define ST_TAG_SIZE 2

// bit utilities
#define ST_NTH_BIT(n)         (1 << (n))
#define ST_NTH_MASK(n)        (ST_NTH_BIT(n) - 1)
#define NIL_SIZE_OOPS (sizeof (struct ObjHeader) / sizeof (Oop))

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


typedef struct StInteger {
	uint64_t dummy:1;
	uint64_t value:63;
} StInteger;

/* basic oop pointer:
 * integral type wide enough to hold a C pointer.
 * Can either point to a heap object or contain a smi or Character immediate.
 */

typedef uintptr_t Oop;

typedef unsigned char st_uchar;
typedef unsigned short st_ushort;
typedef unsigned long st_ulong;
typedef unsigned int st_uint;

typedef st_uint st_unichar;

static inline Oop st_tag_pointer(void * p) {
	return ((Oop) p) + ST_POINTER_TAG;
}

static inline Oop *st_detag_pointer(Oop oop) {
	return (Oop *) (oop - ST_POINTER_TAG);
}

typedef struct VmOptions {
	const char *filepath;
	char *prog;
	bool verbose;
	bool repl;
}VmOptions;



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
typedef struct ObjHeader {
	Oop mark;
	Oop class;
	Oop fields[];
} ObjHeader;

typedef struct ArrayedObject {
	ObjHeader parent;
	Oop size;
} ArrayedObject;

typedef struct Array {
	ArrayedObject parent;
	Oop elements[];
} Array;

typedef struct WordArray {
	ArrayedObject parent;
	st_uint elements[];
} WordArray;

typedef struct FloatArray {
	ArrayedObject parent;
	double elements[];
} FloatArray;

typedef struct ByteArray {
	ArrayedObject parent;
	st_uchar bytes[];
} ByteArray;

typedef struct Association {
	ObjHeader parent;
	Oop key;
	Oop value;
} Association;

typedef struct HashedCollection {
	struct ObjHeader header;
	Oop size;
	Oop deleted;
	Oop array;
} HashedCollection;

typedef struct Behavior {
	ObjHeader header;
	Oop format;
	Oop superclass;
	Oop instance_size;
	Oop method_dictionary;
	Oop instance_variables;
} Behavior;

typedef struct Class {
	Behavior parent;
	Oop name;
} Class;

typedef struct MetaClass {
	Behavior parent;
	Oop instance_class;
} MetaClass;

typedef struct List List;
typedef struct List {
	void * data;
	List *next;
} List;

typedef struct InputMarker {
	int p;
	int line;
	int column;
} InputMarker;

typedef struct LexInput {
	char *text;
	st_uint p;        /* current index into text */
	st_uint n;        /* total number of chars in text */
	st_uint line;     /* current line number, starting from 1 */
	st_uint column;   /* current column number, starting from 1 */
	InputMarker InputMarker;
} LexInput;

typedef struct FileIn {
	const char *filename;
	LexInput *input;
	int line;
} FileIn;

typedef struct CompilerError {
	char message[255];
	st_uint line;
	st_uint column;
} CompilerError;

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
} TokenType;

typedef struct Token {
	TokenType type;
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
} Token;

typedef enum {
	ERROR_MISMATCHED_CHAR,
	ERROR_NO_VIABLE_ALT_FOR_CHAR,
	ERROR_ILLEGAL_CHAR,
	ERROR_UNTERMINATED_COMMENT,
	ERROR_UNTERMINATED_STRING_LITERAL,
	ERROR_INVALID_RADIX,
	ERROR_INVALID_CHAR_CONST,
	ERROR_NO_ALT_FOR_POUND,
	
} ErrCode;

typedef struct Lexer {
	LexInput *input;
	bool filter_comments;
	bool token_matched;
	
	// data for next token
	st_uint line;
	st_uint column;
	st_uint start;
	Token *token;
	
	// error control
	bool failed;
	jmp_buf main_loop;
	
	// last error information
	ErrCode error_code;
	st_uint error_line;
	st_uint error_column;
	char error_char;
	
	List *allocated_tokens; // delayed deallocation
} Lexer;


typedef struct Parser {
	Lexer *lexer;
	bool in_block;
	CompilerError *error;
	jmp_buf jmploc;
} Parser;

typedef enum {
	ST_METHOD_NODE,
	ST_BLOCK_NODE,
	ST_VARIABLE_NODE,
	ST_ASSIGN_NODE,
	ST_RETURN_NODE,
	ST_MESSAGE_NODE,
	ST_CASCADE_NODE,
	ST_LITERAL_NODE,
} NodeType;

typedef enum {
	ST_UNARY_PRECEDENCE,
	ST_BINARY_PRECEDENCE,
	ST_KEYWORD_PRECEDENCE,
} MsgPrecedence;

typedef struct Node Node;
typedef struct Node {
	NodeType type;
	int line;
	Node *next;
	union {
		struct {
			MsgPrecedence precedence;
			int primitive;
			Oop selector;
			Node *statements;
			Node *temporaries;
			Node *arguments;
			
		} method;
		
		struct {
			MsgPrecedence precedence;
			bool is_statement;
			Oop selector;
			Node *receiver;
			Node *arguments;
			bool super_send;
		} msg;
		
		struct {
			char *name;
		} variable;
		
		struct {
			Oop value;
		} literal;
		
		struct {
			Node *assignee;
			Node *expression;
		} assign;
		
		struct {
			Node *expression;
		} retrn;
		
		struct {
			Node *statements;
			Node *temporaries;
			Node *arguments;
		} block;
		
		struct {
			Node *receiver;
			List *messages;
			bool is_statement;
		} cascade;
	};
	
} Node;

typedef struct Handle {
	ObjHeader header;
	uintptr_t value;
} Handle;

typedef struct Generator {
	Oop class;
	jmp_buf jmploc;
	CompilerError *error;
	List *temporaries; // names of temps, in order of appearance
	List *instvars;    // names of instvars, in order they were defined
	List *literals;    // literal frame for the compiled code
} Generator;

typedef struct Bytecode {
	st_uchar *buffer;
	st_uint size;
	st_uint alloc;
	st_uint max_stack_depth;
} Bytecode;

typedef struct Cell {
	Oop object;
	st_uint hash;
} Cell;

typedef struct IdentityHashTable {
	Cell *table;
	st_uint alloc;
	st_uint size;
	st_uint deleted;
	st_uint current_hash;
} IdentityHashTable;

typedef struct MemHeap {
	st_uchar *start;        // start of reserved address space
	st_uchar *p;            // end of committed address space (`start' to `p' is thus writeable memory)
	st_uchar *end;          // end of reserved address space
} MemHeap;

typedef struct ObjMemory {
	MemHeap *heap;
	Oop *start, *end;
	Oop *p;
	Oop *mark_stack;
	st_uint mark_stack_size;
	st_uchar *mark_bits;
	st_uchar *alloc_bits;
	st_uint bits_size;                   // in bytes
	Oop **offsets;
	st_uint offsets_size;                // in bytes
	ptr_array roots;
	st_uint counter;
	Oop free_context;                 // free context pool
	struct timespec total_pause_time;    // total accumulated pause time
	st_ulong bytes_allocated;            // current number of allocated bytes
	st_ulong bytes_collected;            // number of bytes collected in last compaction
	
	IdentityHashTable *ht;
	
} ObjMemory;


typedef struct LargeInt {
	ObjHeader parent;
	mp_int value;
} LargeInt;
