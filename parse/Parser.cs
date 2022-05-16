using System; using System.Collections.Generic; using Lox.Scan; using Lox.Syntax; using static Lox.Scan.TokenType; namespace Lox.Parse {

sealed class SyntaxError : Exception {
    public readonly int Line;
    public SyntaxError( int line, string message ) : base( "[" + line + "] " + message ) => Line = line;
}

sealed class Parser {
    readonly List<Token> tokens_;
    int current_ = 0, errors_ = 0;

    // constructor
    public Parser( List<Token> tokens ) => tokens_ = tokens;

    // helpers
    bool Match( TokenType t ) {
        if( current_ == tokens_.Count || tokens_[current_].Type != t ) return false;
        current_++;
        return true;
    }

    bool Check( TokenType t ) {
        if( current_ == tokens_.Count || tokens_[current_].Type != t ) return false;
        return true;
    }

    Token Consume( TokenType t, string onError ) {
        if( current_ == tokens_.Count || tokens_[current_].Type != t ) throw new SyntaxError( Peek.Line, onError );
        return tokens_[current_++];
    }

    // properties
    bool IsAtEnd => current_ == tokens_.Count;
    public int Errors => errors_;
    Token Peek => current_ < tokens_.Count ? tokens_[current_] : new( WHITESPACE, "", Prior.Line, Prior.Start + Prior.Length, 0 );
    Token Prior => tokens_[current_ - 1];

    public void Synchronize() { // i.e. "MoveToNextStatement"
        while( ++current_ < tokens_.Count ) {
            if( SEMICOLON == Prior.Type ) return;
            switch( Peek.Type ) {
                case CLASS: case FUN: case VAR: case FOR:
                case IF: case WHILE: case RETURN:
                return;
            }
        }
    }

    public List<Statement> Parse( bool catchParseErrors ) {
        List<Statement> statements = new();
        current_ = errors_ = 0;
        while( tokens_.Count > current_ ) {
            if( catchParseErrors ) {
                try {
                    var statement = Declaration();
                    statements.Add( statement );
                } catch( SyntaxError e ) {
                    errors_++;
                    ColorConsole.Error( e.Line, e.Message );
                    Synchronize(); // i.e. move to the next statement
                }
            } else {
                var statement = Declaration();
                statements.Add( statement );
            }
        }
        return statements;
    }

    public Expression ParseExpression() {
        current_ = errors_ = 0;
        try {
            if( tokens_.Count > 0 ) return Expression();
        } catch( SyntaxError e ) {
            errors_++;
            ColorConsole.Error( e.Line, e.Message );
        }
        return new LiteralExpression( null );
    }
    
    Statement Declaration() {
        // TODO: try/catch? https://craftinginterpreters.com/statements-and-state.html
        if( Match( CLASS ) ) return ClassDeclarationStatement();
        if( Match( FUN ) ) return FunctionDeclarationStatement();
        if( Match( VAR ) ) return VariableDeclarationStatement();
        return Statement();
    }

    ClassDeclarationStatement ClassDeclarationStatement() {
        // get class name
        var name = Consume( IDENTIFIER, "Expected class name" );

        // get superclass
        VariableExpression? superclass = null;
        if( Match( LESS ) ) {
            Consume( IDENTIFIER, "expect superclass name" );
            superclass = new( Prior );
        }

        // left brace
        Consume( TokenType.LEFT_BRACE, "Expected '{' before class body" );

        // methods
        // TODO: these need to be "method-type" function declarations
        List<FunctionDeclarationStatement> methods = new();
        while( !Check( TokenType.RIGHT_BRACE ) && !IsAtEnd ) methods.Add( FunctionDeclarationStatement( "method" ) );

        // right brace
        Consume( TokenType.RIGHT_BRACE, "Expected '}' after class body" );
        return new( name, superclass, methods );
    }

    FunctionDeclarationStatement FunctionDeclarationStatement( string kind = "function" ) {
        // get function name
        var name = Consume( IDENTIFIER, $"Expected {kind} name" );

        // get parameters
        Consume( LEFT_PAREN, $"Expect '(' after {kind} name" );
        List<Token> parameters = new();
        if( !Check( RIGHT_PAREN ) ) do parameters.Add( Consume( IDENTIFIER, "Expect parameter name" ) ); while( Match( COMMA ) );
        Consume( RIGHT_PAREN, "Expect ')' after parameters" );
        if( parameters.Count >= 255 ) throw new SyntaxError( Peek.Line, "cannot have more than 255 arguments" );

        // get body
        Consume( LEFT_BRACE, $"Expect '{{' before {kind} body" );
        return new( name, parameters, BlockStatement().Statements );
    }

    Statement VariableDeclarationStatement() {
        var name = Consume( IDENTIFIER, "Expect variable name." ); 
        var initializer = Match( EQUAL ) ? Expression() : new LiteralExpression( null );
        Consume( SEMICOLON, "Expect ';' after variable declaration" );
        return new VariableDeclarationStatement( name, initializer );
    }

    Statement Statement() {
        if( Match( FOR ) ) return ForStatement();
        if( Match( IF ) ) return IfStatement();
        if( Match( RETURN ) ) return ReturnStatement();
        if( Match( WHILE ) ) return WhileStatement();
        if( Match( LEFT_BRACE ) ) return BlockStatement();
        return ExpressionStatement();
    }

    ReturnStatement ReturnStatement() {
        var keyword = Prior;
        Expression? value = null;
        if( !Check( SEMICOLON ) ) value = Expression();
        Consume( SEMICOLON, "Expected ';' after return value" );
        return new( keyword, value );
    }

    Statement ForStatement() { // note that there is no 'for' statement, we instead desugar the 'while' into a 'for'
        // initializer
        Consume( LEFT_PAREN, "Expect '(' after for" );
        var initializer = 
            Match( SEMICOLON ) ? null :
            Match( VAR ) ? VariableDeclarationStatement() : ExpressionStatement();

        // condition
        var condition = Check( SEMICOLON ) ? new LiteralExpression( true ) : Expression();
        Consume( SEMICOLON, "Expect ';' after loop condition." );
        
        // increment
        var increment = Check( RIGHT_PAREN ) ? null : Expression();
        Consume( RIGHT_PAREN, "Expect ')' after for clauses." );

        // body
        var body = Statement();

        // desugar the increment
        if( null != increment ) body = new BlockStatement( body, new ExpressionStatement( increment ) );

        // desugar the condition
        body = new WhileStatement( condition, body );

        // desugar the initializer
        return null != initializer ? new BlockStatement( initializer, body ) : body;
    }

    Statement WhileStatement() {
        // condition
        Consume( LEFT_PAREN, "Expect '(' after while" );
        var condition = Expression();
        Consume( RIGHT_PAREN, "Expect ')' after while condition" );

        // body
        return new WhileStatement( condition, body: Statement() );
    }

    Statement IfStatement() {
        // condition
        Consume( LEFT_PAREN, "Expect '(' after if" );
        var condition = Expression();
        Consume( RIGHT_PAREN, "Expect ')' after if condition" );

        // 'then' and 'else' statements
        var then = Statement();
        var @else = Match( ELSE ) ? Statement() : null;
        return new IfStatement( condition, then, @else );
    }

    BlockStatement BlockStatement() {
        var statements = new List<Statement>();
        while( !Check( RIGHT_BRACE ) && !IsAtEnd ) statements.Add( Declaration() );
        Consume( RIGHT_BRACE, "Expect '}' after block" );
        return new( statements );
    }

    Statement ExpressionStatement() {
        var value = Expression();
        if( !Match( SEMICOLON ) ) throw new SyntaxError( Peek.Line, "Syntax error - expected semicolon" );
        return new ExpressionStatement( value );
    }

    Expression Expression() => Assignment();

    Expression Assignment() {
        var expr = Or();
        if( Match( EQUAL ) ) {
            var equals = Prior;
            var value = Assignment();
            if( expr is VariableExpression ) return new AssignmentExpression( ((VariableExpression)expr).Name, value );
            else if( expr is GetExpression ) return new SetExpression( ((GetExpression)expr).Object, ((GetExpression)expr).Name, value );
            else throw new SyntaxError( equals.Line, "Syntax error - invalid assignment target" );
        }
        return expr;
    }

    Expression Or() {
        var left = And();
        while( Match( OR ) ) left = new LogicalExpression( left, Prior, And() );
        return left;
    }

    Expression And() {
        var left = Equality();
        while( Match( AND ) ) left = new LogicalExpression( left, Prior, Equality() );
        return left;
    }

    Expression Equality() {
        var left = Comparison();
        while( Match( EQUAL_EQUAL ) || Match( BANG_EQUAL ) ) left = new BinaryExpression( left, Prior, Comparison() );
        return left;
    }

    Expression Comparison() {
        var left = Term();
        while( Match( GREATER ) || Match( GREATER_EQUAL ) || Match( LESS ) || Match( LESS_EQUAL ) ) left = new BinaryExpression( left, Prior, Term() );
        return left;
    }

    Expression Term() {
        var left = Factor();
        while( Match( PLUS ) || Match( MINUS ) ) left = new BinaryExpression( left, Prior, Factor() );
        return left;
    }

    Expression Factor() {
        var left = Unary();
        while( Match( STAR ) || Match( SLASH ) ) left = new BinaryExpression( left, Prior, Unary() );
        return left;
    }

    Expression Unary() {
        if( Match( BANG ) || Match( MINUS ) ) return new UnaryExpression( Prior, Unary() );
        return Call();
    }

    Expression Call() {
        var expression = Primary();
        while( true ) {
            if( Match( LEFT_PAREN ) ) expression = FinishCall( callee: expression );
            else if( Match( DOT ) ) expression = new GetExpression( @object: expression, name: Consume( IDENTIFIER, "Expected property name after '.'" ) );
            else break;
        }
        return expression;

        Expression FinishCall( Expression callee ) {
            List<Expression> args = new();
            if( !Check( RIGHT_PAREN ) ) do args.Add( Expression() ); while( Match( COMMA ) );
            if( args.Count >= 255 ) throw new SyntaxError( Peek.Line, "cannot have more than 255 arguments" );
            return new CallExpression( callee, args, closeParen : Consume( RIGHT_PAREN, "Expect ')' after args" ) );
        }
    }

    Expression Primary() {
        if( Match( TRUE ) ) return new LiteralExpression( true );
        if( Match( FALSE ) ) return new LiteralExpression( false );
        if( Match( NIL ) ) return new LiteralExpression( null );
        if( Match( NUMBER ) || Match( STRING ) ) return new LiteralExpression( Prior.Value );
        if( Match( THIS ) ) return new ThisExpression( Prior );
        if( Match( IDENTIFIER ) ) return new VariableExpression( Prior );
        if( Match( LEFT_PAREN ) ) {
            var leftParenToken = Prior;
            var inner = Expression();
            if( !Match( RIGHT_PAREN ) ) throw new SyntaxError( leftParenToken.Line, "Syntax error - mismatched parenthesis" );
            return new GroupExpression( inner );
        }
        throw new SyntaxError( Peek.Line, "Syntax error - expected expression, but got: " + Peek );
    }
}

} // namespace
