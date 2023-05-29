#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "debug.h"
#include "scanner.h"
#include "compiler.h"

static char* readFile( const char* path ) {
    // open file
    FILE* file = fopen( path, "rb" );
    if( NULL == file ) {
        fprintf( stderr, "Could not open file \"%s\".\n", path );
        return NULL;
    }

    // determine file size TODO: handle failure cases
    fseek( file, 0L, SEEK_END );
    size_t fileSize = ftell( file );
    rewind( file );

    // allocate buffer to hold file
    char* buffer = (char*)malloc( fileSize + 1 );
    if( NULL == buffer ) {
        fclose( file );
        fprintf( stderr, "Not enough memory to read \"%s\".\n", path );
        return NULL;
    }

    // read entire file into buffer
    size_t bytesRead = fread( buffer, sizeof( char ), fileSize, file );
    if( bytesRead < fileSize ) {
        free( buffer );
        fclose( file );
        fprintf( stderr, "Could not read file \"%s\".\n", path );
        return NULL;
    }
    
    // mark buffer with null terminator
    buffer[bytesRead] = '\0';

    // close file
    fclose( file );
    return buffer;
}

static int runFile( const char* path ) {
    // read source file
    char* source = readFile( path );
    if( NULL == source ) return 74;

    // interpret source file
    Value value = interpret( source, NIL_VAL );

    // done
    free( source );
    return IS_ERROR( value ) ? 65 : 0; // return INTERPRET_COMPILE_ERROR == result ? 65 : INTERPRET_RUNTIME_ERROR == result ? 70 : 0;
}

bool interpret_test( char* title, char* source, Value expected ) {
    // run test
    printf( "\n=> %s\n", title );
    Value value = interpret( source, expected );
    bool result = valuesEqual( value, expected );

    // print result
    if( result ) {
        printf( "SUCCESS\n" );
    } else {
        printf( "ERROR: expected " );
        printValue( expected );
        printf( " but got " );
        printValue( value );
        printf( "\n" );
    }
    return result;
}

int main( int argc, const char* argv[] ) {
    // dispatch on command's 1st character
    switch( argc > 1 ? argv[1][0] : '\0' ) {
        // run file
        case 'r': {
            if( argc < 3 ) break;
            initVM();
            int result = runFile( argv[2] );
            freeVM();
            return result;
        }

        // shell
        case 's': {
            initVM();
            char line[1024];
            printf( "Welcome to Lox. Type 'q' to quit.\n" );
            for(;;) {
                printf( "> " );
                if( !fgets( line, sizeof( line ), stdin ) ) { printf( "\n" ); break; }
                if( 0 == strcmp( "q\n", line ) ) break;
                Value value = interpret( line, NIL_VAL );
                printf( "=> result: " );
                printValue( value );
                printf( "\n" );
            }
            freeVM();
            return 0;
        }

        // eval
        case 'e': {
            initVM();
            if( argc < 3 ) break;
            interpret( argv[2], NIL_VAL );
            freeVM();
            return 0;
        }

        // test
        case 't': {
            // TEST
            {
                printf( "\n=> TEST -((1.2 + 3.4) / 2)\n" );

                // init VM
                initVM();
                Chunk chunk;
                initChunk( &chunk );

                // write chunk
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 1.2 ) ), 123 );
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 3.4 ) ), 123 );
                writeChunk( &chunk, OP_ADD, 123 );
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 2 ) ), 123 );
                writeChunk( &chunk, OP_DIVIDE, 123 );
                writeChunk( &chunk, OP_NEGATE, 123 );
                writeChunk( &chunk, OP_RETURN, 123 );

                // interpret
                printf( "=> bytecode\n" );
                disassembleChunk( &chunk );

                printf( "=> interpret_chunk\n" );
                Value value = interpret_chunk( chunk ); // <-- note that VM takes ownership of chunk at this point

                // validate
                if( IS_NUMBER( value ) && valuesEqual( value, NUMBER_VAL( -2.3 ) ) ) {
                    printf( "SUCCESS\n" );
                } else {
                    printf( "ERROR: Expected -2.3, but got: " );
                    printValue( value );
                    printf( "\n" );
                    freeVM();
                    return 1;
                }

                // free VM
                freeVM();
            }

            // TEST
            {
                printf( "\n=> TEST intern & concat 2 identical strings\n" );

                // init VM
                initVM();
                Chunk chunk;
                initChunk( &chunk );

                // write chuink
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );
                writeChunk( &chunk, OP_ADD, 123 );
                writeChunk( &chunk, OP_RETURN, 123 );
                printf( "=> bytecode\n" );
                disassembleChunk( &chunk );

                printf( "=> interpret_chunk\n" );
                Value value = interpret_chunk( chunk ); // <-- note that VM takes ownership of chunk here

                // validate
                if( IS_STRING( value ) && valuesEqual( value, OBJ_VAL( makeString( "hihi", 4 ) ) ) ) {
                    printf( "SUCCESS (note: string interned OK, but constant is still duped!)\n" );
                } else {
                    printf( "ERROR: Expected 'hihi', but got: " );
                    printValue( value );
                    printf( "\n" );
                    freeVM();
                    return 1;
                }

                // free VM
                freeVM();
            }

            // TEST
            {
                printf( "\n=> TEST STRING INTERNING\n" );
                initVM();

                // create string objects "hello world" and "hi"
                size_t init_load = vm.strings.load;
                concatStrings( "hello", 5, " world", 6 );
                concatStrings( "hello", 5, " world", 6 );
                makeString( "hi", 2 );

                // test # of strings we actually created in the VM - it should just be two
                if( 2 == vm.strings.load - init_load ) {
                    printf( "SUCCESS\n" );
                } else {
                    printf( "ERROR: Expected 2 strings, but got: %zu strings\n", vm.strings.load );
                    freeVM();
                    return 1;
                }
                freeVM();
            }

            // build VM for interpret tests
            initVM();

            // test simple expression
            if( !interpret_test(
                "TEST SIMPLE EXPRESSION",
                "return !(5 - 4 > 3 * 2 == !nil);",
                BOOL_VAL( true ) ) ) { freeVM(); return 1; }

            // test variable assignment precedence
            if( !interpret_test(
                "TEST ASSIGNMENT PRECEDENCE",
                "var x = 1; return x = 3 + 4;", // should be fine since 'return' is PRECEDENCE_NONE which is above assignment
                NUMBER_VAL( 7 ) ) ) { freeVM(); return 1; }

            // test incorrect variable assignment precedence
            if( !interpret_test(
                "TEST INCORRECT ASSIGNMENT PRECEDENCE",
                "var x = 1; return 2 * x = 3 + 4;",
                ERROR_VAL( COMPILE_ERROR ) ) ) { freeVM(); return 1; }

            // test local variable
            if( !interpret_test(
                "TEST LOCAL VARIABLE",
                "{ var x = 5; return x; }",
                NUMBER_VAL( 5 ) ) ) { freeVM(); return 1; }

            // test redefining local variable
            if( !interpret_test(
                "TEST REDEFINING LOCAL VARIABLE",
                "{ var x = 5; var x = 6; }",
                ERROR_VAL( COMPILE_ERROR ) ) ) { freeVM(); return 1; }

            // test accessing variable within initializer
            // note that this actually should work by referring to the OUTER scoped x, but lox does not support this
            if( !interpret_test(
                "TEST ACCESSING VARIABLE IN INITIALIZER",
                "var x = 1; { var x = x; }",
                ERROR_VAL( COMPILE_ERROR ) ) ) { freeVM(); return 1; }

            // test if statment
            if( !interpret_test(
                "TEST IF STATEMENT",
                "if( true ) return 5; if( false ) return 0;",
                NUMBER_VAL( 5 ) ) ) { freeVM(); return 1; }

            // test if-else statment
            if( !interpret_test(
                "TEST IF-ELSE STATEMENT",
                "if( false ) return 5; else return 0;",
                NUMBER_VAL( 0 ) ) ) { freeVM(); return 1; }
            
            // test logical and
            if( !interpret_test(
                "TEST LOGICAL AND",
                "if( true and false ) return 1; else return \"OK\";",
                OBJ_VAL( makeString( "OK", 2 ) ) ) ) { freeVM(); return 1; }

            // test logical or
            if( !interpret_test(
                "TEST LOGICAL OR",
                "return true or false;",
                BOOL_VAL( true ) ) ) { freeVM(); return 1; }

            // test while loop
            if( !interpret_test(
                "WHILE LOOP",
                "{ var i = 0; while( i < 3 ) { i = i + 1; } return i; }",
                NUMBER_VAL( 3 ) ) ) { freeVM(); return 1; }

            // test for loop
            if( !interpret_test(
                "FOR LOOP",
                "{ var k = 0; for( var i = 0; i < 4; i = i + 1 ) { k = k + i; } return k; }",
                NUMBER_VAL( 6 ) ) ) { freeVM(); return 1; }

            // test function
            if( !interpret_test(
                "FUNCTION",
                "fun return_hi() { return \"hi\"; } return return_hi();",
                OBJ_VAL( makeString( "hi", 2 ) ) ) ) { freeVM(); return 1; }

            // test function with parameters (note that we can use 'return_hi' from before)
            if( !interpret_test(
                "FUNCTION WITH PARAMETERS",
                "fun double( str ) { return str + str; } var doubled = double( return_hi() ); return doubled;",
                OBJ_VAL( makeString( "hihi", 4 ) ) ) ) { freeVM(); return 1; }

            // test calling a native 'clock' function
            if( !interpret_test(
                "CALL NATIVE FUNCTION",
                "var c = clock();\n"
                "return c - c;\n",
                NUMBER_VAL( 0 ) ) ) { freeVM(); return 1; }

            // test calling function with too many parameters
            if( !interpret_test(
                "FUNCTION CALL WITH TOO MANY PARAMETERS",
                "return double( 1, 2 );",
                ERROR_VAL( RUNTIME_ERROR ) ) ) { freeVM(); return 1; }

            // test broken program for testing stack-trace printing
            // (you need to visually ensure the stack trace is correct)
            if( !interpret_test(
                "RUNTIME ERROR SHOULD PRINT STACK TRACE",
                "fun a() { b(); }\n"
                "fun b() { c(); }\n"
                "fun c() {\n"
                "   c(\"too\", \"many\");\n"
                "}\n"
                "\n"
                "a();\n",
                ERROR_VAL( RUNTIME_ERROR ) ) ) { freeVM(); return 1; }

            // test closure disassembly (manual only)
            {
                ObjFunction* main = compile(
                    "fun outer() {\n"
                    "   var a = 1;\n"
                    "   var b = 2;\n"
                    "   fun middle() {\n"
                    "       var c = 3;\n"
                    "       var d = 4;\n"
                    "       fun inner() {\n"
                    "           print a + c + b + d;\n"
                    "       }\n"
                    "   }\n"
                    "}\n" );
                disassembleChunk( &main->chunk );
            }

            // test capturing upvalues to heap as values
            if( !interpret_test(
                "CLOSURE TEST (UPVALUES CAPTURED TO HEAP AS VALUES)",
                "fun outer() {\n"
                "   var x = \"outside\";\n"
                "   fun inner() { return x; }\n"
                "   return inner();\n"
                "}\n"
                "return outer();\n",
                OBJ_VAL( makeString( "outside", 7 ) ) ) ) { freeVM(); return 1; }

            // test capturing upvalues to heap as variables
            if( !interpret_test(
                "CLOSURE TEST (UPVALUES CAPTURED TO HEAP AS VARIABLES)",
                "var globalSet;\n"
                "var globalGet;\n"
                "fun myFunction() {\n"
                "   var a = \"initial\";\n"
                "   fun set() { a = \"updated\"; }\n"
                "   fun get() { return a; }\n"
                "   globalSet = set;\n"
                "   globalGet = get;\n"
                "}\n"
                "myFunction();\n"
                "globalSet();\n"
                "return globalGet();\n",
                OBJ_VAL( makeString( "updated", 7 ) ) ) ) { freeVM(); return 1; }

            // test simple class instance with fields
            if( !interpret_test(
                "SIMPLE CLASS INSTANCE WITH FIELDS",
                "class Pair {}\n"
                "var pair = Pair();\n"
                "pair.first = 1;\n"
                "pair.second = 2;\n"
                "return pair.first + pair.second;",
                NUMBER_VAL( 3 ) ) ) { freeVM(); return 1; }

            // test class instance with static method
            if( !interpret_test(
                "STATIC METHOD",
                "class Test {\n"
                "   returnNumber() { return 6; }\n"
                "}\n"
                "var t = Test();\n"
                "return t.returnNumber();\n",
                NUMBER_VAL( 6 ) ) ) { freeVM(); return 1; }

            // test class instance with dynamic method
            if( !interpret_test(
                "DYNAMIC METHOD",
                "class Test {\n"
                "   returnNumber() { return this.number; }\n"
                "}\n"
                "var t = Test();\n"
                "t.number = 101;\n"
                "return t.returnNumber();\n",
                NUMBER_VAL( 101 ) ) ) { freeVM(); return 1; }

            // test that we cannot reference 'this' outside of a class
            if( !interpret_test(
                "INVALID 'THIS' REFERENCE",
                "return this;",
                ERROR_VAL( COMPILE_ERROR ) ) ) { freeVM(); return 1; }           

            // test a simple constructor
            if( !interpret_test( 
                "SIMPLE CONSTRUCTOR",
                "class CoffeeMaker {\n"
                "   init(coffee) {\n"
                "       this.coffee = coffee;\n"
                "   }\n"
                "   brew() { return this.coffee; }\n"
                "}\n"
                "var maker = CoffeeMaker( 899 );\n"
                "return maker.brew();",
                NUMBER_VAL( 899 ) ) ) { freeVM(); return 1; }

            // test that we cannot return from an initializer
            if( !interpret_test( 
                "SIMPLE CONSTRUCTOR",
                "class CoffeeMaker {\n"
                "   init(coffee) {\n"
                "       this.coffee = coffee;\n"
                "       return 56;\n"
                "   }\n"
                "}\n",
                ERROR_VAL( COMPILE_ERROR ) ) ) { freeVM(); return 1; }

            // test that we can invoke a field
            if( !interpret_test(
                "INVOKING A FIELD",
                "fun myFunction() { return 50; }\n"
                "class Test {}\n"
                "var t = Test();\n"
                "t.field = myFunction;\n"
                "return t.field();\n",
                NUMBER_VAL( 50 ) ) ) { freeVM(); return 1; }

            // test inheriting from an invalid superclass
            if( !interpret_test(
                "INVALID SUPERCLASS",
                "var notClass = 5;\n"
                "class Oops < notClass {}\n",
                ERROR_VAL( RUNTIME_ERROR ) ) ) { freeVM(); return 1; }

            // test inheritance & super
            if( !interpret_test(
                "INHERITANCE & SUPER",
                "class First {\n"
                "   num1() { return 1; }\n"
                "   num2() { return 2; }\n"
                "}\n"
                "class Second < First {\n"
                "   num1() { return super.num1() + this.num2(); }"
                "}\n"
                "var sec = Second();\n"
                "return sec.num1();\n",
                NUMBER_VAL( 3 ) ) ) { freeVM(); return 1; }

            // done
            freeVM();
            return 0;
        }
    }
    
    // unrecognized command
    fprintf( stderr, "Usage: clox [run {file}|shell|eval|test]\n" );
    return 64;
}
