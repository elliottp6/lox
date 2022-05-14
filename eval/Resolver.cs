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
    FunctionType currentFunction; enum FunctionType { NONE, FUNCTION, METHOD }
    ClassType currentClass; enum ClassType { NONE, CLASS }

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
    void Declare( Token variable ) { if( !scopes_.Peek().TryAdd( (string)variable.Value, false ) ) throw new Exception( $"variable already declared in this scope: {variable}" ); }
    void Define( string variable ) => scopes_.Peek()[variable] = true;
    void Define( Token variable ) => Define( (string)variable.Value );

    // statement/expression resolution
    public void Resolve( List<Statement> statements ) { foreach( var s in statements ) Resolve( s ); }
    void Resolve( Expression e ) => e.Accept( this );
    void Resolve( Statement s ) => s.Accept( this );

    void ResolveLocal( Expression e, Token name ) {
        for( var i = scopes_.End(); i >= 0; i-- ) {
            if( scopes_[i].ContainsKey( (string)name.Value ) ) {
                interpreter_.Resolve( e, scopes_.End() - i );
                return;
            }
        }
    }

    // interesting statements
    object? Visitor<object?>.VisitBlockStatement( BlockStatement block ) {
        BeginScope();
        Resolve( block.Statements );
        EndScope();
        return null;
    }

    object? Visitor<object?>.VisitClassDeclarationStatement( ClassDeclarationStatement s ) {
        var enclosingClass = currentClass;
        currentClass = ClassType.CLASS;
        Define( s.Name );
        BeginScope();
        Define( "this" );
        foreach( var m in s.Methods ) ResolveFunction( m, FunctionType.METHOD );
        EndScope();
        currentClass = enclosingClass;
        return null;
    }

    object? Visitor<object?>.VisitVariableDeclarationStatement( VariableDeclarationStatement s ) {
        Declare( s.Name );
        if( null != s.Initializer ) Resolve( s.Initializer ); // variable cannot refer to itself within its initializer, so run the initializer before we define it
        Define( s.Name );
        return null;
    }

    object? Visitor<object?>.VisitFunctionDeclarationStatement( FunctionDeclarationStatement s ) {
        Define( s.Name ); // a function is allowed to refer to itself
        ResolveFunction( s, FunctionType.FUNCTION );
        return null;
    }

    void ResolveFunction( FunctionDeclarationStatement s, FunctionType functionType ) {
        var enclosingFunction = currentFunction;
        currentFunction = functionType;
        BeginScope();
        foreach( var param in s.Parameters ) Define( param );
        Resolve( s.Body );
        EndScope();
        currentFunction = enclosingFunction;
    }

    // interesting expressions
    object? Visitor<object?>.VisitVariableExpression( VariableExpression e ) {
        // see if the variable is defined before we access it
        // (it must be declared, of course, or else this would crash)
        if( scopes_.Peek().TryGetValue( (string)e.Name.Value, out var defined ) & !defined ) throw new Exception( $"Can't read local variable {e.Name} in its own initializer" );
        
        // resolve it        
        ResolveLocal( e, e.Name );
        return null;
    }

    object? Visitor<object?>.VisitAssignmentExpression( AssignmentExpression e ) {
        Resolve( e.Value );
        ResolveLocal( e, e.Name );
        return null;
    }

    // boring statements
    object? Visitor<object?>.VisitReturnStatement( ReturnStatement s ) {
        if( FunctionType.NONE == currentFunction ) throw new Exception( "cannot return from top-level code" );
        if( null != s.Value ) Resolve( s.Value );
        return null;
    }
    
    object? Visitor<object?>.VisitExpressionStatement( ExpressionStatement s ) {
        Resolve( s.Expression );
        return null;
    }

    object? Visitor<object?>.VisitIfStatement( IfStatement s ) {
        Resolve( s.Condition );
        Resolve( s.Then );
        if( null != s.Else ) Resolve( s.Else );
        return null;
    }

    object? Visitor<object?>.VisitWhileStatement( WhileStatement s ) {
        Resolve( s.Condition );
        Resolve( s.Body );
        return null;
    }
    
    // boring expressions
    object? Visitor<object?>.VisitBinaryExpression( BinaryExpression e ) {
        Resolve( e.Left );
        Resolve( e.Right );
        return null;
    }

    object? Visitor<object?>.VisitCallExpression( CallExpression e ) {
        Resolve( e.Callee );
        foreach( var arg in e.Args ) Resolve( arg );
        return null;
    }

    object? Visitor<object?>.VisitGetExpression( GetExpression e ) {
        Resolve( e.Object );
        return null;
    }

    object? Visitor<object?>.VisitSetExpression( SetExpression e ) {
        Resolve( e.Object );
        Resolve( e.Value );
        return null;
    }

    object? Visitor<object?>.VisitThisExpression( ThisExpression e ) {
        if( ClassType.CLASS != currentClass ) throw new Exception( "'this' keyword cannot be used outside of a class definition" );
        ResolveLocal( e, e.Keyword );
        return null;
    }

    object? Visitor<object?>.VisitGroupExpression( GroupExpression e ) {
        Resolve( e.Inside );
        return null;
    }

    object? Visitor<object?>.VisitLiteralExpression( LiteralExpression e ) => null;

    object? Visitor<object?>.VisitLogicalExpression( LogicalExpression e ) {
        Resolve( e.Left );
        Resolve( e.Right );
        return null;
    }

    object? Visitor<object?>.VisitUnaryExpression( UnaryExpression e ) {
        Resolve( e.Right );
        return null;
    }
}

} // namespace
