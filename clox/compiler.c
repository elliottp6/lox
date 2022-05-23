#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

// expression precedence
typedef enum {
  PRECEDENCE_NONE,
  PRECEDENCE_ASSIGNMENT,  // =
  PRECEDENCE_OR,          // or
  PRECEDENCE_AND,         // and
  PRECEDENCE_EQUALITY,    // == !=
  PRECEDENCE_COMPARISON,  // < > <= >=
  PRECEDENCE_TERM,        // + -
  PRECEDENCE_FACTOR,      // * /
  PRECEDENCE_UNARY,       // ! -
  PRECEDENCE_CALL,        // . ()
  PRECEDENCE_PRIMARY
} Precedence;

// global parser object
Parser parser;

// global chunk that is being compiled
Chunk* compilingChunk;

// returns current chunk being compiled
static Chunk* currentChunk() { return compilingChunk; }

// reports parsing errors to stderr
static void errorAt( Token* token, const char* message ) {
    // if we're already in panic mode, no need to report further errors
    if( parser.panicMode ) return;

    // put parser into panic mode
    parser.panicMode = true;
    parser.hadError = true;

    // print line number
    fprintf( stderr, "[line %d] Error", token->line );

    // print token info
    switch( token->type ) {
        case TOKEN_EOF: fprintf( stderr, " at end" ); break;
        case TOKEN_ERROR: break;
        default: fprintf( stderr, " at '%.*s'", token->length, token->start ); break;
    }

    // print error message
    fprintf( stderr, ": %s\n", message );
}

// handy error reporting wrappers
__attribute__((unused)) static void error( const char* message ) { errorAt( &parser.previous, message ); }
static void errorAtCurrent( const char* message ) { errorAt( &parser.current, message ); }

// advances the parser forward to the next non-error token
static void advance() {
    // set the previous token
    parser.previous = parser.current;

    // scan tokens, reporting errors, until we hit a non-error token
    while( TOKEN_ERROR == (parser.current = scanToken()).type )
        errorAtCurrent( parser.current.start );
}

static void consume( TokenType type, const char* message ) {
    if( parser.current.type != type ) { errorAtCurrent( message ); return; }
    advance();
}

// emit byte(s) to the current chunk
static void emitByte( uint8_t byte ) { writeChunk( currentChunk(), byte, parser.previous.line ); }
__attribute__((unused)) static void emitBytes( uint8_t byte1, uint8_t byte2 ) { emitByte( byte1 ); emitByte( byte2 ); }

// emit return opcode
static void emitReturn() { emitByte( OP_RETURN ); }

// ends a chunk
static void endCompiler() { emitReturn(); }

// adds a constant into the static data section of the chunk, and returns its handle
static uint8_t makeConstant( Value value ) {
    int constant = addConstant( currentChunk(), value );
    if( constant > UINT8_MAX ) { error( "Too many constants in one chunk." ); return 0; }
    return (uint8_t)constant;
}

static void emitConstant( Value value ) { emitBytes( OP_CONSTANT, makeConstant( value ) ); }

// -- EXPRESSION PARSING --
static void expression();
static void parsePrecedence( Precedence precedence );

// parses number
static void number() {
    double value = strtod( parser.previous.start, NULL );
    emitConstant( value );
}

// parses grouping (prefix expression)
static void grouping() {
    expression();
    consume( TOKEN_RIGHT_PAREN, "Expect ')' after expression." );
}

// prases unary (prefix expression)
static void unary() {
    // get the operator
    TokenType operatorType = parser.previous.type;

    // compile the operand
    parsePrecedence( PRECEDENCE_UNARY );

    // emit the operator instruction
    switch( operatorType ) {
        case TOKEN_MINUS: emitByte( OP_NEGATE ); return; // negation
        default: return; // unreachable
    }
}

static void parsePrecedence( Precedence precedence ) {
    // What goes here?
}

/*
static ParseRule* getRule( TokenType type ) {
    // TODO: implement
    return NULL;
}*/

static void binary() {
    /*
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule( operatorType );
    parsePrecedence( (Precedence)(rule->precedence + 1) );

    switch( operatorType ) {
        case TOKEN_PLUS: emitByte( OP_ADD ); break;
        case TOKEN_MINUS: emitByte (OP_SUBTRACT ); break;
        case TOKEN_STAR: emitByte( OP_MULTIPLY ); break;
        case TOKEN_SLASH: emitByte( OP_DIVIDE ); break;
        default: return; // unreachable
    }*/
}

// compiles an expression
static void expression() {
    parsePrecedence( PRECEDENCE_ASSIGNMENT );
}

bool compile( const char* source, Chunk* chunk ) {
    // start scanner
    initScanner( source );

    // set the chunk that we're compiling into
    compilingChunk = chunk;

    // initialize parser
    parser.hadError = false;
    parser.panicMode = false;
    advance();

    // compile the expression
    expression();

    // done
    consume( TOKEN_EOF, "expect end of expression" );
    endCompiler();
    return !parser.hadError;
}
