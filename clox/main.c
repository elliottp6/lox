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
            for(;;) {
                printf( "> " );
                if( !fgets( line, sizeof( line ), stdin ) ) { printf( "\n" ); break; }
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

            // negate
            writeChunk( &chunk, OP_NEGATE, 123 );
            writeChunk( &chunk, OP_RETURN, 123 );

            // disassemble
            disassembleChunk( &chunk, "disassemble chunk" );

            // interpret
            interpret_chunk( &chunk );

            // done
            freeChunk( &chunk );
            freeVM();
            return 0;
        }
    }
    
    // unrecognized command
    fprintf( stderr, "Usage: clox [run {file}|shell|eval|test]\n" );
    return 64;
}
