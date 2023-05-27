// good read on Pratt parsers: http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memcmp
#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"
#include "object.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

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

// top-level parser object
typedef struct {
    Token current, previous;
    bool hadError, panicMode;
} Parser;

// function pointer for parsing expressions
typedef void (*ParseFn)( bool canAssign );

// Pratt parser parsing rule
typedef struct {
    ParseFn prefix, infix;
    Precedence precedence;
} ParseRule;

// local variable
typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

// closed-over variables
typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

// type of function the compiler is compiling
typedef enum {
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_INITIALIZER,
    TYPE_SCRIPT
} FunctionType;

// compiler state
typedef struct Compiler {
    struct Compiler* enclosing; // the parent function's compiler
    ObjFunction* function; // current function being compiled
    FunctionType type; // type of current function being compiled
    int localCount, scopeDepth;
    Local locals[UINT8_COUNT];
    Upvalue upvalues[UINT8_COUNT];
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
} ClassCompiler;

// globals 
Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

// initializes the compiler state
static void initCompiler( Compiler* compiler, FunctionType type ) {
    // setup variable tracking
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    
    // setup current function
    compiler->type = type;
    compiler->function = NULL;
    compiler->function = newFunction();
   
    // push new compiler
    compiler->enclosing = current;
    current = compiler;

    // give the function a name
    if( TYPE_SCRIPT != type ) current->function->name = makeString( parser.previous.start, parser.previous.length );
    else current->function->name = makeString( "main", 4 );

    // reserve the 1st stack slot for the main function object (without a name so we cannot refer to it within the code)
    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;

    // non-fuctions have access to 'this', which is placed into the 1st stack slot
    // question: why do we allow 'this' at the TYPE_SCRIPT scope?
    if( type != TYPE_FUNCTION ) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

// returns current chunk being compiled
static Chunk* currentChunk() { return &current->function->chunk; }

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
    // TODO: instead of doing this here, we should have this be part of the scanner,
    //       so we can print out debug info before entering the compiler
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

// called for implicit returns only (or a return w/o a value), where we usually return NIL (except for initializers, where we return 'this')
static void emitReturn() {
    if( current->type == TYPE_INITIALIZER ) {
        emitBytes( OP_GET_LOCAL, 0 ); // 'this'
    } else {
        emitByte( OP_NIL ); // nil
    }
    emitByte( OP_RETURN );
}

static int emitJump( uint8_t jumpInstruction ) {
    emitByte( jumpInstruction );
    emitBytes( 0xff, 0xff ); // placeholder for jump
    return currentChunk()->count - 2; // address of jump instruction
}

static void patchJump( int jump ) {
    // -2 to adjust for jump instruction itself
    int jumpOffset = currentChunk()->count - jump - 2;

    // see if the offset we're jumping is too large for a 16-bit short jump
    if( jumpOffset > UINT16_MAX ) error( "Too much code to jump over for a 16-bit jump." );

    // encode the 'jumpOffset' into the bytecode as a 16-bit big-endian value
    currentChunk()->code[jump] = (jumpOffset >> 8) & 0xff;
    currentChunk()->code[jump + 1] = jumpOffset & 0xff;
}

static void emitLoop( int loopStart ) {
    // emit the loop instruction
    emitByte( OP_LOOP );

    // offset must include the LOOP instruction itself
    int offset = currentChunk()->count - loopStart + 2;
    if( offset > UINT16_MAX ) error( "Loop body too large for a 16-bit jump." );

    // emit the offset parameter
    emitBytes( (offset >> 8) & 0xff, offset & 0xff );
}

// ends a chunk
static ObjFunction* endCompiler() {
    // return from our 'main' function
    emitReturn();

    // get the compiled function
    ObjFunction* function = current->function;

    // disassemble code before running it
    #ifdef DEBUG_PRINT_CODE
    if( !parser.hadError ) {
        printConstants( currentChunk() );
        printf( "=> bytecode for: " );
        printFunction( function );
        printf( "\n" );
        disassembleChunk( currentChunk() );
    }
    #endif

    // restore enclosing function's compiler
    current = current->enclosing;
    return function;
}

// pushes new scope for compiler
static void beginScope() {
    current->scopeDepth++;    
}

// pops scope for compiler
static void endScope() {
    current->scopeDepth--;

    // pop locals TODO: add a POPN instruction so we don't need so many individual pops
    while( current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth ) {
        if( current->locals[current->localCount - 1].isCaptured ) emitByte( OP_CLOSE_UPVALUE );
        else emitByte( OP_POP );
        current->localCount--;
    }
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
static uint8_t identifierConstant( Token* name );
static void varDeclaration();

// parses number
static void number( bool canAssign ) {
    double value = strtod( parser.previous.start, NULL );
    emitConstant( NUMBER_VAL( value ) );
}

// parses grouping (prefix expression)
static void grouping( bool canAssign ) {
    expression();
    consume( TOKEN_RIGHT_PAREN, "Expect ')' after expression." );
}

// prases unary (prefix expression)
static void unary( bool canAssign ) {
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

static void binary( bool canAssign ) {
    // get operator & parsing rule
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule( operatorType );

    // parse RHS w/ higher precedence than the binary operator
    // (this makes the operator left-associative)
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

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if( !check( TOKEN_RIGHT_PAREN ) ) {
        do {
            expression();
            if( 255 == argCount ) error( "Can't have more than 255 arguments." );
            argCount++;
        } while( match( TOKEN_COMMA ) );
    }
    consume( TOKEN_RIGHT_PAREN, "Expect ')' after arguments." );
    return argCount;
}

static void call( bool canAssign ) {
    uint8_t argCount = argumentList();
    emitBytes( OP_CALL, argCount );
}

static void dot( bool canAssign ) {
    consume( TOKEN_IDENTIFIER, "Expect property name after '.'." );
    uint8_t name = identifierConstant( &parser.previous );

    if( canAssign && match( TOKEN_EQUAL ) ) {
        expression();
        emitBytes( OP_SET_PROPERTY, name );
    } else {
        emitBytes( OP_GET_PROPERTY, name );
    }
}

static void literal( bool canAssign ) {
    switch( parser.previous.type ) {
        case TOKEN_FALSE:   emitByte( OP_FALSE ); break;
        case TOKEN_NIL:     emitByte( OP_NIL ); break;
        case TOKEN_TRUE:    emitByte( OP_TRUE ); break;
        default: return; // unreachable
    }
}

static void string( bool canAssign ) {
    emitConstant( OBJ_VAL( makeString( parser.previous.start + 1, parser.previous.length - 2 ) ) );
}

// figure out the stack index for the local. note that this goes backwards to inner scoped variables shadow those of outer scopes.
static int resolveLocal( Compiler* compiler, Token* name ) {
    for( int i = compiler->localCount - 1; i >= 0; i-- ) {
        Local* local = &compiler->locals[i];

        // we found a match!
        if( lexemesEqual( name, &local->name ) ) {
            // if the variable is undefined, then we must be reading it within its own initializer
            if( -1 == local->depth ) error( "Can't read local variable in its own initializer." );
            return i;
        }
    }
    return -1; // we couldn't find it, so it must be a global variable
}

static int addUpvalue( Compiler* compiler, uint8_t index, bool isLocal ) {
    // check to see if there's already an upvalue for this
    int upvalueCount = compiler->function->upvalueCount;
    for( int i = 0; i < upvalueCount; i++ ) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if( upvalue->index == index && upvalue->isLocal == isLocal ) return i;
    }

    // otherwise: add the upvalue 
    if( UINT8_COUNT == upvalueCount ) { error( "Too many closure variables in function." ); return 0; }
    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue( Compiler* compiler, Token* name ) {
    // base case: see if enclosing function contains a local we can close over
    if( NULL == compiler->enclosing ) return -1;
    int local = resolveLocal( compiler->enclosing, name );
    if( -1 != local ) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue( compiler, (uint8_t)local, true );
    }

    // otherwise: no local found, so try to find upvalue in the enclosing compiler
    int upvalue = resolveUpvalue( compiler->enclosing, name );
    if( -1 != upvalue ) return addUpvalue( compiler, (uint8_t)upvalue, false );

    // otherwise: we have a global
    return -1;
}

static void namedVariable( Token name, bool canAssign ) {
    // see if this is a local variable
    int arg = resolveLocal( current, &name );
    uint8_t getOp, setOp;
    if( -1 != arg ) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if( -1 != (arg = resolveUpvalue( current, &name )) ) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
        arg = identifierConstant( &name ); // get index for embedded constant
    }

    // check if this is a variable assignment -- note: we could instead check for TOKEN_EQUAL, and report "Invalid assignment target." (for example, 2 * x = 3 would hit this), but we don't have to, b/c the expression would end at 'x', and therefore expect ';' instead of '=', so we get an error anyway
    if( canAssign && match( TOKEN_EQUAL ) ) {
        expression();
        emitBytes( setOp, (uint8_t)arg );
        return;
    }

    // otherwise, this is a regular get expression
    emitBytes( getOp, (uint8_t)arg );
}

static void variable( bool canAssign ) {
    namedVariable( parser.previous, canAssign );
}

static void this_( bool canAssign ) {
    if( NULL == currentClass ) {
        error( "Can't use 'this' outside of a class." );
        return;
    }

    variable( false );
}

static void and( bool canAssign ) {
    // don't evaluate RHS if LHS was false
    int endJump = emitJump( OP_JUMP_IF_FALSE ); // <-- OP_JUMP_IF_FALSE leaves value on stack precisely for this case

    // parse RHS (also pops stack, b/c RHS becomes the new stack value)
    emitByte( OP_POP );
    parsePrecedence( PRECEDENCE_AND );

    // location to skip to
    patchJump( endJump );
}

static void or( bool canAssign ) {
    // if !LHS, then jump to RHS
    int elseJump = emitJump( OP_JUMP_IF_FALSE ), endJump = emitJump( OP_JUMP );

    // parse RHS
    patchJump( elseJump );
    emitByte( OP_POP );
    parsePrecedence( PRECEDENCE_OR );

    // location to skip to
    patchJump( endJump );
}

// parsing rules
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, call,   PRECEDENCE_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PRECEDENCE_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_DOT]           = {NULL,     dot,    PRECEDENCE_CALL},
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
    [TOKEN_IDENTIFIER]    = {variable, NULL,   PRECEDENCE_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PRECEDENCE_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PRECEDENCE_NONE},
    [TOKEN_AND]           = {NULL,     and,   PRECEDENCE_AND},
    [TOKEN_CLASS]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PRECEDENCE_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PRECEDENCE_NONE},
    [TOKEN_OR]            = {NULL,     or,   PRECEDENCE_OR},
    [TOKEN_PRINT]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PRECEDENCE_NONE},
    [TOKEN_THIS]          = {this_,    NULL,   PRECEDENCE_NONE},
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
    
    // prefix parser
    ParseFn prefixRule = getRule( parser.previous.type )->prefix;
    if( NULL == prefixRule ) { error( "Expect expression." ); return; }
    bool canAssign = precedence <= PRECEDENCE_ASSIGNMENT;
    prefixRule( canAssign );

    // if we didn't consume the '=' token before, but we can assign, then it must be an expression like "5 = 3", because we didn't hit a variable to assign
    if( canAssign && match( TOKEN_EQUAL ) ) error( "Invalid assignment target." );

    // infix parsing loop
    while( precedence <= getRule( parser.current.type )->precedence ) {
        advance();
        ParseFn infixRule = getRule( parser.previous.type )->infix;
        infixRule( canAssign );
    }
}

// compiles an expression
static void expression() { parsePrecedence( PRECEDENCE_ASSIGNMENT ); }

static void expressionStatement() {
    expression();
    consume( TOKEN_SEMICOLON, "Expect ';' after expression." );
    emitByte( OP_POP );
}

// compile a block statement
static void block() {
    // until the block closes, parse declarations (i.e. top level statements)
    while( !check( TOKEN_RIGHT_BRACE ) && !check( TOKEN_EOF ) ) declaration();

    // consume the right brace if we hit EOF
    consume( TOKEN_RIGHT_BRACE, "Expect '}' after block." );
}

// compiles an if statement
static void ifStatement() {
    // condition
    consume( TOKEN_LEFT_PAREN, "Expect '(' after 'if'." );
    expression();
    consume( TOKEN_RIGHT_PAREN, "Expect ')' after condition." ); 

    // "if" block
    int jumpPastIf = emitJump( OP_JUMP_IF_FALSE );
    emitByte( OP_POP ); // pop condition
    statement();
    int jumpPastElse = emitJump( OP_JUMP );

    // "else" block
    patchJump( jumpPastIf );
    emitByte( OP_POP ); // pop condition
    if( match( TOKEN_ELSE ) ) statement();

    // "end-if"
    patchJump( jumpPastElse );
}

// compiles a print statement
static void printStatement() {
    expression();
    consume( TOKEN_SEMICOLON, "Expect ';' after value." );
    emitByte( OP_PRINT );
}

static void whileStatement() {
    // save start
    int loopStart = currentChunk()->count;

    // condition
    consume( TOKEN_LEFT_PAREN, "Expect '(' after 'while'." );
    expression();
    consume( TOKEN_RIGHT_PAREN, "Expect ')' after condition." );

    // if false, goto exit
    int exitJump = emitJump( OP_JUMP_IF_FALSE );

    // body
    emitByte( OP_POP );
    statement();
    emitLoop( loopStart ); // ...could just use a signed integer jump instead?

    // exit
    patchJump( exitJump );
    emitByte( OP_POP );
}

static void forStatement() {
    // begin scope
    consume( TOKEN_LEFT_PAREN, "Expect '(' after 'for'." );
    beginScope();
    
    // initializer
    if( match( TOKEN_SEMICOLON ) ); else if( match( TOKEN_VAR ) ) varDeclaration(); else expressionStatement();

    // condition
    int loopStart = currentChunk()->count, exitJump = -1;
    if( !match( TOKEN_SEMICOLON ) ) {
        expression();
        consume( TOKEN_SEMICOLON, "Expect ';' after loop condition." );
        exitJump = emitJump( OP_JUMP_IF_FALSE ); // leave the loop if the condition is false
        emitByte( OP_POP ); // pop condition
    }
    
    // increment
    if( !match( TOKEN_RIGHT_PAREN ) ) {
        // increment doesn't run on first loop iteration, so jump over it
        int bodyJump = emitJump( OP_JUMP ), incrementStart = currentChunk()->count;
        
        // increment body
        expression();
        emitByte( OP_POP );
        consume( TOKEN_RIGHT_PAREN, "Expect ')' after for clauses." );

        // go back to the top of the for loop
        emitLoop( loopStart );

        // now, change the loopStart to incrementStart, so that the body will loop back to the increment
        loopStart = incrementStart;

        // jump here to skip the initializer
        patchJump( bodyJump );
    }

    // body
    statement();
    emitLoop( loopStart );

    // exit
    if( -1 != exitJump ) {
        patchJump( exitJump );
        emitByte( OP_POP ); // pop condition
    }
    
    // end scope
    endScope();
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

static void returnStatement() {
    // TODO: EP disabled this, so we can actually call "return" on top-level to avoid getting a NIL value
    // do not allow a return from top-level
    // if( TYPE_SCRIPT == current->type ) error( "Can't return from top-level code." );

    // return nil
    if( match( TOKEN_SEMICOLON ) ) { emitReturn(); return; }

    // prevent returning a value from an initializer (emitReturn instead will return 'this')
    if( current->type == TYPE_INITIALIZER ) {
        error( "Can't return a value from an initializer." );
    }

    // return value
    expression();
    consume( TOKEN_SEMICOLON, "Expect ';' after return value." );
    emitByte( OP_RETURN );
}

// compiles a statement
static void statement() {
    if( match( TOKEN_PRINT ) ) printStatement();
    else if( match( TOKEN_FOR ) ) forStatement();
    else if( match( TOKEN_IF ) ) ifStatement();
    else if( match( TOKEN_WHILE ) ) whileStatement();
    else if( match( TOKEN_LEFT_BRACE ) ) { beginScope(); block(); endScope(); }
    else if( match( TOKEN_RETURN ) ) returnStatement();
    else expressionStatement();
}

// creates a new local variable
static void addLocal( Token name ) {
    // ensure we don't overflow our maximum # of locals
    if( UINT8_COUNT == current->localCount ) {
        error( "Too many local variables in function." );
        return;
    }
    
    // otherwise, create the local
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1; // special value which indicates that the variable is declared but undefined
    local->isCaptured = false;
}

static void declareVariable() {
    // globals do not need to be declared (they're late bound, so we don't track them)
    if( 0 == current->scopeDepth ) return;

    // get the name of the local variable
    Token* name = &parser.previous;
    
    // check backwards up the stack of local variables to see if another variable within this same scope has the same name
    for( int i = current->localCount - 1; i >= 0; i-- ) {
        // get the local variable
        Local* local = &current->locals[i];

        // if we escape the current scope, bail
        // (note that '-1' depth means the local is undefined)
        if( -1 != local->depth && local->depth < current->scopeDepth ) break;

        // otherwise, check if the variable's name matches another varible that's in this same scope
        if( lexemesEqual( name, &local->name ) ) error( "Already a variable with this name in this scope." );
    }

    // create the new local variable
    addLocal( *name );
}

static void markInitialized() { // give the variable a depth value, which marks it as "defined"
    if( 0 == current->scopeDepth ) return; // global variables are not locals
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable( uint8_t global ) {
    // define local variable: no OPCODE required to define variable, b/c we just let the value sit in the stack AS the local variable
    if( current->scopeDepth > 0 ) {
        markInitialized();
        return;
    }

    // define global varible
    emitBytes( OP_DEFINE_GLOBAL, global );
}

static uint8_t identifierConstant( Token* name ) {
    return makeConstant( OBJ_VAL( makeString( name->start, name->length ) ) );
}

static uint8_t parseVariable( const char* errorMessage ) {
    // consume the identifier
    consume( TOKEN_IDENTIFIER, errorMessage );
    
    // declare the varible (but don't define it yet)
    declareVariable();

    // if it's a local varible, we do NOT put the identifier into the global constant table
    if( current->scopeDepth > 0 ) return 0;

    // turn token into a constant (for global variables only)
    return identifierConstant( &parser.previous );
}

// TODO: what if a user declares a global varible twice with the same name?
//       right now, we'll create another identical constant, but the actual global in the VM will be overwritten
static void varDeclaration() {
    // create constant w/ name of variable
    uint8_t global = parseVariable( "Expect variable name." );

    // check for variable initializer
    if( match( TOKEN_EQUAL )) expression(); else emitByte( OP_NIL );

    // must terminate statement w/ semicolon
    consume( TOKEN_SEMICOLON, "Expect ';' after variable declaration." );

    // push instruction which causes VM to put an entry into its globals table
    // or, if it's a local variable, then just mark it as defined
    defineVariable( global );
}

static void function( FunctionType type ) {
    // creates sub-compiler for this function
    Compiler compiler;
    initCompiler( &compiler, type );

    // this function has its own scope for variables (no need to call endScope, b/c we endCompiler)
    beginScope();

    // parse parameters
    consume( TOKEN_LEFT_PAREN, "Expect '(' after function name." );
    if( !check( TOKEN_RIGHT_PAREN ) ) {
        do {
            current->function->arity++;
            if( current->function->arity > 255 ) errorAtCurrent( "Cannot have more than 255 parameters." );
            uint8_t constant = parseVariable( "Expect parameter name." );
            defineVariable( constant );
        } while( match( TOKEN_COMMA ) );
    }
    consume( TOKEN_RIGHT_PAREN, "Expect ')' after parameters." );
    
    // parse body
    consume( TOKEN_LEFT_BRACE, "Expect '{' before function body." );
    block();

    // push function onto stack
    ObjFunction* function = endCompiler();
    emitBytes( OP_CLOSURE, makeConstant( OBJ_VAL( function ) ) );

    // push upvalue info
    for( int i = 0; i < function->upvalueCount; i++ ) {
        emitByte( compiler.upvalues[i].isLocal ? 1 : 0 );
        emitByte( compiler.upvalues[i].index );
    }
}

static void method() {
    // consume the method name as a constant
    consume( TOKEN_IDENTIFIER, "Expect method name." );
    uint8_t constant = identifierConstant( &parser.previous );

    // compile method body as function
    FunctionType type = TYPE_METHOD;
    if( parser.previous.length == 4 && memcmp( parser.previous.start, "init", 4 ) == 0 ) {
        type = TYPE_INITIALIZER;
    }
    function( type );

    // bind function as method to class which is on the stack right above function
    // (i.e. OP_METHOD is a 2-argument operator, taking class & function & binding them, plus a constant for the name)
    emitBytes( OP_METHOD, constant );
}

static void classDeclaration() {
    // consume class name
    consume( TOKEN_IDENTIFIER, "Expect class name." );
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant( &className );

    // declare class name as a variable pointing to class
    declareVariable();
    emitBytes( OP_CLASS, nameConstant );
    defineVariable( nameConstant );

    // allocate classCompiler on stack, and set the global currentClass to it
    ClassCompiler classCompiler;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    // load class onto stack, s.t. each method() knows where to find the class
    namedVariable( className, false );

    // compile class body
    consume( TOKEN_LEFT_BRACE, "Expect '{' before class body." );
    while( !check( TOKEN_RIGHT_BRACE ) && !check( TOKEN_EOF ) ) {
        method();
    }
    consume( TOKEN_RIGHT_BRACE, "Expect '}' after class body." );

    // pop class off stack
    emitByte( OP_POP );

    // we're done compiling this class, so restore currentClass
    currentClass = currentClass->enclosing;
}

static void funDeclaration() {
    // declare a variable for the function. mark it initialized b/c it's legal for the function to self-reference
    uint8_t global = parseVariable( "Expect function name." );
    markInitialized();
    
    // parse the function body
    function( TYPE_FUNCTION );

    // emit opcode which defines the function based on what's on the stack (which is the function object itself)
    defineVariable( global );
}

// compiles a declaration
static void declaration() {
    // dispatch on declaration type
    if( match( TOKEN_CLASS ) ) classDeclaration();
    else if( match( TOKEN_FUN ) ) funDeclaration();
    else if( match( TOKEN_VAR ) ) varDeclaration();
    else statement();

    // deal with panics
    if( parser.panicMode ) synchronize();
}

// compiles source to chunk
ObjFunction* compile( const char* source ) {
    // start scanner
    #ifdef DEBUG_PRINT_SCAN
    printf( "== scanned tokens ==\n" );
    #endif
    initScanner( source );

    // initialize the compiler
    Compiler compiler;
    initCompiler( &compiler, TYPE_SCRIPT );

    // initialize parser
    parser.hadError = false;
    parser.panicMode = false;
    advance();

    // compile each declaration
    while( !match( TOKEN_EOF ) ) declaration();

    // return the compiled function w/ the name "main"
    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
    Compiler* compiler = current;
    while( NULL != compiler ) {
        markObject( (Obj*)compiler->function );
        compiler = compiler->enclosing;
    }
}
