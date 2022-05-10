using System; using System.Collections.Generic; using Lox.Scan; using Lox.Syntax; namespace Lox.Eval {

// the resolver processes a list of statements, and for every var, tells interpreter
// how many scopes (environments) there are between current scope & scope where var is defined
// TODO: a cleaner design would be if the resolver actually created the side-data-structure that
// held a map between expressions & their scopes
// then this could just be passed to the interpreter
// also, we could/should avoid defining the global scope here, so the interpreter doesn't need to provide these globals to the resolver
sealed class Resolver : Visitor<object?> {
    readonly List<Dictionary<string,bool>> scopes_ = new();
    readonly Interpreter interpreter_;

    // constructor
    public Resolver( Interpreter interpreter ) {
        BeginScope(); // global scope
        foreach( var g in interpreter.Globals() ) scopes_.Peek().Add( g, true );
        interpreter_ = interpreter;
    }

    // scope
    void BeginScope() => scopes_.Push( new() );
    void EndScope() => scopes_.Pop();
    
    // variable is in scope, but not yet able to be accessed b/c its initializer is not yet run
    // in this way, the initialize sees this variable, but cannot use it, so we can throw an error (cannot refer to self in initilaizer)
    // (alternatively, we could use the value of outer-scope version of this variable, or perhaps just use 'nil', but that's not how lox works)
    void Declare( Token variable ) {
        if( !scopes_.Peek().TryAdd( (string)variable.Value, false ) )
            throw new Exception( $"variable already declared in this scope: {variable}" );
    }
    
    // after the initializer is run, the variable has a value, and can safely be accessed
    void Define( Token variable ) => scopes_.Peek()[(string)variable.Value] = true;

    // do both at once
    void DeclareDefine( Token variable ) { Declare( variable ); Define( variable ); }

    // statement/expression resolution
    public void Resolve( List<Statement> statements ) { foreach( var s in statements ) Resolve( s ); }
    void Resolve( Expression e ) => e.Accept( this );
    void Resolve( Statement s ) => s.Accept( this );

    void ResolveLocal( Expression e, Token name ) {
        for( var i = scopes_.End(); i >= 0; i-- ) {
            if( scopes_[i].ContainsKey( (string)name.Value ) ) {
                interpreter_.Resolve( e, scopes_.End() - i );
                // TODO: remove: Console.WriteLine( $"{name} resolved to scope {scopes_.End() - i}" );
                return;
            }
        }
    }

    // interesting statements
    public object? VisitBlockStatement( BlockStatement block ) {
        BeginScope();
        Resolve( block.Statements );
        EndScope();
        return null;
    }

    public object? VisitVariableDeclarationStatement( VariableDeclarationStatement s ) {
        Declare( s.Name );
        if( null != s.Initializer ) Resolve( s.Initializer ); // variable cannot refer to itself within its initializer, so run the initializer before we define it
        Define( s.Name );
        return null;
    }

    public object? VisitFunctionDeclarationStatement( FunctionDeclarationStatement s ) {
        DeclareDefine( s.Name ); // a function is allowed to refer to itself
        ResolveFunction( s );
        return null;
    }

    void ResolveFunction( FunctionDeclarationStatement s ) {
        BeginScope();
        foreach( var param in s.Parameters ) DeclareDefine( param );
        Resolve( s.Body );
        EndScope();
    }

    // interesting expressions
    public object? VisitVariableExpression( VariableExpression e ) {
        // see if the variable is defined before we access it
        // (it must be declared, of course, or else this would crash)
        if( scopes_.Peek().TryGetValue( (string)e.Name.Value, out var defined ) & !defined ) throw new Exception( $"Can't read local variable {e.Name} in its own initializer" );
        
        // resolve it        
        ResolveLocal( e, e.Name );
        return null;
    }

    public object? VisitAssignmentExpression( AssignmentExpression e ) {
        Resolve( e.Value );
        ResolveLocal( e, e.Name );
        return null;
    }

    // boring statements
    public object? VisitReturnStatement( ReturnStatement s ) {
        if( null != s.Value ) Resolve( s.Value );
        return null;
    }
    
    public object? VisitExpressionStatement( ExpressionStatement s ) {
        Resolve( s.Expression );
        return null;
    }

    public object? VisitIfStatement( IfStatement s ) {
        Resolve( s.Condition );
        Resolve( s.Then );
        if( null != s.Else ) Resolve( s.Else );
        return null;
    }

    public object? VisitWhileStatement( WhileStatement s ) {
        Resolve( s.Condition );
        Resolve( s.Body );
        return null;
    }
    
    // boring expressions
    public object? VisitBinaryExpression( BinaryExpression e ) {
        Resolve( e.Left );
        Resolve( e.Right );
        return null;
    }

    public object? VisitCallExpression( CallExpression e ) {
        Resolve( e.Callee );
        foreach( var arg in e.Args ) Resolve( arg );
        return null;
    }

    public object? VisitGroupExpression( GroupExpression e ) {
        Resolve( e.Inside );
        return null;
    }

    public object? VisitLiteralExpression( LiteralExpression e ) => null;

    public object? VisitLogicalExpression( LogicalExpression e ) {
        Resolve( e.Left );
        Resolve( e.Right );
        return null;
    }

    public object? VisitUnaryExpression( UnaryExpression e ) {
        Resolve( e.Right );
        return null;
    }
}

} // namespace
