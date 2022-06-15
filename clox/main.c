#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "debug.h"
#include "scanner.h"

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
    InterpretResult result = interpret_source( source );

    // done
    free( source );
    return INTERPRET_COMPILE_ERROR == result ? 65 : INTERPRET_RUNTIME_ERROR == result ? 70 : 0;
}

int main( int argc, const char* argv[] ) {
    // dispatch on command's 1st character
    switch( argc > 1 ? argv[1][0] : '\0' ) {
        // run
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
            printf( "Welcome to Lox. Type 'exit' to quit.\n" );
            for(;;) {
                printf( "> " );
                if( !fgets( line, sizeof( line ), stdin ) ) { printf( "\n" ); break; }
                if( 0 == strcmp( "exit\n", line ) ) break;
                interpret_source( line );
            }
            freeVM();
            return 0;
        }

        // eval
        case 'e': {
            initVM();
            if( argc < 3 ) break;
            interpret_source( argv[2] );
            freeVM();
            return 0;
        }

        // test
        case 't': {
            // init virtual machine
            initVM();

            // init chunk
            Chunk chunk;
            initChunk( &chunk );
            
            // 1.2 + 3.4
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 1.2 ) ), 123 );
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 3.4 ) ), 123 );
            writeChunk( &chunk, OP_ADD, 123 );
            
            // divide by 5.6
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 5.6 ) ), 123 );
            writeChunk( &chunk, OP_DIVIDE, 123 );

            // negate & print
            writeChunk( &chunk, OP_NEGATE, 123 );
            writeChunk( &chunk, OP_PRINT, 123 );
            
            // write a constant and then pop it
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 6 ) ), 123 );
            writeChunk( &chunk, OP_POP, 123 );

            // write two strings
            // TODO: there's a bug here! we intern the strings OK, but they get added to the constant table in two different slots
            // (note that the same thing happens for numerical values, too)
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );

            // concat them
            writeChunk( &chunk, OP_ADD, 123 );
            writeChunk( &chunk, OP_POP, 123 );
            writeChunk( &chunk, OP_RETURN, 123 );

            // disassemble
            disassembleChunk( &chunk, "disassemble chunk" );

            // interpret
            interpret_chunk( &chunk );
            freeChunk( &chunk );

            // now run something else
            // this should print 'true'
            InterpretResult result = interpret_source( "print !(5 - 4 > 3 * 2 == !nil);" );
            if( INTERPRET_OK != result ) fprintf( stderr, "test failed\n" );

            // test string interning (should have some addresses)
            ObjString* s1 = concatStrings( "hello", 5, " world", 6 );
            printString( s1 );
            printf( "\n" );

            ObjString* s2 = concatStrings( "hello", 5, " world", 6 );
            printString( s2 );
            printf( "\n" );

            ObjString* s3 = makeString( "hi", 2 );
            printString( s3 );
            printf( "\n" );

            // print table load (should be 2)
            printf( "vm.strings.load: %ld\n", vm.strings.load );

            // now delete 's3' and see what happens
            tableDelete( &vm.strings, s3 );

            // print table load (should be 2)
            printf( "after deleting 'hi' - vm.strings.load: %ld\n", vm.strings.load ); 

            // recreate string (new address because we removed it from the hash table)
            ObjString* s4 = makeString( "hi", 2 );
            printString( s4 );
            printf( "\n" );

            // load is still 2
            printf( "vm.strings.load: %ld\n", vm.strings.load );

            // test variable assignment precedence
            InterpretResult result2 = interpret_source( "var x = 1; print x = 3 + 4;" ); // should be fine since 'print' is PRECEDENCE_NONE which is above assignment
            InterpretResult result3 = interpret_source( "var x = 1; print 2 * x = 3 + 4;" ); // should give a compiler error since 2 * x calls variable with PRECEDENCE_UNARY but "=" is PREDENCE_ASSIGNMENT
            if( INTERPRET_OK != result2 || INTERPRET_COMPILE_ERROR != result3 ) fprintf( stderr, "test2 failed\n" );

            // test creation of local variables
            result = interpret_source( "{ var x = 5; var y = 6; }" );
            if( INTERPRET_OK != result ) fprintf( stderr, "local varible test failed\n" );

            // test re-definition of local variable
            result = interpret_source( "{ var x = 5; var x = 6; }" );
            if( INTERPRET_COMPILE_ERROR != result ) fprintf( stderr, "variable redefinition test failed\n" );

            // test accessing local variable
            result = interpret_source( "{ var x = 8; print x; }" );
            if( INTERPRET_OK != result ) fprintf( stderr, "accessing local varible test failed\n" );

            // test accessing variable within its own initializer
            result = interpret_source( "{ var x = x; }" );
            if( INTERPRET_COMPILE_ERROR != result ) fprintf( stderr, "error - variable allowed to access itself within its initializer! (reads stale value from stack, which is leftover from VM's last execution)\n" );

            // test initializing variable with a variable of same name but higher scope
            // note that LOX does not support this (at the moment), but it probably should eventually
            result = interpret_source( "var x = 1; { var x = x; print x; }" );
            if( INTERPRET_COMPILE_ERROR != result ) fprintf( stderr, "error - was able to initialize varible with a same-named higher-scoped variable\n" );

            // test a simple if statement
            result = interpret_source( "if( true ) print \"CORRECT!\"; if( false ) print \"ERROR!\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile a set of if statements\n" );

            // test if-else statements
            result = interpret_source( "if( true ) print \"CORRECT!\"; else print \"ERROR!\"; if( false ) print \"ERROR!\"; else print \"CORRECT!\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile a set of if-else statements\n" );

            // logical and
            result = interpret_source( "if( true and false ) print \"ERROR!\"; else print \"CORRECT\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile logical and\n" );

            // logical or
            result = interpret_source( "if( true or false ) print \"CORRECT!\"; else print \"ERROR\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile logical or\n" );

            // while
            result = interpret_source( "{ var i = 0; while( i < 3 ) { print i; i = i + 1; } }" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile while statement\n" );

            // for loop (no content)
            result = interpret_source( "for( var i = 0;;) { if( i >= 3 ) return; print i; i = i + 1; }" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile for statement\n" );

            // done - release all the objects, which will include both versions of 'hi'
            freeVM();
            return 0;
        }
    }
    
    // unrecognized command
    fprintf( stderr, "Usage: clox [run {file}|shell|eval|test]\n" );
    return 64;
}
