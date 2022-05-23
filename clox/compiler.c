#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

bool compile( __attribute__((unused)) const char* source, __attribute__((unused)) Chunk* chunk ) {
    // start scanner
    //initScanner( source );

    // "primes the pump" on the scanner???
    //advance();

    // compile the expression
    //expression();

    // the expression should be the only expression in the file
    //consume( TOKEN_EOF, "expect end of expression" );

    // done
    printf( "compiler not yet implemented\n" );
    return false;
}
