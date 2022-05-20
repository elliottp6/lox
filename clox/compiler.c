#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile( const char* source ) {
    initScanner( source );
    for( int line = -1;; ) {
        Token token = scanToken();
        if( token.line != line ) {
            printf( "%4d ", token.line );
            line = token.line;
        } else {
            printf("   | ");
        }
        printf( "%2d '%.*s'\n", token.type, token.length, token.start ); 
        if( TOKEN_EOF == token.type ) break;
    }
}
