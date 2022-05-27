// good read on Pratt parsers: http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

// top-level parser object
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

// function pointer for parsing expressions
typedef void (*ParseFn)();

// Pratt parser parsing rule
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

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
static void error( const char* message ) { errorAt( &parser.previous, message ); }
static void errorAtCurrent( const char* message ) { errorAt( &parser.current, message ); }

// advances the parser forward to the next non-error token
static void advance() {
    // set the previous token
    parser.previous = parser.current;

    // scan tokens, reporting errors, until we hit a non-error token
    for(;;) {
        Token token = parser.current = scanToken();
        #ifdef DEBUG_PRINT_SCAN
        if( TOKEN_EOF != token.type )
            printf( "%4d %2d '%.*s'\n", token.line, token.type, token.length, token.start ); 
        #endif
        if( TOKEN_ERROR != token.type ) break;
        errorAtCurrent( parser.current.start );
    }
}

static bool check( TokenType type ) { return parser.current.type == type; }

static bool match( TokenType type ) {
    if( !check( type ) ) return false;
    advance();
    return true;
}

static void consume( TokenType type, const char* message ) {
    if( !match( type ) ) errorAtCurrent( message );
}

// emit byte(s) to the current chunk
static void emitByte( uint8_t byte ) { writeChunk( currentChunk(), byte, parser.previous.line ); }
static void emitBytes( uint8_t byte1, uint8_t byte2 ) { emitByte( byte1 ); emitByte( byte2 ); }

// emit return opcode
static void emitReturn() { emitByte( OP_RETURN ); }

// ends a chunk
static void endCompiler() {
    // return from our 'main' function
    emitReturn();

    // disassemble code before running it
    #ifdef DEBUG_PRINT_CODE
    if( !parser.hadError ) disassembleChunk( currentChunk(), "compiled bytecode" );
    #endif
}

// adds a constant into the static data section of the chunk, and returns its handle
static uint8_t makeConstant( Value value ) {
    int constant = addConstant( currentChunk(), value );
    if( constant > UINT8_MAX ) { error( "Too many constants in one chunk." ); return 0; }
    return (uint8_t)constant;
}

static void emitConstant( Value value ) { emitBytes( OP_CONSTANT, makeConstant( value ) ); }

// -- EXPRESSION PARSING --
static void expression();
static ParseRule* getRule( TokenType type );
static void parsePrecedence( Precedence precedence );
static void statement();
static void declaration();

// parses number
static void number() {
    double value = strtod( parser.previous.start, NULL );
    emitConstant( NUMBER_VAL( value ) );
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
        case TOKEN_BANG: emitByte( OP_NOT ); return;
        case TOKEN_MINUS: emitByte( OP_NEGATE ); return;
        default: return; // unreachable
    }
}

static void binary() {
    // get operator
    TokenType operatorType = parser.previous.type;

    // get parsing rule
    ParseRule* rule = getRule( operatorType );

    // parse RHS w/ higher precedence than the binary operator
    // this makes the operator left-associative
    parsePrecedence( (Precedence)(rule->precedence + 1) );

    // emit token for operator
    switch( operatorType ) {
        case TOKEN_BANG_EQUAL:      emitBytes( OP_EQUAL, OP_NOT ); break;
        case TOKEN_EQUAL_EQUAL:     emitByte( OP_EQUAL ); break;
        case TOKEN_GREATER:         emitByte( OP_GREATER ); break;
        case TOKEN_GREATER_EQUAL:   emitBytes( OP_LESS, OP_NOT ); break;
        case TOKEN_LESS:            emitByte( OP_LESS ); break;
        case TOKEN_LESS_EQUAL:      emitBytes( OP_GREATER, OP_NOT ); break;
        case TOKEN_PLUS:            emitByte( OP_ADD ); break;
        case TOKEN_MINUS:           emitByte( OP_SUBTRACT ); break;
        case TOKEN_STAR:            emitByte( OP_MULTIPLY ); break;
        case TOKEN_SLASH:           emitByte( OP_DIVIDE ); break;
        default: return; // unreachable
    }
}

static void literal() {
    switch( parser.previous.type ) {
        case TOKEN_FALSE:   emitByte( OP_FALSE ); break;
        case TOKEN_NIL:     emitByte( OP_NIL ); break;
        case TOKEN_TRUE:    emitByte( OP_TRUE ); break;
        default: return; // unreachable
    }
}

static void string() {
    emitConstant( OBJ_VAL( makeString( parser.previous.start + 1, parser.previous.length - 2, NULL, 0 ) ) );
}

// parsing rules
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PRECEDENCE_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PRECEDENCE_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PRECEDENCE_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PRECEDENCE_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PRECEDENCE_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PRECEDENCE_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PRECEDENCE_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary, PRECEDENCE_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PRECEDENCE_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PRECEDENCE_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PRECEDENCE_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PRECEDENCE_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PRECEDENCE_COMPARISON},
    [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PRECEDENCE_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PRECEDENCE_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PRECEDENCE_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PRECEDENCE_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PRECEDENCE_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PRECEDENCE_NONE},
};

static ParseRule* getRule( TokenType type ) { return &rules[type]; }

static void parsePrecedence( Precedence precedence ) {
    // get next token
    advance();
    
    // the 1st token MUST be a prefix expression (by definition)
    ParseFn prefixRule = getRule( parser.previous.type )->prefix;
    if( NULL == prefixRule ) { error( "Expect expression." ); return; }
    prefixRule();

    // after parsing the left side, now we check the precedence of the next token
    // (note that EOF has precedence lower than assignment even, so it will exit this loop)
    while( precedence <= getRule( parser.current.type )->precedence ) {
        // get next token
        advance();

        // run infixRule
        ParseFn infixRule = getRule( parser.previous.type )->infix;
        infixRule();
    }
}

// compiles an expression
static void expression() { parsePrecedence( PRECEDENCE_ASSIGNMENT ); }

static void expressionStatement() {
    expression();
    consume( TOKEN_SEMICOLON, "Expect ';' after expression." );
    emitByte( OP_POP );
}

// compilres a print statement
static void printStatement() {
    expression();
    consume( TOKEN_SEMICOLON, "Expect ';' after value." );
    emitByte( OP_PRINT );
}

static void synchronize() {
    // we have a parser error, and we're in panicMode
    // after we synchronize, however, we'll still have an error, but we won't be in panic mode
    // this means we can keep parsing!
    parser.panicMode = false;

    // scan ahead to see where it is safe to continue parsing from
    while( TOKEN_EOF != parser.current.type ) {
        // if we just ended a statement (via a semicolon), then we're already synchronized!
        if( TOKEN_SEMICOLON == parser.previous.type ) return;

        // check if we're starting out a new statement
        switch( parser.current.type ) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN: return;
            default: advance(); break; // otherwise, we must scan forward!
        }
    }
}

// compiles a statement
static void statement() {
    if( match( TOKEN_PRINT ) ) printStatement();
    else expressionStatement();
}

// compiles a declaration
static void declaration() {
    statement();
    if( parser.panicMode ) synchronize();
}

// compiles source to chunk
bool compile( const char* source, Chunk* chunk ) {
    // start scanner
    #ifdef DEBUG_PRINT_SCAN
    printf( "== scanned tokens ==\n" );
    #endif
    initScanner( source );

    // set the chunk that we're compiling into
    compilingChunk = chunk;

    // initialize parser
    parser.hadError = false;
    parser.panicMode = false;
    advance();

    // compile each declaration
    while( !match( TOKEN_EOF ) ) declaration();

    // done (emit the return and print compiled bytecode)
    endCompiler();
    return !parser.hadError;
}
