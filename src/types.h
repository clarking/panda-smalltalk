
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2023, Aaron Clark Diaz.
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

#define ST_OBJECT_UNUSED_BITS   15
#define ST_OBJECT_HASH_BITS     1
#define ST_OBJECT_SIZE_BITS     8
#define ST_OBJECT_FORMAT_BITS   6
#define ST_OBJECT_FORMAT_SHIFT  ST_TAG_SIZE
#define ST_OBJECT_SIZE_SHIFT    (ST_OBJECT_FORMAT_BITS + ST_OBJECT_FORMAT_SHIFT)
#define ST_OBJECT_HASH_SHIFT    (ST_OBJECT_SIZE_BITS + ST_OBJECT_SIZE_SHIFT)
#define ST_OBJECT_UNUSED_SHIFT  (ST_OBJECT_HASH_BITS + ST_OBJECT_HASH_SHIFT)
#define ST_OBJECT_FORMAT_MASK   (ST_NTH_MASK(ST_OBJECT_FORMAT_BITS))
#define ST_OBJECT_SIZE_MASK     (ST_NTH_MASK(ST_OBJECT_SIZE_BITS))
#define ST_OBJECT_HASH_MASK     (ST_NTH_MASK(ST_OBJECT_HASH_BITS))
#define ST_OBJECT_UNUSED_MASK   (ST_NTH_MASK(ST_OBJECT_UNUSED_BITS))

#define ST_FORMAT_OBJECT        0
#define ST_FORMAT_FLOAT         1
#define ST_FORMAT_LARGE_INTEGER 2
#define ST_FORMAT_HANDLE        3
#define ST_FORMAT_ARRAY         4
#define ST_FORMAT_BYTE_ARRAY    5
#define ST_FORMAT_FLOAT_ARRAY   6
#define ST_FORMAT_INTEGER_ARRAY 7
#define ST_FORMAT_WORD_ARRAY    8
#define ST_FORMAT_CONTEXT       9
#define ST_NUM_FORMATS          10

typedef uint8_t st_format;

/* basic oop pointer:
 * integral type wide enough to hold a C pointer.
 * Can either point to a heap object or contain a smi or Character immediate.
 */

typedef uintptr_t Oop;
typedef uint8_t uchar;

#define st_tag_pointer(p) (((Oop)(p)) + ST_POINTER_TAG)
#define st_detag_pointer(oop) ((Oop *) ((oop) - ST_POINTER_TAG))

typedef struct VmOptions {
	char *src_dir;
	char *script;
	bool verbose;
	uint mode;

} VmOptions;

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
	uint elements[];
} WordArray;

typedef struct FloatArray {
	ArrayedObject parent;
	double elements[];
} FloatArray;

typedef struct ByteArray {
	ArrayedObject parent;
	uchar bytes[];
} ByteArray;

typedef struct Association {
	ObjHeader parent;
	Oop key;
	Oop value;
} Association;

typedef struct HashedCollection {
	ObjHeader header;
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
	void *data;
	List *next;
} List;

typedef struct InputMarker {
	int p;
	int line;
	int column;
} InputMarker;

typedef struct LexInput {
	char *text;
	uint p;        /* current index into text */
	uint n;        /* total number of chars in text */
	uint line;     /* current line number, starting from 1 */
	uint column;   /* current column number, starting from 1 */
	InputMarker InputMarker;
} LexInput;

typedef struct FileIn {
	const char *filename;
	LexInput *input;
	int line;
} FileIn;

typedef struct CompilerError {
	char message[255];
	uint line;
	uint column;
} CompilerError;


#define TOKEN_INVALID           0
#define TOKEN_LPAREN            1
#define TOKEN_RPAREN            2
#define TOKEN_BLOCK_BEGIN       3
#define TOKEN_BLOCK_END         4
#define TOKEN_COMMA             5
#define TOKEN_SEMICOLON         6
#define TOKEN_PERIOD            7
#define TOKEN_RETURN            8
#define TOKEN_COLON             9
#define TOKEN_ASSIGN            10
#define TOKEN_TUPLE_BEGIN       11
#define TOKEN_IDENTIFIER        12
#define TOKEN_CHARACTER_CONST   13
#define TOKEN_STRING_CONST      14
#define TOKEN_NUMBER_CONST      15
#define TOKEN_SYMBOL_CONST      16
#define TOKEN_COMMENT           17
#define TOKEN_BINARY_SELECTOR   18
#define TOKEN_KEYWORD_SELECTOR  19
#define TOKEN_EOF               20

typedef uint8_t TokenType;

typedef struct Token {
	TokenType type;
	uint line;
	uint column;
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


#define ERROR_MISMATCHED_CHAR 0
#define ERROR_NO_VIABLE_ALT_FOR_CHAR      1
#define ERROR_ILLEGAL_CHAR                2
#define ERROR_UNTERMINATED_COMMENT        3
#define ERROR_UNTERMINATED_STRING_LITERAL 4
#define ERROR_INVALID_RADIX        5
#define ERROR_INVALID_CHAR_CONST   6
#define ERROR_NO_ALT_FOR_POUND     7

typedef uint8_t ErrCode;

typedef struct Lexer {
	LexInput *input;
	bool filter_comments;
	bool token_matched;

	// data for next token
	uint line;
	uint column;
	uint start;
	Token *token;

	// error control
	bool failed;
	jmp_buf main_loop;

	// last error information
	ErrCode error_code;
	uint error_line;
	uint error_column;
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
	uint line;
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
	uchar *buffer;
	uint size;
	uint alloc;
	uint max_stack_depth;
} Bytecode;

typedef struct Cell {
	Oop object;
	uint hash;
} Cell;

typedef struct IdentityHashTable {
	Cell *table;
	uint alloc;
	uint size;
	uint deleted;
	uint current_hash;
} IdentityHashTable;

typedef struct MemHeap {
	uchar *start;        // start of reserved address space
	uchar *p;            // end of committed address space (`start' to `p' is thus writeable memory)
	uchar *end;          // end of reserved address space
} MemHeap;

typedef struct ObjMemory {
	MemHeap *heap;
	Oop *start, *end;
	Oop *p;
	Oop *mark_stack;
	uint mark_stack_size;
	uchar *mark_bits;
	uchar *alloc_bits;
	uint bits_size;                   // in bytes
	Oop **offsets;
	uint offsets_size;                // in bytes
	PtrArray roots;
	uint counter;
	Oop free_context;                 // free context pool
	struct timespec total_pause_time;    // total accumulated pause time
	ulong bytes_allocated;            // current number of allocated bytes
	ulong bytes_collected;            // number of bytes collected in last compaction

	IdentityHashTable *ht;

} ObjMemory;


typedef struct LargeInt {
	ObjHeader parent;
	mp_int value;
} LargeInt;
