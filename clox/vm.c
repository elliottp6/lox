#include <stdio.h>
#include <stdarg.h>
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"
#include "memory.h"

VM vm; // global variable!

static void resetStack() {
    vm.stackTop = vm.stack;
}

static void runtimeError( const char* format, ... ) {
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    fputs( "\n", stderr );

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf( stderr, "[line %d] in script\n", line );
    resetStack();
}

void push( Value value ) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek( int distance ) {
    return vm.stackTop[-1 - distance];
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable( &vm.globals );
    initTable( &vm.strings );
}

void freeVM() {
    freeTable( &vm.globals );
    freeTable( &vm.strings );
    freeObjects();
}

static bool isFalsey( Value value ) {
    return IS_NIL (value ) || ( IS_BOOL( value ) && !AS_BOOL( value ) );
}

static InterpretResult run() {
    // if we're tracing, show it
    #ifdef DEBUG_TRACE_EXECUTION
    printf( "== execution trace ==\n" );
    #endif

    // macros
    #define READ_BYTE() (*vm.ip++)
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    #define READ_STRING() AS_STRING(READ_CONSTANT())

    // this macro looks strange, but it's a way to define a block that permits a semicolon at the end
    #define BINARY_OP(valueType, op) \
        do { \
            if( !IS_NUMBER( peek( 0 ) ) || !IS_NUMBER( peek( 1 ) ) ) { \
                runtimeError( "Operands must be numbers." ); \
                return INTERPRET_RUNTIME_ERROR; \
            } \
            double b = AS_NUMBER( pop() ); \
            double a = AS_NUMBER( pop() ); \
            push( valueType( a op b ) ); \
        } while( false )

    // main loop
    for( uint8_t instruction;; ) {
        // trace execution
        #ifdef DEBUG_TRACE_EXECUTION
            // print instruction info
            disassembleInstruction( vm.chunk, (size_t)(vm.ip - vm.chunk->code) );

            // print stack contents
            if( vm.stack < vm.stackTop ) {
                printf( " [" );
                for( Value* slot = vm.stack; slot < vm.stackTop; slot++ )  {
                    if( slot != vm.stack ) printf( ", " );
                    printValue( *slot );
                }
                printf( "]" );
            }

            // newline
            printf( "\n" );
        #endif

        // interpret instruction
        switch( instruction = READ_BYTE() ) {
            case OP_CONSTANT:   push( READ_CONSTANT() ); break;
            case OP_NIL:        push( NIL_VAL ); break;
            case OP_TRUE:       push( BOOL_VAL( true ) ); break;
            case OP_FALSE:      push( BOOL_VAL( false ) ); break;
            case OP_POP:        pop(); break;
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if( !tableGet( &vm.globals, name, &value ) ) {
                    runtimeError( "Undefined variable '%.*s'.", (int)name->len, name->buf );
                    return INTERPRET_RUNTIME_ERROR;
                }
                push( value );
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet( &vm.globals, name, peek(0) );
                pop(); // pop AFTER adding it, just in case a GC is triggered (we want to ensure that string still exists on the stack!)
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push( BOOL_VAL( valuesEqual( a, b ) ) );
                break;
            }
            case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if( IS_STRING( peek(0) ) && IS_STRING( peek(1) ) ) {
                    // TODO: isn't this a potential GC problem since the strings won't exist on the stack (so a concurrent GC could collect them after pop, but before concat?)
                    ObjString* b = AS_STRING( pop() );
                    ObjString* a = AS_STRING( pop() );
                    push( OBJ_VAL( concatStrings( a->buf, a->len, b->buf, b->len ) ) );
                } else if( IS_NUMBER( peek(0) ) && IS_NUMBER( peek(1) ) ) {
                    double b = AS_NUMBER( pop() );
                    double a = AS_NUMBER( pop() );
                    push( NUMBER_VAL( a + b ) );
                } else {
                    runtimeError( "Operands must be two numbers or two strings." );
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL,-); break;
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL,*); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL,/); break;
            case OP_NOT:        push( BOOL_VAL( isFalsey( pop() ) ) ); break;
            case OP_NEGATE:
                if( !IS_NUMBER( peek( 0 ) ) ) {
                    runtimeError( "Operand must be a number." );
                    return INTERPRET_RUNTIME_ERROR;
                }
                push( NUMBER_VAL( -AS_NUMBER( pop() ) ) );
                break;
            case OP_PRINT: printValue( pop() ); printf( "\n" ); break;
            case OP_RETURN: return INTERPRET_OK; // exit interpreter
        }
    }

    // undefine macros
    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef READ_STRING
    #undef BINARY_OP
}

InterpretResult interpret_chunk( Chunk* chunk ) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

InterpretResult interpret_source( const char* source ) {
    // create chunk
    Chunk chunk;
    initChunk( &chunk );

    // compile source into chunk
    if( !compile( source, &chunk ) ) {
        freeChunk( &chunk );
        return INTERPRET_COMPILE_ERROR;
    }

    // put chunk into VM
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    // run VM
    InterpretResult result = run();

    // done
    freeChunk( &chunk );
    return result;
}
