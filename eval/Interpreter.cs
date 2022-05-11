using System; using System.Collections.Generic; using Lox.Scan; using Lox.Syntax; using static Lox.Scan.TokenType; namespace Lox.Eval {

sealed class Interpreter : Visitor<object?> {
    Environment environment_ = new( parent: null );
    Dictionary<Expression,int> locals_ = new( new IdentityEqualityComparer<Expression>() ); // side table for adding data to expressions (in this case, )
      
    // list of variables in the most local possible scope
    public List<string> Globals() => environment_.Global().List();

    // adds an expression to the side table w/ scope information
    public void Resolve( Expression e, int scope ) => locals_.Add( e, scope );

    // constructor
    public Interpreter() {
        // print function
        environment_.Define(
            new( TokenType.IDENTIFIER, "print", -1, -1, -1 ),
            (LoxCallable)delegate( Interpreter interpreter, object?[] args, Token closeParen ) {
                // check arity
                if( args.Length != 1 ) throw new RuntimeError( closeParen, $"mismatch # of arguments: expected {0} but got {args.Length}" );
                
                // print
                var s = Stringify( args[0] );
                Console.WriteLine( s );
                return s;
            });
        
        // define sleep function
        environment_.Define(
            new( TokenType.IDENTIFIER, "sleep", -1, -1, -1 ),
            (LoxCallable)delegate( Interpreter interpreter, object?[] args, Token closeParen ) {
                // check arity
                if( args.Length != 1 ) throw new RuntimeError( closeParen, $"mismatch # of arguments: expected {0} but got {args.Length}" );
                if( !(args[0] is double) ) throw new RuntimeError( closeParen, $"mismatch argument 1: expected double" );

                // sleep
                System.Threading.Thread.Sleep( (int)(1000 * (double)(args[0] ?? 0)).Round() );
                return args[0];
            });
    }

    // top-level methods
    public void Interpret( List<Statement> statements, bool catchRuntimeExceptions ) {
        if( catchRuntimeExceptions ) {
            try {
                foreach( var statement in statements ) statement.Accept( this );
            } catch( RuntimeError error ) {
                ColorConsole.WriteLine( "[" + error.Token.Line + "] " + error.Message, ConsoleColor.Red );
                ColorConsole.WriteLine( "nil", ConsoleColor.DarkYellow );
            }
        } else foreach( var statement in statements ) statement.Accept( this );
    }

    // -- expressions -
    object? Visitor<object?>.VisitLiteralExpression( LiteralExpression e ) => e.Value;    
    
    object? Visitor<object?>.VisitLogicalExpression( LogicalExpression e ) {
        var left = e.Left.Accept( this );
        return e.Operator.Type switch {
            AND => IsTruthy( left ) ? e.Right.Accept( this ) : left,
            OR => IsTruthy( left ) ? left : e.Right.Accept( this ),
            _ => throw new RuntimeError( e.Operator, "incompatible operands for: " + e.Operator.Value )
        };
    }

    object? Visitor<object?>.VisitGroupExpression( GroupExpression e ) => e.Inside.Accept( this );

    object? Visitor<object?>.VisitCallExpression( CallExpression e ) {
        // eval callee
        var callee = e.Callee.Accept( this );
        
        // eval args
        var argValues = new object?[e.Args.Count];
        for( var i = 0; i < argValues.Length; i++ ) argValues[i] = e.Args[i].Accept( this );

        // call function
        var function = callee as LoxCallable;
        if( null == function ) throw new RuntimeError( e.CloseParen, "expected a function" );
        return function( this, argValues, e.CloseParen );
    }

    object? Visitor<object?>.VisitGetExpression( GetExpression e ) {
        var inst = e.Object.Accept( this ) as LoxInstance;
        if( null == inst ) throw new RuntimeError( e.Name, "Only object instances have properties" );
        return inst.Get( e.Name );
    }

    object? Visitor<object?>.VisitUnaryExpression( UnaryExpression e ) {
        var right = e.Right.Accept( this );
        return e.Operator.Type switch {
            MINUS when right is double => -(double)right!,
            BANG => !IsTruthy( right ),
            _ => throw new RuntimeError( e.Operator, "incompatible operand for: " + e.Operator.Value ),
        };
    }

    object? Visitor<object?>.VisitBinaryExpression( BinaryExpression e ) {
        object? left = e.Left.Accept( this ), right = e.Right.Accept( this );
        return e.Operator.Type switch {
            PLUS when (left is string) & (right is string) => (string)left! + (string)right!,
            PLUS when (left is double) & (right is double) => (double)left! + (double)right!,
            MINUS when (left is double) & (right is double) => (double)left! - (double)right!,
            STAR when (left is double) & (right is double) => (double)left! * (double)right!,
            SLASH when (left is double) & (right is double) => (double)left! / (double)right!,
            GREATER when (left is double) & (right is double) => (double)left! > (double)right!,
            GREATER_EQUAL when (left is double) & (right is double) => (double)left! >= (double)right!,
            LESS when (left is double) & (right is double) => (double)left! < (double)right!,
            LESS_EQUAL when (left is double) & (right is double) => (double)left! <= (double)right!,
            EQUAL_EQUAL => Equal( left, right ), // == not the same as eq
            BANG_EQUAL => !Equal( left, right ), // != not the same as !eq
            _ => throw new RuntimeError( e.Operator, "incompatible operands for: " + e.Operator.Value )
        };
    }

    object? Visitor<object?>.VisitVariableExpression( VariableExpression e ) =>
        environment_.Get( e.Name, locals_[e] );

    object? Visitor<object?>.VisitAssignmentExpression( AssignmentExpression e ) {
        var rvalue = e.Value.Accept( this );
        environment_.Set( e.Name, rvalue, locals_[e] );
        return rvalue;
    }

    // -- statements --    
    object? Visitor<object?>.VisitExpressionStatement( ExpressionStatement s ) {
        s.Expression.Accept( this );
        return null;
    }

    object? Visitor<object?>.VisitBlockStatement( BlockStatement s ) {
        ExecuteBlock( s.Statements, new( environment_ ) );
        return null;
    }

    object? Visitor<object?>.VisitClassDeclarationStatement( ClassDeclarationStatement s ) {
        // create class
        LoxClass c = new( (string)s.Name.Value );

        // define the class constructor
        environment_.Define( s.Name,
            (LoxCallable)delegate( Interpreter interpreter, object?[] args, Token closeParen ) {
                // check arigty
                if( args.Length != 0 ) throw new RuntimeError( closeParen, $"constructor cannot take any arguments" );

                // construct instance using cached class
                return new LoxInstance( c );
            });
        return null;
    }

    void ExecuteBlock( List<Statement> statements, Environment e ) {
        var backup = environment_;
        environment_ = e;
        try { foreach( var statement in statements ) statement.Accept( this ); } finally { environment_ = backup; }
    }

    object? Visitor<object?>.VisitVariableDeclarationStatement( VariableDeclarationStatement s ) {
        environment_.Define( s.Name, null != s.Initializer ? s.Initializer.Accept( this ) : null );
        return null;
    }

    object? Visitor<object?>.VisitIfStatement( IfStatement s ) {
        if( IsTruthy( s.Condition.Accept( this ) ) ) return s.Then.Accept( this );
        return null == s.Else ? null : s.Else.Accept( this );
    }

    object? Visitor<object?>.VisitWhileStatement( WhileStatement s ) {
        while( IsTruthy( s.Condition.Accept( this ) ) ) s.Body.Accept( this );
        return null;
    }

    object? Visitor<object?>.VisitFunctionDeclarationStatement( FunctionDeclarationStatement s ) {
        var closure = environment_;
        environment_.Define(
            s.Name,
            (LoxCallable)delegate( Interpreter interpreter, object?[] args, Token closeParen ) {
                // put variables into scope
                if( args.Length != s.Parameters.Count ) throw new RuntimeError( closeParen, $"mismatch # of arguments: expected {0} but got {args.Length}" );
                Environment local = new( closure );
                for( var i = 0; i < s.Parameters.Count; i++ ) local.Define( s.Parameters[i], args[i] );
                
                // execute
                try { interpreter.ExecuteBlock( s.Body, local ); }
                catch( Return r ) { return r.Value; }
                return null;
            });
        return null;
    }

    object? Visitor<object?>.VisitReturnStatement( ReturnStatement s ) =>
        throw new Return( null == s.Value ? null : s.Value!.Accept( this ) );

    // utilities
    // TODO: reference equality vs value equality?
    static bool Equal( object? a, object? b ) =>
        ((null == a) & (null == b)) || ((a is double) & (b is double) ? (double)a! == (double)b! : null != a && a.Equals( b ));
    
    static bool IsTruthy( object? o ) =>
        o is bool ? (bool)o : null == o ? false : !(o is double) || .0 == (double)o;
    
    static string Stringify( object? o ) => null != o ? o.ToString()! : "nil";
}

} // namespace
