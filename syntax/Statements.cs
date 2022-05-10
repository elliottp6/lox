using System.Collections.Generic; using Lox.Scan; namespace Lox.Syntax {

interface Statement { R Accept<R>( Visitor<R> v ); }

sealed class ClassDeclarationStatement : Statement {
    public readonly Token Name;
    public readonly List<FunctionDeclarationStatement> Methods;
    public ClassDeclarationStatement( Token name, List<FunctionDeclarationStatement> methods ) { Name = name; Methods = methods; }
    public R Accept<R>( Visitor<R> v ) => v.VisitClassDeclarationStatement( this );
}

sealed class ExpressionStatement : Statement {
    public readonly Expression Expression;
    public ExpressionStatement( Expression e ) => Expression = e;
    public R Accept<R>( Visitor<R> v ) => v.VisitExpressionStatement( this );
}

sealed class BlockStatement : Statement {
    public readonly List<Statement> Statements;
    public BlockStatement( List<Statement> statements ) => Statements = statements;
    public BlockStatement( params Statement[] statements ) => Statements = new( statements );
    public R Accept<R>( Visitor<R> v ) => v.VisitBlockStatement( this );
}

sealed class VariableDeclarationStatement : Statement {
    public readonly Token Name;
    public readonly Expression Initializer;
    public VariableDeclarationStatement( Token name, Expression initializer ) { Name = name; Initializer = initializer; }
    public R Accept<R>( Visitor<R> v ) => v.VisitVariableDeclarationStatement( this );
}

sealed class IfStatement : Statement {
    public readonly Expression Condition;
    public readonly Statement Then;
    public readonly Statement? Else;
    public IfStatement( Expression condition, Statement then, Statement? @else = null ) { Condition = condition; Then = then; Else = @else; }
    public R Accept<R>( Visitor<R> v ) => v.VisitIfStatement( this );
}

sealed class WhileStatement : Statement {
    public readonly Expression Condition;
    public readonly Statement Body;
    public WhileStatement( Expression condition, Statement body ) { Condition = condition; Body = body; }
    public R Accept<R>( Visitor<R> v ) => v.VisitWhileStatement( this );
}

sealed class FunctionDeclarationStatement : Statement {
    public readonly Token Name;
    public readonly List<Token> Parameters;
    public readonly List<Statement> Body;
    public FunctionDeclarationStatement( Token name, List<Token> parameters, List<Statement> body ) { Name = name; Parameters = parameters; Body = body; }
    public R Accept<R>( Visitor<R> v ) => v.VisitFunctionDeclarationStatement( this );
}

sealed class ReturnStatement : Statement {
    public readonly Token Keyword;
    public readonly Expression? Value;
    public ReturnStatement( Token keyword, Expression? value ) { Keyword = keyword; Value = value; }
    public R Accept<R>( Visitor<R> v ) => v.VisitReturnStatement( this );
}

} // namespace
