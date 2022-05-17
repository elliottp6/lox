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
            new Token( TokenType.IDENTIFIER, "print", -1, -1, -1 ),
            (LoxFunction)delegate( object?[] args, Token closeParen ) {
                // check arity
                if( args.Length != 1 ) throw new RuntimeError( closeParen, $"mismatch # of arguments: expected {0} but got {args.Length}" );
                
                // print
                var s = Stringify( args[0] );
                Console.WriteLine( s );
                return s;
            });
        
        // define sleep function
        environment_.Define(
            new Token( TokenType.IDENTIFIER, "sleep", -1, -1, -1 ),
            (LoxFunction)delegate( object?[] args, Token closeParen ) {
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
        var function = callee as LoxFunction;
        if( null == function ) throw new RuntimeError( e.CloseParen, "expected a function" );
        return function( argValues, e.CloseParen );
    }

    object? Visitor<object?>.VisitGetExpression( GetExpression e ) {
        var inst = e.Object.Accept( this ) as LoxInstance;
        if( null == inst ) throw new RuntimeError( e.Name, "Only object instances have properties" );
        return inst.Get( e.Name );
    }

    object? Visitor<object?>.VisitSetExpression( SetExpression e ) {
        var inst = e.Object.Accept( this ) as LoxInstance;
        if( null == inst ) throw new RuntimeError( e.Name, "Only object instances have properties" );
        return inst.Set( e.Name, e.Value.Accept( this ) );
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

    object? Visitor<object?>.VisitThisExpression( ThisExpression e ) =>
        environment_.Get( e.Keyword, locals_[e] );

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
        // bind methods
        Dictionary<string,LoxMethod> methods = new();
        LoxMethod? constructor = null;
        var constructorArity = 0;
        foreach( var func in s.Methods ) {
            // get name and method
            var name = (string)func.Name.Value;
            var method = MakeMethod( func.Parameters, func.Body, environment_ );
            
            // check for constructor
            if( "init" == name ) {
                constructor = method;
                constructorArity = func.Parameters.Count;
                continue;
            }

            // otherwise, it's a normal method
            methods.Add( name, method );
        }

        // get superclass
        var super = s.Superclass?.Accept( this );
        if( null != super && !(super is LoxClass) )
            throw new RuntimeError( s.Superclass!.Name, "superclass must be a class" );

        // create class
        LoxClass c = new( (string)s.Name.Value, super as LoxClass, methods );

        // define the class constructor
        environment_.Define( s.Name,
            (LoxFunction)delegate( object?[] args, Token closeParen ) {
                // check arity
                if( args.Length != constructorArity )
                    throw new RuntimeError( closeParen, $"constructor # of args mistmach: expected {constructorArity} args but got {args.Length} args" );

                // allocate instance
                LoxInstance instance = new( c );

                // run constructor
                if( null != constructor ) {
                    var bound = constructor( instance );
                    bound( args, closeParen );
                }
                return instance;
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
        var function = MakeUnboundFunction( s.Parameters, s.Body );
        environment_.Define( s.Name, BindFunction( function, environment_ ) );
        return null;
    }

    LoxMethod MakeMethod( List<Token> parameters, List<Statement> body, Environment environment ) {
        var unboundFunction = MakeUnboundFunction( parameters, body );
        return (LoxMethod)delegate( LoxInstance instance ) {
            Environment thisEnvironment = new( environment );
            thisEnvironment.Define( "this", instance );
            return BindFunction( unboundFunction, thisEnvironment );
        };
    }

    LoxFunction BindFunction( LoxUnboundFunction unboundFunction, Environment e ) =>
        (LoxFunction)delegate( object?[] args, Token closeParen ) { return unboundFunction( args, closeParen, e ); };

    LoxUnboundFunction MakeUnboundFunction( List<Token> parameters, List<Statement> body ) =>
        (LoxUnboundFunction)delegate( object?[] args, Token closeParen, Environment environment ) {
                // put variables into local scope
                if( args.Length != parameters.Count ) throw new RuntimeError( closeParen, $"mismatch # of arguments: expected {0} but got {args.Length}" );
                Environment local = new( environment );
                for( var i = 0; i < parameters.Count; i++ ) local.Define( parameters[i], args[i] );
                
                // execute
                try { ExecuteBlock( body, local ); }
                catch( Return r ) { return r.Value; }
                return null;
            };

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
