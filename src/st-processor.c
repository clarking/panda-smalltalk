
#include "st-types.h"
#include "st-compiler.h"
#include "st-universe.h"
#include "st-hashed-collection.h"
#include "st-symbol.h"
#include "st-object.h"
#include "st-behavior.h"
#include "st-context.h"
#include "st-primitives.h"
#include "st-method.h"
#include "st-array.h"
#include "st-association.h"

#include <stdlib.h>
#include <setjmp.h>

static st_oop 
method_context_new (st_processor *pr)
{
    st_oop  context;
    int     stack_size;
    st_oop *stack;

    stack_size = 32;

    context = st_memory_allocate (ST_SIZE_OOPS (struct st_method_context) + stack_size);
    st_object_initialize_header (context, st_method_context_class);

    ST_CONTEXT_PART (context)->sender     = pr->context;
    ST_CONTEXT_PART (context)->ip         = st_smi_new (0);
    ST_CONTEXT_PART (context)->sp         = st_smi_new (0);
    ST_METHOD_CONTEXT (context)->receiver = pr->message_receiver;
    ST_METHOD_CONTEXT (context)->method   = pr->new_method;

    stack = ST_METHOD_CONTEXT (context)->stack;
    for (st_uint i=0; i < stack_size; i++)
	stack[i] = st_nil;

    return context;
}

static st_oop
block_context_new (st_processor *pr, st_uint initial_ip, st_uint argcount)
{
    st_oop  home;
    st_oop  context;
    st_oop  method;
    st_oop *stack;
    st_uint stack_size;

    stack_size = 32;
    
    context = st_memory_allocate (ST_SIZE_OOPS (struct st_block_context) + stack_size);
    st_object_initialize_header (context, st_block_context_class);
    
    if (ST_HEADER (pr->context)->class == st_block_context_class)
	home = ST_BLOCK_CONTEXT (pr->context)->home;
    else
	home = pr->context;

    ST_CONTEXT_PART (context)->sender = st_nil;
    ST_CONTEXT_PART (context)->ip     = st_smi_new (0);
    ST_CONTEXT_PART (context)->sp     = st_smi_new (0);

    ST_BLOCK_CONTEXT (context)->initial_ip = st_smi_new (initial_ip);
    ST_BLOCK_CONTEXT (context)->argcount   = st_smi_new (argcount);
    ST_BLOCK_CONTEXT (context)->caller     = st_nil;
    ST_BLOCK_CONTEXT (context)->home       = home;

    stack = ST_BLOCK_CONTEXT (context)->stack;
    for (st_uint i=0; i < stack_size; i++)
	stack[i] = st_nil;
    
    return context;
}

static void
create_actual_message (st_processor *pr)
{
    st_oop *elements;
    st_oop array;

    array = st_object_new_arrayed (st_array_class, pr->message_argcount);
    elements = st_array_elements (array);
    for (st_uint i = 0; i < pr->message_argcount; i++)
	elements[i] =  pr->stack[pr->sp - pr->message_argcount + i];

    pr->sp -= pr->message_argcount;

    ST_STACK_PUSH (pr, st_message_new (pr->message_selector, array));

    pr->message_selector = st_selector_doesNotUnderstand;
    pr->message_argcount = 1;
}

static inline st_oop
lookup_method (st_processor *pr, st_oop class)
{
    st_oop method;
    st_oop parent = class;
    st_uint index;

    index = ST_METHOD_CACHE_HASH (class, pr->message_selector);

    if (pr->method_cache[index].class    == class &&
	pr->method_cache[index].selector == pr->message_selector)
	return pr->method_cache[index].method;

    while (parent != st_nil) {
	method = st_dictionary_at (ST_BEHAVIOR (parent)->method_dictionary, pr->message_selector);
	if (method != st_nil) {
	    index = ST_METHOD_CACHE_HASH (parent, pr->message_selector);
	    pr->method_cache[index].class = class;
	    pr->method_cache[index].selector = pr->message_selector;
	    pr->method_cache[index].method = method;
	    return method;
	}
	parent = ST_BEHAVIOR (parent)->superclass;
    }

    if (pr->message_selector == st_selector_doesNotUnderstand) {
	fprintf (stderr, "no method found for #doesNotUnderstand:");
	exit(1);
    }

    create_actual_message (pr);

    return lookup_method (pr, class);
}

st_oop
st_processor_lookup_method (st_processor *pr, st_oop class)
{
    return lookup_method (pr, class);
}

/* 
 * Creates a new method context. Parameterised by
 * @sender, @receiver, @method, and @argcount
 *
 * Message arguments are copied into the new context's temporary
 * frame. Receiver and arguments are then popped off the stack.
 *
 */
static inline void
activate_method (st_processor *pr)
{
    st_oop  context;
    st_oop *arguments;
    
    context = method_context_new (pr);

    arguments = ST_METHOD_CONTEXT_TEMPORARY_FRAME (context);
    for (st_uint i = 0; i < pr->message_argcount; i++)
	arguments[i] =  pr->stack[pr->sp - pr->message_argcount + i];

    pr->sp -= pr->message_argcount + 1;

    st_processor_set_active_context (pr, context);
}

void
st_processor_execute_method (st_processor *pr)
{
    st_uint primitive_index;
    st_method_flags flags;

    flags = st_method_get_flags (pr->new_method);
    if (flags == ST_METHOD_PRIMITIVE) {
	primitive_index = st_method_get_primitive_index (pr->new_method);
	pr->success = true;
	st_primitives[primitive_index].func (pr);
	if (ST_LIKELY (pr->success))
	    return;
    }
    
    activate_method (pr);
}


void
st_processor_set_active_context (st_processor *pr,
				 st_oop  context)
{
    st_oop home;

    /* save executation state of active context */
    if (pr->context != st_nil) {
	ST_CONTEXT_PART (pr->context)->ip = st_smi_new (pr->ip);
	ST_CONTEXT_PART (pr->context)->sp = st_smi_new (pr->sp);
    }
    
    if (st_object_class (context) == st_block_context_class) {

	home = ST_BLOCK_CONTEXT (context)->home;

	pr->method   = ST_METHOD_CONTEXT (home)->method;
	pr->receiver = ST_METHOD_CONTEXT (home)->receiver;
	pr->literals = st_array_elements (ST_METHOD (pr->method)->literals);
	pr->temps    = ST_METHOD_CONTEXT_TEMPORARY_FRAME (home);
	pr->stack    = ST_BLOCK_CONTEXT (context)->stack;
    } else {
	pr->method   = ST_METHOD_CONTEXT (context)->method;
	pr->receiver = ST_METHOD_CONTEXT (context)->receiver;
	pr->literals = st_array_elements (ST_METHOD (pr->method)->literals);
	pr->temps    = ST_METHOD_CONTEXT_TEMPORARY_FRAME (context);
	pr->stack    = ST_METHOD_CONTEXT_STACK (context);
    }

    pr->context  = context;
    pr->sp       = st_smi_value (ST_CONTEXT_PART (context)->sp);
    pr->ip       = st_smi_value (ST_CONTEXT_PART (context)->ip);
    pr->bytecode = st_method_bytecode_bytes (pr->method);

}

void
st_processor_prologue (st_processor *pr)
{
    if (st_verbose_mode ()) {
	fprintf (stderr, "** gc: totalPauseTime: %.6fs\n", st_timespec_to_double_seconds (&memory->total_pause_time));
    }
}

void
st_processor_send_selector (st_processor *pr,
			    st_oop        selector,
			    st_uint       argcount)
{
    st_oop method;
    st_uint primitive_index;
    st_method_flags flags;

    pr->message_argcount = argcount;
    pr->message_receiver = pr->stack[pr->sp - argcount - 1];
    pr->message_selector = selector;
    
    pr->new_method = st_processor_lookup_method (pr, st_object_class (pr->message_receiver));
    
    flags = st_method_get_flags (pr->new_method);
    if (flags == ST_METHOD_PRIMITIVE) {
	primitive_index = st_method_get_primitive_index (pr->new_method);
	pr->success = true;
	st_primitives[primitive_index].func (pr);
	if (ST_LIKELY (pr->success))
	    return;
    }

    activate_method (pr);
}

#define SEND_SELECTOR(pr, selector, argcount)				\
    pr->ip = ip - pr->bytecode;						\
    st_processor_send_selector (pr, selector, argcount);		\
    ip = pr->bytecode + pr->ip;

#define ACTIVATE_CONTEXT(pr, context)			\
    pr->ip = ip - pr->bytecode;				\
    st_processor_set_active_context (pr, context);	\
    ip = pr->bytecode + pr->ip;

#define ACTIVATE_METHOD(pr)				\
    pr->ip = ip - pr->bytecode;				\
    activate_method (pr);				\
    ip = pr->bytecode + pr->ip;

#define EXECUTE_PRIMITIVE(pr, index)			\
    pr->success = true;					\
    pr->ip = ip - pr->bytecode;				\
    st_primitives[index].func (pr);			\
    ip = pr->bytecode + pr->ip;

#define SEND_TEMPLATE(pr)						\
    st_uint  prim;							\
    st_method_flags flags;						\
    									\
    ip += 1;								\
    									\
    pr->new_method = st_processor_lookup_method (pr, st_object_class (pr->message_receiver));\
    									\
    flags = st_method_get_flags (pr->new_method);			\
    if (flags == ST_METHOD_PRIMITIVE) {					\
	prim = st_method_get_primitive_index (pr->new_method);		\
									\
	EXECUTE_PRIMITIVE (pr, prim);					\
	if (ST_LIKELY (pr->success))					\
            NEXT ();							\
    }									\
    									\
    ACTIVATE_METHOD (pr);


#ifdef __GNUC__
#define I_HAS_COMPUTED_GOTO
#endif

#ifdef I_HAS_COMPUTED_GOTO
#define SWITCH(ip)                              \
static const st_pointer labels[] =		\
{						\
    NULL,					\
    && PUSH_TEMP,          && PUSH_INSTVAR,     \
    && PUSH_LITERAL_CONST, && PUSH_LITERAL_VAR,			\
    								\
    && STORE_LITERAL_VAR, && STORE_TEMP, && STORE_INSTVAR,	       \
    && STORE_POP_LITERAL_VAR, && STORE_POP_TEMP, && STORE_POP_INSTVAR, \
    								       \
    && PUSH_SELF, && PUSH_NIL, && PUSH_TRUE, && PUSH_FALSE,            \
                                                                       \
    && RETURN_STACK_TOP, && BLOCK_RETURN,       \
    && POP_STACK_TOP, && DUPLICATE_STACK_TOP,   \
                                                \
    && PUSH_ACTIVE_CONTEXT, && BLOCK_COPY,      \
                                                \
    && JUMP_TRUE, && JUMP_FALSE, && JUMP,       \
                                                \
    && SEND, && SEND_SUPER,                     \
                                                \
    && SEND_PLUS,	&& SEND_MINUS,          \
    && SEND_LT,     && SEND_GT,                 \
    && SEND_LE,     && SEND_GE,                 \
    && SEND_EQ,     && SEND_NE,                 \
    && SEND_MUL,    && SEND_DIV,                \
    && SEND_MOD, 	&& SEND_BITSHIFT,       \
    && SEND_BITAND,	&& SEND_BITOR,          \
    && SEND_BITXOR,                             \
                                                \
    && SEND_AT,        && SEND_AT_PUT,          \
    && SEND_SIZE,      && SEND_VALUE,           \
    && SEND_VALUE_ARG, && SEND_IDENTITY_EQ,     \
    && SEND_CLASS,     && SEND_NEW,             \
    && SEND_NEW_ARG,                            \
};                                              \
goto *labels[*ip];
#else
#define SWITCH(ip) \
start:             \
switch (*ip)
#endif

#ifdef I_HAS_COMPUTED_GOTO
#define CASE(OP) OP:              
#else
#define CASE(OP) case OP:
#endif

#ifdef I_HAS_COMPUTED_GOTO
#define NEXT() goto *labels[*ip]
#else
#define NEXT() goto start
#endif


void
st_processor_main (st_processor *pr)
{
    register const st_uchar *ip;

    if (setjmp (pr->main_loop))
	goto out;

    ip = pr->bytecode + pr->ip;

    SWITCH (ip) {

	CASE (PUSH_TEMP) {

	    ST_STACK_PUSH (pr, pr->temps[ip[1]]);
	    
	    ip += 2;
	    NEXT ();
	}

	CASE (PUSH_INSTVAR) {
    
	    ST_STACK_PUSH (pr, ST_HEADER (pr->receiver)->fields[ip[1]]);
	    
	    ip += 2;
	    NEXT ();
	}

	CASE (STORE_POP_INSTVAR) {
	    
	    ST_HEADER (pr->receiver)->fields[ip[1]] = ST_STACK_POP (pr);
	    
	    ip += 2;
	    NEXT ();
	}

	CASE (STORE_INSTVAR) {
    
	    ST_HEADER (pr->receiver)->fields[ip[1]] = ST_STACK_PEEK (pr);
	    
	    ip += 2;
	    NEXT ();
	}

	CASE (STORE_POP_TEMP) {
	    
	    pr->temps[ip[1]] = ST_STACK_POP (pr);
	    
	    ip += 2;
	    NEXT ();
	}    

	CASE (STORE_TEMP) {
	    
	    pr->temps[ip[1]] = ST_STACK_PEEK (pr);
	    
	    ip += 2;
	    NEXT ();
	}    
	
	CASE (STORE_LITERAL_VAR) {
	    
	    ST_ASSOCIATION (pr->literals[ip[1]])->value = ST_STACK_PEEK (pr);
	    
	    ip += 2;
	    NEXT ();
	}
	    
	CASE (STORE_POP_LITERAL_VAR) {
	    
	    ST_ASSOCIATION (pr->literals[ip[1]])->value = ST_STACK_POP (pr);
	    
	    ip += 2;
	    NEXT ();
	}
	
	CASE (PUSH_SELF) {
    
	    ST_STACK_PUSH (pr, pr->receiver);

	    ip += 1;
	    NEXT ();
	}

	CASE (PUSH_TRUE) {
    
	    ST_STACK_PUSH (pr, st_true);
	    
	    ip += 1;
	    NEXT ();
	}

	CASE (PUSH_FALSE) {
	    
	    ST_STACK_PUSH (pr, st_false);
	
	    ip += 1;
	    NEXT ();
	}

	CASE (PUSH_NIL) {
    
	    ST_STACK_PUSH (pr, st_nil);
	    
	    ip += 1;
	    NEXT ();
	}
	
	CASE (PUSH_ACTIVE_CONTEXT) {
    
	    ST_STACK_PUSH (pr, pr->context);
	    
	    ip += 1;
	    NEXT ();
	}

	CASE (PUSH_LITERAL_CONST) {
    
	    ST_STACK_PUSH (pr, pr->literals[ip[1]]);
	    
	    ip += 2;
	    NEXT ();
	}

	CASE (PUSH_LITERAL_VAR) {
	    
	    st_oop var;
	    
	    var = ST_ASSOCIATION (pr->literals[ip[1]])->value;
	    
	    ST_STACK_PUSH (pr, var);	    
	    
	    ip += 2;
	    NEXT ();
	}
	    
	CASE (JUMP_TRUE) {
	    
	    if (ST_STACK_PEEK (pr) == st_true) {
		(void) ST_STACK_POP (pr);
		ip += 3 + ((ip[1] << 8) | ip[2]);
	    } else if (ST_STACK_PEEK (pr) == st_false) {
		(void) ST_STACK_POP (pr);
		ip += 3;
	    } else {
		ip += 3;
		SEND_SELECTOR (pr, st_selector_mustBeBoolean, 0);
	    }
	    
	    NEXT ();
	}

	CASE (JUMP_FALSE) {
	    
	    if (ST_STACK_PEEK (pr) == st_false) {
		(void) ST_STACK_POP (pr);
		ip += 3 + ((ip[1] << 8) | ip[2]);
	    } else if (ST_STACK_PEEK (pr) == st_true) {
		(void) ST_STACK_POP (pr);
		ip += 3;
	    } else {
		ip += 3;
		SEND_SELECTOR (pr, st_selector_mustBeBoolean, 0);
	    }
	    
	    NEXT ();
	}
    
	CASE (JUMP) {
	    
	    short offset =  ((ip[1] << 8) | ip[2]);    
	    
	    ip += ((offset >= 0) ? 3 : 0) + offset;
	    
	    NEXT ();    
	}
	
	CASE (SEND_PLUS) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_PLUS];	    
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];

	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_MINUS) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_MINUS];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];

	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
    
	CASE (SEND_MUL) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_MUL];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	
	}
    
	CASE (SEND_MOD) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_MOD];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	    
	}
    
	CASE (SEND_DIV) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_DIV];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_BITSHIFT) {
    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_BITSHIFT];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();	    
	}
	
	CASE (SEND_BITAND) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_BITAND];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();	    
	}
	
	CASE (SEND_BITOR) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_BITOR];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();	    
	}
	
	CASE (SEND_BITXOR) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_BITXOR];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_LT) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_LT];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_GT) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_GT];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	
	    NEXT ();
	}
	
	CASE (SEND_LE) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_LE];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();	    
	}
	
	CASE (SEND_GE) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_GE];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_CLASS) {

	    pr->message_argcount = 0;
	    pr->message_selector = st_specials[ST_SPECIAL_CLASS];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	
	    NEXT ();
	}
	
	CASE (SEND_SIZE) {
	    
	    pr->message_argcount = 0;
	    pr->message_selector = st_specials[ST_SPECIAL_SIZE];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
    
	CASE (SEND_AT) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_AT];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
    
	CASE (SEND_AT_PUT) {
	    
	    pr->message_argcount = 2;
	    pr->message_selector = st_specials[ST_SPECIAL_ATPUT];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_EQ) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_EQ];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_NE) {

	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_NE];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	
	    NEXT ();
	}
	
	CASE (SEND_IDENTITY_EQ) {

	    st_oop a, b;
	    a = ST_STACK_POP (pr);
	    b = ST_STACK_POP (pr);
	    
	    ST_STACK_PUSH (pr, (a == b) ? st_true : st_false);

	    ip += 1;
	    NEXT ();
	}
	
	CASE (SEND_VALUE) {
    
	    pr->message_argcount = 0;
	    pr->message_selector = st_specials[ST_SPECIAL_VALUE];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
    
	CASE (SEND_VALUE_ARG) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_VALUE_ARG];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_NEW) {
    
	    pr->message_argcount = 0;
	    pr->message_selector = st_specials[ST_SPECIAL_NEW];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND_NEW_ARG) {
	    
	    pr->message_argcount = 1;
	    pr->message_selector = st_specials[ST_SPECIAL_NEW_ARG];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    SEND_TEMPLATE (pr);
	    
	    NEXT ();
	}
	
	CASE (SEND) {
	    
	    st_uint  primitive_index;
	    st_method_flags flags;

	    pr->message_argcount = ip[1];
	    pr->message_selector = pr->literals[ip[2]];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];

	    ip += 3;

	    pr->new_method = st_processor_lookup_method (pr, st_object_class (pr->message_receiver));
	    
	    flags = st_method_get_flags (pr->new_method);
	    if (flags == ST_METHOD_PRIMITIVE) {
		primitive_index = st_method_get_primitive_index (pr->new_method);
		
		EXECUTE_PRIMITIVE (pr, primitive_index);
		if (ST_LIKELY (pr->success))
		    NEXT ();
	    }
	    
	    ACTIVATE_METHOD (pr);
	    NEXT ();
	}
    
	CASE (SEND_SUPER) {
	    
	    st_oop literal_index;
	    st_uint  primitive_index;
	    st_method_flags flags;
	    
	    pr->message_argcount = ip[1];
	    pr->message_selector = pr->literals[ip[2]];
	    pr->message_receiver = pr->stack[pr->sp - pr->message_argcount - 1];
	    
	    ip += 3;
	    
	    literal_index = st_smi_value (st_arrayed_object_size (ST_METHOD (pr->method)->literals)) - 1;

	    pr->new_method = st_processor_lookup_method (pr, ST_BEHAVIOR (pr->literals[literal_index])->superclass);
	    
	    flags = st_method_get_flags (pr->new_method);
	    if (flags == ST_METHOD_PRIMITIVE) {
		primitive_index = st_method_get_primitive_index (pr->new_method);
		
		EXECUTE_PRIMITIVE (pr, primitive_index);
		if (ST_LIKELY (pr->success))
		    NEXT ();
	    }
	    
	    ACTIVATE_METHOD (pr);
	    NEXT ();
	}
	
	CASE (POP_STACK_TOP) {
	    
	    (void) ST_STACK_POP (pr);
	    
	    ip += 1;
	    NEXT ();
	}

	CASE (DUPLICATE_STACK_TOP) {
	    
	    ST_STACK_PUSH (pr, ST_STACK_PEEK (pr));
	    
	    ip += 1;
	    NEXT ();
	}
	
	CASE (BLOCK_COPY) {
	    
	    st_oop block;
	    st_oop home;
	    st_uint argcount = ip[1];
	    st_uint initial_ip;
	    
	    ip += 2;
	    
	    initial_ip = ip - pr->bytecode + 3;
	    
	    block = block_context_new (pr, initial_ip, argcount);
	    
	    ST_STACK_PUSH (pr, block);
	    
	    NEXT ();
	}
	
	CASE (RETURN_STACK_TOP) {
	    
	    st_oop sender;
	    st_oop value;
	    
	    value = ST_STACK_PEEK (pr);
	    
	    if (ST_HEADER (pr->context)->class == st_block_context_class)
		sender = ST_CONTEXT_PART (ST_BLOCK_CONTEXT (pr->context)->home)->sender;
	    else
		sender = ST_CONTEXT_PART (pr->context)->sender;
	    
	    st_assert (st_object_is_heap (sender));

	    if (sender == st_nil) {
		ST_STACK_PUSH (pr, pr->context);
		ST_STACK_PUSH (pr, value);
		SEND_SELECTOR (pr, st_selector_cannotReturn, 1);
		NEXT ();
	    }

	    ACTIVATE_CONTEXT (pr, sender);
	    ST_STACK_PUSH (pr, value);

	    NEXT ();
	}

	CASE (BLOCK_RETURN) {
	    
	    st_oop caller;
	    st_oop value;
	    
	    caller = ST_BLOCK_CONTEXT (pr->context)->caller;
	    value = ST_STACK_PEEK (pr);
	    ACTIVATE_CONTEXT (pr, caller);
	    
	    /* push returned value onto caller's stack */
	    ST_STACK_PUSH (pr, value);
	    st_assert (pr->context == caller);
	    
	    NEXT ();
	}
    }

out:
    st_processor_prologue (pr);
}

void
st_processor_clear_caches (st_processor *pr)
{
    for (st_uint i = 0; i < ST_METHOD_CACHE_SIZE; i++) {
	pr->method_cache[i].class    = st_nil;
	pr->method_cache[i].selector = st_nil;
	pr->method_cache[i].method   = st_nil;
    }
}

void
st_processor_initialize (st_processor *pr)
{
    st_oop context;
    st_oop method;

    /* clear contents */
    memset (pr, 0, sizeof (st_processor));
    pr->context = st_nil;
    pr->receiver = st_nil;
    pr->method = st_nil;

    pr->ip = 0;
    pr->sp = 0;
    pr->stack = NULL;

    pr->message_argcount = 0;
    pr->message_receiver = st_nil;
    pr->message_selector = st_selector_startupSystem;
    st_processor_clear_caches (pr);

    pr->new_method = st_processor_lookup_method (pr, st_object_class (pr->message_receiver));
    st_assert (st_method_get_flags (pr->new_method) == ST_METHOD_NORMAL);

    context = method_context_new (pr);
    st_processor_set_active_context (pr, context);
}
