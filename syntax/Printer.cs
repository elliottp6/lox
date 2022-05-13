using System; using System.Text; using System.Collections.Generic; using Lox.Scan; using NUnit.Framework; using static Lox.Scan.TokenType; namespace Lox.Syntax {

sealed class Printer : Visitor<string> {
    // top-level methods
    public void Print( List<Statement> statements ) {
        foreach( var statement in statements )
            ColorConsole.WriteLine( statement.Accept( this ), ConsoleColor.Magenta );
    }

    // expressions
    string Visitor<string>.VisitBinaryExpression( BinaryExpression e ) => $"({e.Operator.Value} {e.Left.Accept( this )} {e.Right.Accept( this )})";
    string Visitor<string>.VisitUnaryExpression( UnaryExpression e ) => $"({e.Operator.Value} {e.Right.Accept( this )})";
    string Visitor<string>.VisitGroupExpression( GroupExpression e ) => $"({e.Inside.Accept( this )})";
    string Visitor<string>.VisitLiteralExpression( LiteralExpression e ) => $"{(null == e.Value ? "nil" : e.Value is bool ? ((bool)e.Value ? "true" : "false") : e.Value is string ? "\"" + (string)e.Value + "\"" : e.Value)}";
    string Visitor<string>.VisitLogicalExpression( LogicalExpression e ) => $"({e.Operator.Value} {e.Left.Accept( this )} {e.Right.Accept( this )})";
    string Visitor<string>.VisitVariableExpression( VariableExpression e ) => $"{e.Name.Value}";
    string Visitor<string>.VisitAssignmentExpression( AssignmentExpression e ) => $"(= {e.Name.Value} {e.Value.Accept( this )})";

    string Visitor<string>.VisitCallExpression( CallExpression e ) {
        var b = new StringBuilder();
        b.Append( '(' );
        b.Append( e.Callee.Accept( this ) );
        foreach( var arg in e.Args ) { b.Append( ' ' ); b.Append( arg.Accept( this ) ); }
        b.Append( ')' );
        return b.ToString();
    }

    string Visitor<string>.VisitGetExpression( GetExpression e ) => $"(get {e.Object.Accept( this )} {e.Name.Value})";
    string Visitor<string>.VisitSetExpression( SetExpression e ) => $"(set {e.Object.Accept( this )} {e.Name.Value} {e.Value.Accept( this )})";
    string Visitor<string>.VisitThisExpression( ThisExpression e ) => $"this";

    // statements
    string Visitor<string>.VisitExpressionStatement( ExpressionStatement s ) => $"{s.Expression.Accept( this )}";
    string Visitor<string>.VisitVariableDeclarationStatement( VariableDeclarationStatement s ) => $"(define {s.Name.Value} {s.Initializer.Accept( this )})";
    
    string Visitor<string>.VisitBlockStatement( BlockStatement s ) {
        StringBuilder b = new();
        b.Append( "(block" );
        foreach( var statement in s.Statements ) { b.Append( ' ' ); b.Append( statement.Accept( this ) ); }
        b.Append( ')' );
        return b.ToString();
    }

    string Visitor<string>.VisitClassDeclarationStatement( ClassDeclarationStatement s ) {
        StringBuilder b = new();
        b.Append( "(class " );
        b.Append( s.Name );
        foreach( var method in s.Methods ) { b.Append( ' ' ); b.Append( method.Accept( this ) ); }
        b.Append( ')' );
        return b.ToString();
    }

    string Visitor<string>.VisitIfStatement( IfStatement s ) => $"(if {s.Condition.Accept( this )} {s.Then.Accept( this )}{(null != s.Else ? " " + s.Else.Accept( this ) : "")})";
    string Visitor<string>.VisitWhileStatement( WhileStatement s ) => $"(while {s.Condition.Accept( this )} {s.Body.Accept( this )})";

    string Visitor<string>.VisitFunctionDeclarationStatement( FunctionDeclarationStatement s ) {
        StringBuilder b = new();
        b.Append( "(fun " );
        b.Append( s.Name.Value );
        b.Append( " (" );
        var first = true;
        foreach( var parameter in s.Parameters ) { if( !first ) b.Append( ' ' ); b.Append( parameter ); first = false; }
        b.Append( ") (" );
        first = true;
        foreach( var statement in s.Body ) { if( !first ) b.Append( ' ' ); b.Append( statement.Accept( this ) ); first = false; }
        b.Append( "))" );
        return b.ToString();
    }

    string Visitor<string>.VisitReturnStatement( ReturnStatement s ) => "(return" + (null == s.Value ? ")" : " " + s.Value!.Accept( this ) + ")");

    // tests
    [Test] public static void Test() {
        var expr = new BinaryExpression(
            new UnaryExpression( new( MINUS, "-", -1, -1, -1 ), new LiteralExpression( 123 ) ),
            new Token( STAR, "*", -1, -1, -1 ),
            new GroupExpression( new LiteralExpression( 45.67 ) ) );
        Assert.AreEqual( "(* (- 123) (45.67))", expr.Accept( new Printer() ) );
    }
}

} // namespace
