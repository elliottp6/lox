#include <stdio.h>
#include <stdarg.h>
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"
#include "memory.h"
#include <string.h>
//#include <time.h>

VM vm; // global variable!

// a simple native function
static Value clockNative( int argCount, Value* args ) {
    return NUMBER_VAL( 1618 ); // NUMBER_VAL( (double)clock() / CLOCKS_PER_SEC );
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
}

static void runtimeError( const char* format, ... ) {
    // print error message
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    fputs( "\n", stderr );

    // print stack trace
    for( int i = vm.frameCount - 1; i >= 0; i-- ) {
        // print line # for stack frame
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf( stderr, "[line %d] in ", function->chunk.lines[instruction] );

        // print function name
        if ( NULL == function->name ) {
            fprintf( stderr, "script\n" );
        } else {
            printStringToErr( function->name );
            fprintf( stderr, "\n" );
        }
    }

    // reset the stack
    resetStack();
}

// defines a native function
static void defineNative( const char* name, NativeFn function ) {
    // push a string object w/ name of function
    push( OBJ_VAL( makeString( name, (int)strlen( name ) ) ) );

    // push the native function
    push( OBJ_VAL( newNative( function ) ) );

    // set this string in the globals table to point to this native function
    tableSet( &vm.globals, AS_STRING( vm.stack[0] ), vm.stack[1] );

    // pop the string & function from the stack
    // note that the ONLY reason we pushed them onto the stack to begin with was so the GC
    //  would be able to find them if it kicked off before we put them into the table
    pop();
    pop();
}

void push( Value value ) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek( int distance ) { return vm.stackTop[-1 - distance]; }

static bool call( ObjClosure* closure, int argCount ) {
    // sanity check argCount
    if( argCount != closure->function->arity ) {
        runtimeError( "Expected %d arguments but got %d.", closure->function->arity, argCount );
        return false;
    }

    // check for stack overflow
    if( FRAMES_MAX == vm.frameCount ) { runtimeError( "Stack overflow." ); return false; }

    // push a new callFrame
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue( Value callee, int argCount ) {
    if( IS_OBJ( callee ) ) {
        switch( OBJ_TYPE( callee ) ) {
            case OBJ_FUNCTION: {
               runtimeError( "Encountered a raw function (should be wrapped in a closure)." );
                //return call( AS_FUNCTION( callee ), argCount );
                return false;
            }

            case OBJ_CLOSURE:
                return call( AS_CLOSURE( callee ), argCount );

            case OBJ_NATIVE: {
                // call native function
                NativeFn native = AS_NATIVE( callee );
                Value result = native( argCount, vm.stackTop - argCount );

                // unwind args
                vm.stackTop -= argCount + 1;

                // return
                push( result );
                return true;
            }
            default: break; // non-callable object type
        }
    }
    runtimeError( "Can only call functions and classes." );
    return false;
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable( &vm.globals );
    initTable( &vm.strings );
    defineNative( "clock", clockNative );
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
    printf( "=> execution trace\n" );
    #endif

    // get the current call frame
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

    // macros
    #define READ_BYTE() (*frame->ip++)
    #define READ_USHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
    #define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
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
            disassembleInstruction( &frame->closure->function->chunk, (size_t)(frame->ip - frame->closure->function->chunk.code) );

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
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE(); // get the local's slot
                push( frame->slots[slot] ); // read the local, and push it onto the stack (for other instructions to use)
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE(); // get the local's slot
                frame->slots[slot] = peek( 0 ); // set the slot to the value that's on the top of the stack (don't pop it, because it's an expression, and so it should return a value which is itself)
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet( &vm.globals, name, peek( 0 ) );
                pop(); // pop AFTER adding it, just in case a GC is triggered (we want to ensure that string still exists on the stack!)
                break;
            }
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
            case OP_SET_GLOBAL: { // sets the global, but leaves the value on the stack (since setting a value is an expression)
                ObjString* name = READ_STRING();
                if( tableSet( &vm.globals, name, peek(0) ) ) { // set value, but if it's a NEW value then...
                    tableDelete( &vm.globals, name ); // mistake! must use 'DEFINE_GLOBAL' for that!
                    runtimeError( "Undefined variable '%.*s'.", (int)name->len, name->buf );
                    return INTERPRET_RUNTIME_ERROR;
                }
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
            case OP_JUMP: {
                uint16_t offset = READ_USHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_USHORT();
                //if( isFalsey( peek( 0 ) ) ) vm.ip += offset;
                frame->ip += offset * isFalsey( peek( 0 ) ); // no branching version of above
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_USHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                if( !callValue( peek( argCount ), argCount ) ) return INTERPRET_RUNTIME_ERROR;
                frame = &vm.frames[vm.frameCount - 1]; // callValue changed the VM frame, so update our local variable
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION( READ_CONSTANT() );
                ObjClosure* closure = newClosure( function );
                push( OBJ_VAL( closure ) );
                break;
            }
            case OP_RETURN: {
                // pop result & the frame
                Value result = pop();
                vm.frameCount--;

                // if it is the root frame: exit the program
                if( 0 == vm.frameCount ) {
                    pop(); // pop main function
                    push( result ); // push result of calling main
                    return INTERPRET_OK; // signal no errors
                }

                // otherwise: set stack & frame to that of caller
                vm.stackTop = frame->slots;
                push( result );
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            // not in book: error on unrecognized opcodes
            default:
                runtimeError( "unrecognized opcode: %d", instruction );
                return INTERPRET_RUNTIME_ERROR; // 
        }
    }

    // undefine macros
    #undef READ_BYTE
    #undef READ_USHORT
    #undef READ_CONSTANT
    #undef READ_STRING
    #undef BINARY_OP
}

static Value interpret_main( ObjFunction* main ) {
    // validate args
    if( NULL == main ) return ERROR_VAL( COMPILE_ERROR );

    // clear stack
    resetStack();

    // wrap main in a closure
    ObjClosure* closure = newClosure( main );
    push( OBJ_VAL( closure ) );
    call( closure, 0 );

    // run VM
    InterpretResult result = run();

    // check for errors
    if( INTERPRET_COMPILE_ERROR == result ) return ERROR_VAL( COMPILE_ERROR );
    if( INTERPRET_RUNTIME_ERROR == result ) return ERROR_VAL( RUNTIME_ERROR );
    
    // otherwise, return what's on the stack
    return pop();
}

Value interpret( const char* source ) {
    return interpret_main( compile( source ) );
}

Value interpret_chunk( Chunk* chunk ) {
    ObjFunction main;
    main.arity = 0;
    main.chunk = *chunk;
    main.name = makeString( "main", 4 );
    main.obj.next = NULL;
    main.obj.type = OBJ_FUNCTION;
    return interpret_main( &main );
}
