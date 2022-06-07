#include <stdio.h>
#include <string.h>
#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start, *current;
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

static bool isDigit( char c ) { return c >= '0' && c <= '9'; }

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

static bool isAlphaOrUnderscore( char c ) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static TokenType checkKeyword( int start, int length, const char* rest, TokenType type ) {
    if( scanner.current - scanner.start == start + length && 
        0 == memcmp( scanner.start + start, rest, length ) )
        return type;
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch( scanner.start[0] ) {
        case 'a': return checkKeyword( 1, 2, "nd", TOKEN_AND );
        case 'c': return checkKeyword( 1, 4, "lass", TOKEN_CLASS );
        case 'e': return checkKeyword( 1, 3, "lse", TOKEN_ELSE );
        case 'f': {
            if( scanner.current - scanner.start > 1 ) {
                switch( scanner.start[1] ) {
                    case 'a': return checkKeyword( 2, 3, "lse", TOKEN_FALSE );
                    case 'o': return checkKeyword( 2, 1, "r", TOKEN_FOR );
                    case 'u': return checkKeyword( 2, 1, "n", TOKEN_FUN );
                }
            }
            break;
        }
        case 'i': return checkKeyword( 1, 1, "f", TOKEN_IF );
        case 'n': return checkKeyword( 1, 2, "il", TOKEN_NIL );
        case 'o': return checkKeyword( 1, 1, "r", TOKEN_OR );
        case 'p': return checkKeyword( 1, 4, "rint", TOKEN_PRINT );
        case 'r': return checkKeyword( 1, 5, "eturn", TOKEN_RETURN );
        case 's': return checkKeyword( 1, 4, "uper", TOKEN_SUPER );
        case 't': {
            if( scanner.current - scanner.start > 1 ) {
                switch( scanner.start[1] ) {
                    case 'h': return checkKeyword( 2, 2, "is", TOKEN_THIS );
                    case 'r': return checkKeyword( 2, 2, "ue", TOKEN_TRUE );
                }
            }
            break;
        }
        case 'v': return checkKeyword( 1, 2, "ar", TOKEN_VAR );
        case 'w': return checkKeyword( 1, 4, "hile", TOKEN_WHILE );
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while( isAlphaOrUnderscore( peek() ) || isDigit( peek() ) ) advance();
    return makeToken( identifierType() );
}

Token scanToken() {
    // skip whitespace
    skipWhitespace();

    // move the lexeme start to the scanner's current position
    scanner.start = scanner.current;

    // check for termination
    if( isAtEnd() ) return makeToken( TOKEN_EOF );

    // parse token
    char c = advance();
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
        default:
            if( isAlphaOrUnderscore( c ) ) return identifier();
            if( isDigit( c ) ) return number();
            return errorToken( "Unexpected character" );
    }
}
