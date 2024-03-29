#include <stdio.h>
#include <stdarg.h>
#include "common.h"
#include "debug.h"
#include "vm.h"
#include "compiler.h"
#include "memory.h"
#include <string.h>
#include <time.h>

VM vm; // global variable!

// a simple clock function
static Value clockNative( int argCount, Value* args ) {
    return NUMBER_VAL( (double)clock() / CLOCKS_PER_SEC );
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
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

            case OBJ_CLASS: {
                // allocate class
                ObjClass* class = AS_CLASS( callee );
                vm.stackTop[-argCount - 1] = OBJ_VAL( newInstance( class ) );

                // call initializer
                Value initializer;
                if( tableGet( &class->methods, vm.initString, &initializer ) ) {
                    return call( AS_CLOSURE( initializer ), argCount );
                } else if( 0 != argCount ) {
                    runtimeError( "Expected 0 arguments but got %d.", argCount );
                    return false;
                }
                return true;
            }

            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD( callee );
                vm.stackTop[-argCount - 1] = bound->receiver; // replace the CallFrame's function (0th slot) w/ the receiver. 'this' points here b/c of the work we did in initCompiler
                return call( bound->method, argCount );
            }

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

static bool invokeFromClass( ObjClass* class, ObjString* name, int argCount ) {
    Value method;
    if( !tableGet( &class->methods, name, &method ) ) {
        runtimeError( "Undefined property '%.*s'", (int)name->len, name->buf );
        return false;
    }
    return call( AS_CLOSURE( method ), argCount );
}

static bool invoke( ObjString* name, int argCount ) {
    // check that receiver is a class instance
    Value receiver = peek( argCount );
    if( !IS_INSTANCE( receiver ) ) {
        runtimeError( "Only instances have methods." );
        return false;
    }

    // get the class instance
    ObjInstance* instance = AS_INSTANCE( receiver );

    // if we're invoking a field on this instance, then call that field
    Value value;
    if( tableGet( &instance->fields, name, &value ) ) {
        vm.stackTop[-argCount - 1] = value; // replace receiver with the value of the field
        return callValue( value, argCount );
    }

    // otherwise, it must be a method, so invoke a method
    return invokeFromClass( instance->class, name, argCount );
}

static bool bindMethod( ObjClass* class, ObjString* name ) {
    // lookup the method
    Value method;
    if( !tableGet( &class->methods, name, &method ) ) {
        runtimeError( "Undefined property '%.*s'", (int)name->len, name->buf );
        return false;
    }

    // bind method to class instance, then put bound method on the stack in place of the class instance
    ObjBoundMethod* bound = newBoundMethod( peek( 0 ), AS_CLOSURE( method ) );
    pop();
    push( OBJ_VAL( bound ) );
    return true;
}

static ObjUpvalue* captureUpvalue( Value* local ) {
    // see if another closure has already captured this upvalue, so it can be shared
    // (note: since list is sorted by upvalue->location, we don't have to keep searching once upvalue->location > local)
    ObjUpvalue *prevUpvalue = NULL, *upvalue = vm.openUpvalues;
    while( NULL != upvalue && upvalue->location > local ) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    // if we found it, we're done
    if( NULL != upvalue && upvalue->location == local ) return upvalue;

    // otherwise: create a new heap-allocated upvalue, & insert it into the list s.t. the list remains sorted
    ObjUpvalue* createdUpvalue = newUpvalue( local );
    createdUpvalue->next = upvalue;
    if( NULL == prevUpvalue ) vm.openUpvalues = createdUpvalue; else prevUpvalue->next = createdUpvalue;
    return createdUpvalue;
}

static void closeUpvalues( Value* last ) {
    while( NULL != vm.openUpvalues && vm.openUpvalues->location >= last ) {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void defineMethod( ObjString* name ) {
    Value method = peek( 0 ); // grab function we compiled from top of stack
    ObjClass* class = AS_CLASS( peek( 1 ) ); // grab class from second-to-top of stack
    tableSet( &class->methods, name, method ); // bind method to class
    pop(); // remove method from stack (leaving class on stack, ready for next method)
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024; // 1 KB (1MB is probably best, but this causes GC's to actually run during simple testing, which is handy)
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;
    initTable( &vm.globals );
    initTable( &vm.strings );
    vm.initString = NULL; // must set this null BEFORE calling makeString, or else a GC could trigger, and try to access vm.initString, which might hold garbage!
    vm.initString = makeString( "init", 4 );
    defineNative( "clock", clockNative );
}

void freeVM() {
    freeTable( &vm.globals );
    freeTable( &vm.strings );
    vm.initString = NULL;
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
            case OP_GET_PROPERTY: {
                // validate that top of stack is an instance
                if( !IS_INSTANCE( peek( 0 ) ) ) {
                    runtimeError( "Only instances have properties." );
                    return INTERPRET_RUNTIME_ERROR;
                }

                // get instance from top of stack
                ObjInstance* instance = AS_INSTANCE( peek( 0 ) );

                // the next bytecode contains a string constant for the field
                ObjString* name = READ_STRING();

                // if we have a field: replace 'instance' on stack with 'value' from the field
                // (note that fields shadow methods, which is why we check for a field first)
                Value value;
                if( tableGet( &instance->fields, name, &value ) ) {
                    pop(); // Instance.
                    push( value );
                    break;
                }

                // if we have a method: bind it to this class before putting it on the stack
                if( !bindMethod( instance->class, name ) ) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                // ensure we have an instance second-to-top of stack
                if( !IS_INSTANCE( peek( 1 ) ) ) {
                    runtimeError( "Only instances have fields." );
                    return INTERPRET_RUNTIME_ERROR;
                }

                // instance is second-to-top of stack
                ObjInstance* instance = AS_INSTANCE( peek(1) );

                // value to set field to is on top of stack
                // note that we have to use 'peek' b/c 'pop' might make the value temporarily invisible to the GC (and tableSet can potentially trigger a GC)
                tableSet( &instance->fields, READ_STRING(), peek(0) );

                // it is now safe to pop the value (since we've already stashed it into a table)
                Value value = pop();

                // now pop the instance and replace it with the value (since 'set property' is an expression that returns the value it was set to)
                pop();
                push( value );
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push( BOOL_VAL( valuesEqual( a, b ) ) );
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek( 0 );
                break;
            }
            case OP_GREATER:    BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:       BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if( IS_STRING( peek(0) ) && IS_STRING( peek(1) ) ) {
                    // EP: isn't this a potential GC problem since the strings won't exist on the stack (so a concurrent GC could collect them after pop, but before concat?)
                    // EP on GC chaper: yes, it is!
                    ObjString* b = AS_STRING( peek( 0 ) );
                    ObjString* a = AS_STRING( peek( 1 ) );
                    ObjString* c = concatStrings( a->buf, a->len, b->buf, b->len );
                    pop(); // pop a
                    pop(); // pop b
                    push( OBJ_VAL( c ) );
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
            case OP_INVOKE: {
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();
                if( !invoke( method, argCount ) ) return INTERPRET_RUNTIME_ERROR;

                // after invoke, there's a new call frame on the stack, so refresh our local copy
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }

            case OP_SUPER_INVOKE: {
                // pull method & argCount from instructions
                ObjString* method = READ_STRING();
                int argCount = READ_BYTE();

                // pop superclass from stack
                ObjClass* superclass = AS_CLASS( pop() );

                // directly invoke the method
                if( !invokeFromClass( superclass, method, argCount ) ) return INTERPRET_RUNTIME_ERROR;

                // after invoke, there's a new call frame on the stack, so refresh our local copy
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            
            case OP_CLOSURE: {
                // push the closure to the stack
                ObjFunction* function = AS_FUNCTION( READ_CONSTANT() );
                ObjClosure* closure = newClosure( function );
                push( OBJ_VAL( closure ) );

                // read the upvalues
                for( int i = 0; i < closure->upvalueCount; i++ ) {
                    uint8_t isLocal = READ_BYTE(), index = READ_BYTE();
                    closure->upvalues[i] = isLocal ? captureUpvalue( frame->slots + index ) :
                                                     frame->closure->upvalues[index];
                }
                break;
            }

            case OP_CLOSE_UPVALUE: {
                closeUpvalues( vm.stackTop - 1 );
                pop();
                break;
            }

            case OP_RETURN: {
                // pop result & the frame
                Value result = pop();
                closeUpvalues( frame->slots );
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
            
            // creates a new class
            case OP_CLASS: {
                push( OBJ_VAL( newClass( READ_STRING() ) ) );
                break;
            }

            // creates a new method
            case OP_METHOD: {
                defineMethod( READ_STRING() );
                break;
            }

            case OP_INHERIT: {
                // get superclass
                Value superclass = peek( 1 );
                if( !IS_CLASS( superclass ) ) {
                    runtimeError( "Superclass must be a class." );
                    return INTERPRET_RUNTIME_ERROR;
                }

                // get subclass
                ObjClass* subclass = AS_CLASS( peek( 0 ) );

                // copy all of the superclass methods into the subclass
                // "copy-down inheritance", which makes method invocation *fast*
                // this only works b/c user cannot add methods to the superclass at runtime
                // also note: this runs BEFORE methods are defined on the class, so it can override any of the superclass methods
                tableAddAll( &AS_CLASS( superclass )->methods, &subclass->methods );

                // pop the subclass (leaving the superclass)
                pop();
                break;
            }

            case OP_GET_SUPER: {
                // get method name from constant table
                ObjString* name = READ_STRING();

                // pop superclass from stack
                ObjClass* superclass = AS_CLASS( pop() );

                // create a new bound method, connecting super's method to the class
                // (this pops the class off the stack, and replaces it with the bound method)
                if( !bindMethod( superclass, name ) ) {
                    return INTERPRET_RUNTIME_ERROR;
                }
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

static Value interpret_main( ObjFunction* main, Value keepAlive ) {
    // validate args
    if( NULL == main ) return ERROR_VAL( COMPILE_ERROR );

    // clear stack
    resetStack();
    push( keepAlive );

    // wrap main in a closure (this allocates and triggers a GC)
    push( OBJ_VAL( main ) );
    ObjClosure* closure = newClosure( main );
    pop();
    push( OBJ_VAL( closure ) );

    // replace main function with clousre
    call( closure, 0 );

    // run VM
    InterpretResult result = run();

    // check for errors
    if( INTERPRET_COMPILE_ERROR == result ) return ERROR_VAL( COMPILE_ERROR );
    if( INTERPRET_RUNTIME_ERROR == result ) return ERROR_VAL( RUNTIME_ERROR );
    
    // return what's on the top of the stack (but don't pop it, or else it could get GC'd)
    return peek( 0 );
}

Value interpret( const char* source, Value keepAlive ) {
    resetStack();
    push( keepAlive );
    ObjFunction* main = compile( source );
    return interpret_main( main, keepAlive );
}

// WARNING: this takes OWNERSHIP over chunk, so caller should not free the chunk!
Value interpret_chunk( Chunk chunk ) {
    // push constants onto stack so GC won't collect them when we call ''makeString' or 'newFunction'
    resetStack();
    ValueArray constants = chunk.constants;
    for( size_t i = 0; i < constants.count; i++ ) push( constants.values[i] );

    // similarly, push the name of the function onto the stack
    push( OBJ_VAL( makeString( "main", 4 ) ) );

    // create main function
    ObjFunction *main = newFunction();
    main->name = (ObjString*)AS_OBJ( pop() );
    main->chunk = chunk;

    // now we can pop the constants off safely
    resetStack();
    return interpret_main( main, NIL_VAL );
}
