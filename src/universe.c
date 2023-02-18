
/*
 * Copyright (C) 2008 Vincent Geddes
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: MIT
 */


#include <string.h>

#include "st-types.h"
#include "st-utils.h"
#include "st-object.h"
#include "st-behavior.h"
#include "st-array.h"
#include "st-small-integer.h"
#include "st-dictionary.h"
#include "st-symbol.h"
#include "st-universe.h"
#include "st-compiler.h"
#include "st-memory.h"
#include "st-parser.h"
#include "st-machine.h"

static bool verbose_mode = false;

st_memory *memory = NULL;

st_oop st_global_get(const char *name) {
	st_assert (name != NULL);
	return st_dictionary_at(ST_GLOBALS, st_symbol_new(name));
}

enum {
	INSTANCE_SIZE_UNDEFINED = 0,
	INSTANCE_SIZE_CLASS = 6,
	INSTANCE_SIZE_METACLASS = 6,
	INSTANCE_SIZE_DICTIONARY = 3,
	INSTANCE_SIZE_SET = 3,
	INSTANCE_SIZE_ASSOCIATION = 2,
	INSTANCE_SIZE_SYSTEM = 2,
	INSTANCE_SIZE_METHOD_CONTEXT = 5,
	INSTANCE_SIZE_BLOCK_CONTEXT = 6
};

st_oop class_new(st_format format, st_uint instance_size) {
	st_oop class;
	
	class = st_memory_allocate(ST_SIZE_OOPS (struct st_class));
	
	ST_OBJECT_MARK (class) = 0 | ST_MARK_TAG;
	ST_OBJECT_CLASS (class) = ST_NIL;
	st_object_set_format(class, ST_FORMAT_OBJECT);
	st_object_set_instance_size(class, INSTANCE_SIZE_CLASS);
	
	ST_BEHAVIOR_FORMAT (class) = st_smi_new(format);
	ST_BEHAVIOR_INSTANCE_SIZE (class) = st_smi_new(instance_size);
	ST_BEHAVIOR_SUPERCLASS (class) = ST_NIL;
	ST_BEHAVIOR_METHOD_DICTIONARY (class) = ST_NIL;
	ST_BEHAVIOR_INSTANCE_VARIABLES (class) = ST_NIL;
	ST_CLASS (class)->name = ST_NIL;
	return class;
}

void add_global(const char *name, st_oop object) {
	st_oop symbol;
	
	// sanity check for symbol interning
	st_assert (st_symbol_new(name) == st_symbol_new(name));
	
	symbol = st_symbol_new(name);
	st_dictionary_at_put(ST_GLOBALS, symbol, object);
	
	// sanity check for dictionary
	st_assert (st_dictionary_at(ST_GLOBALS, symbol) == object);
}

void initialize_class(const char *name, const char *super_name, st_list *ivarnames) {
	st_oop metaclass, class, superclass;
	st_oop names;
	st_uint i = 1;
	
	if (streq (name, "Object") && streq (super_name, "nil")) {
		
		class = st_dictionary_at(ST_GLOBALS, st_symbol_new("Object"));
		st_assert (class != ST_NIL);
		
		metaclass = st_object_class(class);
		if (metaclass == ST_NIL) {
			metaclass = st_object_new(ST_METACLASS_CLASS);
			ST_OBJECT_CLASS (class) = metaclass;
		}
		
		ST_BEHAVIOR_SUPERCLASS (class) = ST_NIL;
		ST_BEHAVIOR_INSTANCE_SIZE (class) = st_smi_new(0);
		ST_BEHAVIOR_SUPERCLASS (metaclass) = st_dictionary_at(ST_GLOBALS, st_symbol_new("Class"));
		
	} else {
		superclass = st_global_get(super_name);
		if (superclass == ST_NIL)
			st_assert (superclass != ST_NIL);
		
		class = st_global_get(name);
		if (class == ST_NIL)
			class = class_new(st_smi_value(ST_BEHAVIOR_FORMAT (superclass)), 0);
		
		metaclass = ST_HEADER (class)->class;
		if (metaclass == ST_NIL) {
			metaclass = st_object_new(ST_METACLASS_CLASS);
			ST_OBJECT_CLASS (class) = metaclass;
		}
		
		ST_BEHAVIOR_SUPERCLASS (class) = superclass;
		ST_BEHAVIOR_SUPERCLASS (metaclass) = ST_HEADER (superclass)->class;
		ST_BEHAVIOR_INSTANCE_SIZE (class) = st_smi_new(
				st_list_length(ivarnames) + st_smi_value(ST_BEHAVIOR_INSTANCE_SIZE (superclass)));
	}
	
	names = ST_NIL;
	if (st_list_length(ivarnames) != 0) {
		names = st_object_new_arrayed(ST_ARRAY_CLASS, st_list_length(ivarnames));
		for (st_list *l = ivarnames; l; l = l->next)
			st_array_at_put(names, i++, st_symbol_new(l->data));
		ST_BEHAVIOR_INSTANCE_VARIABLES (class) = names;
	}
	
	ST_BEHAVIOR_FORMAT (metaclass) = st_smi_new(ST_FORMAT_OBJECT);
	ST_BEHAVIOR_METHOD_DICTIONARY (metaclass) = st_dictionary_new();
	ST_BEHAVIOR_INSTANCE_VARIABLES (metaclass) = ST_NIL;
	ST_BEHAVIOR_INSTANCE_SIZE (metaclass) = st_smi_new(INSTANCE_SIZE_CLASS);
	ST_METACLASS_INSTANCE_CLASS (metaclass) = class;
	ST_BEHAVIOR_INSTANCE_VARIABLES (class) = names;
	ST_BEHAVIOR_METHOD_DICTIONARY (class) = st_dictionary_new();
	ST_CLASS_NAME (class) = st_symbol_new(name);
	
	st_dictionary_at_put(ST_GLOBALS, st_symbol_new(name), class);
}


void file_in_classes(void) {
	char *filename;
	
	parse_classes("../st/class-defs.st");
	
	static const char *files[] = {
			"Stream.st",
			"PositionableStream.st",
			"WriteStream.st",
			"Collection.st",
			"SequenceableCollection.st",
			"ArrayedCollection.st",
			"HashedCollection.st",
			"Set.st",
			"Dictionary.st",
			"IdentitySet.st",
			"IdentityDictionary.st",
			"Bag.st",
			"Array.st",
			"ByteArray.st",
			"WordArray.st",
			"FloatArray.st",
			"Association.st",
			"Magnitude.st",
			"Number.st",
			"Integer.st",
			"SmallInteger.st",
			"LargeInteger.st",
			"Fraction.st",
			"Float.st",
			"Object.st",
			"UndefinedObject.st",
			"String.st",
			"Symbol.st",
			"ByteString.st",
			"WideString.st",
			"Character.st",
			"Behavior.st",
			"Boolean.st",
			"True.st",
			"False.st",
			"Behavior.st",
			"ContextPart.st",
			"BlockContext.st",
			"Message.st",
			"OrderedCollection.st",
			"List.st",
			"System.st",
			"CompiledMethod.st",
			"FileStream.st",
			"pidigits.st"
	};
	
	for (st_uint i = 0; i < ST_N_ELEMENTS (files); i++) {
		filename = st_strconcat("..", ST_DIR_SEPARATOR_S, "st", ST_DIR_SEPARATOR_S, files[i], NULL);
		compile_file_in(filename);
		st_free(filename);
	}
}

st_oop create_nil_object(void) {
	st_oop nil;
	nil = st_memory_allocate(NIL_SIZE_OOPS);
	ST_OBJECT_MARK (nil) = 0 | ST_MARK_TAG;
	ST_OBJECT_CLASS (nil) = nil;
	st_object_set_format(nil, ST_FORMAT_OBJECT);
	st_object_set_instance_size(nil, 0);
	return nil;
}

void init_specials(void) {
	ST_SELECTOR_PLUS = st_symbol_new("+");
	ST_SELECTOR_MINUS = st_symbol_new("-");
	ST_SELECTOR_LT = st_symbol_new("<");
	ST_SELECTOR_GT = st_symbol_new(">");
	ST_SELECTOR_LE = st_symbol_new("<=");
	ST_SELECTOR_GE = st_symbol_new(">=");
	ST_SELECTOR_EQ = st_symbol_new("=");
	ST_SELECTOR_NE = st_symbol_new("~=");
	ST_SELECTOR_MUL = st_symbol_new("*");
	ST_SELECTOR_DIV = st_symbol_new("/");
	ST_SELECTOR_MOD = st_symbol_new("\\");
	ST_SELECTOR_BITSHIFT = st_symbol_new("bitShift:");
	ST_SELECTOR_BITAND = st_symbol_new("bitAnd:");
	ST_SELECTOR_BITOR = st_symbol_new("bitOr:");
	ST_SELECTOR_BITXOR = st_symbol_new("bitXor:");
	ST_SELECTOR_AT = st_symbol_new("at:");
	ST_SELECTOR_ATPUT = st_symbol_new("at:put:");
	ST_SELECTOR_SIZE = st_symbol_new("size");
	ST_SELECTOR_VALUE = st_symbol_new("value");
	ST_SELECTOR_VALUE_ARG = st_symbol_new("value:");
	ST_SELECTOR_IDEQ = st_symbol_new("==");
	ST_SELECTOR_CLASS = st_symbol_new("class");
	ST_SELECTOR_NEW = st_symbol_new("new");
	ST_SELECTOR_NEW_ARG = st_symbol_new("new:");
	ST_SELECTOR_DOESNOTUNDERSTAND = st_symbol_new("doesNotUnderstand:");
	ST_SELECTOR_MUSTBEBOOLEAN = st_symbol_new("mustBeBoolean");
	ST_SELECTOR_STARTUPSYSTEM = st_symbol_new("startupSystem");
	ST_SELECTOR_CANNOTRETURN = st_symbol_new("cannotReturn");
	ST_SELECTOR_OUTOFMEMORY = st_symbol_new("outOfMemory");
}

void bootstrap_universe(void) {
	st_oop st_object_class_, st_class_class_;
	st_memory_new();
	ST_NIL = create_nil_object();
	st_object_class_ = class_new(ST_FORMAT_OBJECT, 0);
	ST_UNDEFINED_OBJECT_CLASS = class_new(ST_FORMAT_OBJECT, 0);
	ST_METACLASS_CLASS = class_new(ST_FORMAT_OBJECT, INSTANCE_SIZE_METACLASS);
	ST_BEHAVIOR_CLASS = class_new(ST_FORMAT_OBJECT, 0);
	st_class_class_ = class_new(ST_FORMAT_OBJECT, INSTANCE_SIZE_CLASS);
	ST_SMI_CLASS = class_new(ST_FORMAT_OBJECT, 0);
	ST_LARGE_INTEGER_CLASS = class_new(ST_FORMAT_LARGE_INTEGER, 0);
	ST_CHARACTER_CLASS = class_new(ST_FORMAT_OBJECT, 0);
	ST_TRUE_CLASS = class_new(ST_FORMAT_OBJECT, 0);
	ST_FALSE_CLASS = class_new(ST_FORMAT_OBJECT, 0);
	ST_FLOAT_CLASS = class_new(ST_FORMAT_FLOAT, 0);
	ST_ARRAY_CLASS = class_new(ST_FORMAT_ARRAY, 0);
	ST_WORD_ARRAY_CLASS = class_new(ST_FORMAT_WORD_ARRAY, 0);
	ST_FLOAT_ARRAY_CLASS = class_new(ST_FORMAT_FLOAT_ARRAY, 0);
	ST_DICTIONARY_CLASS = class_new(ST_FORMAT_OBJECT, INSTANCE_SIZE_DICTIONARY);
	ST_SET_CLASS = class_new(ST_FORMAT_OBJECT, INSTANCE_SIZE_SET);
	ST_BYTE_ARRAY_CLASS = class_new(ST_FORMAT_BYTE_ARRAY, 0);
	ST_SYMBOL_CLASS = class_new(ST_FORMAT_BYTE_ARRAY, 0);
	ST_STRING_CLASS = class_new(ST_FORMAT_BYTE_ARRAY, 0);
	ST_WIDE_STRING_CLASS = class_new(ST_FORMAT_WORD_ARRAY, 0);
	ST_ASSOCIATION_CLASS = class_new(ST_FORMAT_OBJECT, INSTANCE_SIZE_ASSOCIATION);
	ST_COMPILED_METHOD_CLASS = class_new(ST_FORMAT_OBJECT, 0);
	ST_METHOD_CONTEXT_CLASS = class_new(ST_FORMAT_CONTEXT, INSTANCE_SIZE_METHOD_CONTEXT);
	ST_BLOCK_CONTEXT_CLASS = class_new(ST_FORMAT_CONTEXT, INSTANCE_SIZE_BLOCK_CONTEXT);
	ST_SYSTEM_CLASS = class_new(ST_FORMAT_OBJECT, INSTANCE_SIZE_SYSTEM);
	ST_HANDLE_CLASS = class_new(ST_FORMAT_HANDLE, 0);
	ST_MESSAGE_CLASS = class_new(ST_FORMAT_OBJECT, 2);
	ST_OBJECT_CLASS(ST_NIL) = ST_UNDEFINED_OBJECT_CLASS;
	
	ST_TRUE = st_object_new(ST_TRUE_CLASS);
	ST_FALSE = st_object_new(ST_FALSE_CLASS);
	ST_SYMBOLS = st_set_new_with_capacity(256);
	ST_GLOBALS = st_dictionary_new_with_capacity(256);
	ST_SMALLTALK = st_object_new(ST_SYSTEM_CLASS);
	ST_OBJECT_FIELDS(ST_SMALLTALK)[0] = ST_GLOBALS;
	ST_OBJECT_FIELDS(ST_SMALLTALK)[1] = ST_SYMBOLS;
	
	// fill symbol table
	add_global("Object", st_object_class_);
	add_global("UndefinedObject", ST_UNDEFINED_OBJECT_CLASS);
	add_global("Behavior", ST_BEHAVIOR_CLASS);
	add_global("Class", st_class_class_);
	add_global("Metaclass", ST_METACLASS_CLASS);
	add_global("SmallInteger", ST_SMI_CLASS);
	add_global("LargeInteger", ST_LARGE_INTEGER_CLASS);
	add_global("Character", ST_CHARACTER_CLASS);
	add_global("True", ST_TRUE_CLASS);
	add_global("False", ST_FALSE_CLASS);
	add_global("Float", ST_FLOAT_CLASS);
	add_global("Array", ST_ARRAY_CLASS);
	add_global("ByteArray", ST_BYTE_ARRAY_CLASS);
	add_global("WordArray", ST_WORD_ARRAY_CLASS);
	add_global("FloatArray", ST_FLOAT_ARRAY_CLASS);
	add_global("ByteString", ST_STRING_CLASS);
	add_global("ByteSymbol", ST_SYMBOL_CLASS);
	add_global("WideString", ST_WIDE_STRING_CLASS);
	add_global("IdentitySet", ST_SET_CLASS);
	add_global("IdentityDictionary", ST_DICTIONARY_CLASS);
	add_global("Association", ST_ASSOCIATION_CLASS);
	add_global("CompiledMethod", ST_COMPILED_METHOD_CLASS);
	add_global("MethodContext", ST_METHOD_CONTEXT_CLASS);
	add_global("BlockContext", ST_BLOCK_CONTEXT_CLASS);
	add_global("Handle", ST_HANDLE_CLASS);
	add_global("Message", ST_MESSAGE_CLASS);
	add_global("System", ST_SYSTEM_CLASS);
	add_global("Smalltalk", ST_SMALLTALK);
	
	init_specials();
	file_in_classes();
	
	st_memory_add_root(ST_NIL);
	st_memory_add_root(ST_TRUE);
	st_memory_add_root(ST_FALSE);
	st_memory_add_root(ST_SMALLTALK);
}

void st_set_verbose_mode(bool verbose) {
	verbose_mode = verbose;
}

bool st_get_verbose_mode(void) {
	return verbose_mode;
}