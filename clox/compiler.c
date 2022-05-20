#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile( const char* source ) {
    initScanner( source );
    for(;;) {
        Token token = scanToken();
        printf( "%4d %2d '%.*s'\n", token.line, token.type, token.length, token.start ); 
        if( TOKEN_EOF == token.type ) break;
    }
}
