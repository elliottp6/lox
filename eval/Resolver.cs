using System; using System.Collections.Generic; using Lox.Scan; using Lox.Syntax; using static Lox.Scan.TokenType; namespace Lox.Eval {

// the resolver processes a list of statements, and for every var, tells interpreter
// how many scopes (environments) there are between current scope & scope where var is defined
sealed class Resolver : Visitor<object?> {
    readonly Interpreter interpreter_;
    readonly List<Dictionary<string,bool>> scopes_ = new();

    // constructor
    public Resolver( Interpreter interpreter ) {
        interpreter_ = interpreter;
        
        // push globals
        // TODO: not needed?
        //BeginScope();
        //foreach( var g in interpreter.Globals() ) scopes_[0].Add( g, true );
    }

    // scope
    void BeginScope() => scopes_.Push( new() );
    void EndScope() => scopes_.Pop();

    // declare & define
    // split into "Declare" vs "Define" to handle cases where initializer refers to same name, e.g.:
    // var a = "outer";
    // {
    //      var a = a;
    // }
    void Declare( Token variable ) {
        if( !scopes_.TryPeek( out var scope ) ) return;
        scope.Add( (string)variable.Value, false );
    }

    void Define( Token variable ) {
        if( !scopes_.TryPeek( out var scope ) ) return;
        scope[(string)variable.Value] = true;
    }

    // statement/expression resolution
    public void Resolve( List<Statement> statements ) { foreach( var s in statements ) Resolve( s ); }
    void Resolve( Expression e ) => e.Accept( this );
    void Resolve( Statement s ) => s.Accept( this );

    void ResolveLocal( Expression e, Token name ) {
        for( var i = scopes_.End(); i >= 0; i-- ) {
            if( scopes_[i].ContainsKey( (string)name.Value ) ) {
                // TODO: interpreter_.Resolve( e, scopes_.End() - i );
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
        if( null != s.Initializer ) Resolve( s.Initializer );
        Define( s.Name );
        return null;
    }

    public object? VisitFunctionDeclarationStatement( FunctionDeclarationStatement s ) {
        Declare( s.Name );
        Define( s.Name );
        ResolveFunction( s );
        return null;
    }

    void ResolveFunction( FunctionDeclarationStatement s ) {
        BeginScope();
        foreach( var param in s.Parameters ) { Declare( param ); Define( param ); }
        Resolve( s.Body );
        EndScope();
    }

    // interesting expressions
    public object? VisitVariableExpression( VariableExpression e ) {
        if( scopes_.TryPeek( out var scope ) &&
            !scope[(string)e.Name.Value] ) throw new Exception( "Can't read local variable in its own initializer" );
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
