#include <stdio.h>
#include <string.h>
#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner( const char* source ) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd() { return '\0' == *scanner.current; }

static Token makeToken( TokenType type ) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken( const char* message ) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen( message );
    token.line = scanner.line;
    return token;
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static bool match( char expected ) {
    if( isAtEnd() || *scanner.current != expected ) return false;
    scanner.current++;
    return true;
}

static char peek() { return *scanner.current; }

static char peekNext() {
  if( isAtEnd() ) return '\0';
  return scanner.current[1];
}

static void skipWhitespace() {
    for(;;) {
        switch( peek() ) {
            // whitespace
            case ' ': case '\r': case '\t':
                advance();
                break;

            // newlines
            case '\n':
                scanner.line++;
                advance();
                break;

            // comments
            case '/':
                if( '/' != peekNext() ) return; // non-whitespace
                while( '\n' != peek() && !isAtEnd() ) advance(); // eat the line
                break;

            // non-whitespace
            default:
                return;
        }
    }
}

static Token string() {
    // search for the closing quote
    while( '"' != peek() ) {
        if( isAtEnd() ) return errorToken( "Unterminated string" );
        if( peek() == '\n' ) scanner.line++;
        advance();
    }

    // eat the closing quote
    advance();

    // return the token w/ the string as the lexeme
    return makeToken( TOKEN_STRING );
}

static bool isDigit( char c ) {
    return c >= '0' && c <= '9';
}

static Token number() {
    // parse the integer part
    while( isDigit( peek() ) ) advance();

    // look for a fractional part
    if( peek() == '.' && isDigit( peekNext() ) ) {
        // consume the dot
        advance();

        // parse the fractional part
        while( isDigit( peek() ) ) advance();
    }
    return makeToken( TOKEN_NUMBER );
}

Token scanToken() {
    // skip whitespace
    skipWhitespace();

    // check for termination
    scanner.start = scanner.current;
    if( isAtEnd() ) return makeToken( TOKEN_EOF );
    
    // advance
    char c = advance();

    // handle numerical literals
    if( isDigit( c ) ) return number();

    // handle symbols and string literals
    switch( c ) {
        case '(': return makeToken( TOKEN_LEFT_PAREN );
        case ')': return makeToken( TOKEN_RIGHT_PAREN );
        case '{': return makeToken( TOKEN_LEFT_BRACE );
        case '}': return makeToken( TOKEN_RIGHT_BRACE );
        case ';': return makeToken( TOKEN_SEMICOLON );
        case ',': return makeToken( TOKEN_COMMA );
        case '.': return makeToken( TOKEN_DOT );
        case '-': return makeToken( TOKEN_MINUS );
        case '+': return makeToken( TOKEN_PLUS );
        case '/': return makeToken( TOKEN_SLASH );
        case '*': return makeToken( TOKEN_STAR );
        case '!': return makeToken( match( '=' ) ? TOKEN_BANG_EQUAL : TOKEN_BANG );
        case '=': return makeToken( match( '=' ) ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL );
        case '<': return makeToken( match( '=' ) ? TOKEN_LESS_EQUAL : TOKEN_LESS );
        case '>': return makeToken( match( '=' ) ? TOKEN_GREATER_EQUAL : TOKEN_GREATER );
        case '"': return string();
    }

    // could not tokenize
    return errorToken( "Unexpected character" );
}
