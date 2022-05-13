using System.Collections.Generic; using Lox.Scan; namespace Lox.Syntax {

interface Expression { R Accept<R>( Visitor<R> v ); }

sealed class BinaryExpression : Expression {
    public readonly Token Operator;
    public readonly Expression Left, Right;
    public BinaryExpression( Expression left, Token op, Expression right ) { Left = left; Operator = op; Right = right; } 
    public R Accept<R>( Visitor<R> v ) => v.VisitBinaryExpression( this );
}

sealed class GroupExpression : Expression {
    public readonly Expression Inside;
    public GroupExpression( Expression inside ) => Inside = inside;
    public R Accept<R>( Visitor<R> v ) => v.VisitGroupExpression( this );
}

sealed class LiteralExpression : Expression {
    public readonly object? Value;
    public LiteralExpression( object? value ) => Value = value;
    public R Accept<R>( Visitor<R> v ) => v.VisitLiteralExpression( this );
}

sealed class LogicalExpression : Expression {
    public readonly Token Operator;
    public readonly Expression Left, Right;
    public LogicalExpression( Expression left, Token op, Expression right ) { Left = left; Operator = op; Right = right; } 
    public R Accept<R>( Visitor<R> v ) => v.VisitLogicalExpression( this );
}

sealed class CallExpression : Expression {
    public readonly Expression Callee;
    public readonly List<Expression> Args;
    public readonly Token CloseParen;
    public CallExpression( Expression callee, List<Expression> args, Token closeParen ) { Callee = callee; Args = args; CloseParen = closeParen; }
    public R Accept<R>( Visitor<R> v ) => v.VisitCallExpression( this );
}

sealed class GetExpression : Expression {
    public readonly Expression Object;
    public readonly Token Name;
    public GetExpression( Expression @object, Token name ) { Object = @object; Name = name; }
    public R Accept<R>( Visitor<R> v ) => v.VisitGetExpression( this );
}

sealed class SetExpression : Expression {
    public readonly Expression Object, Value;
    public readonly Token Name;
    public SetExpression( Expression @object, Token name, Expression value ) { Object = @object; Name = name; Value = value; }
    public R Accept<R>( Visitor<R> v ) => v.VisitSetExpression( this );
}

sealed class ThisExpression : Expression {
    public readonly Token Keyword;
    public ThisExpression( Token keyword ) => Keyword = keyword;
    public R Accept<R>( Visitor<R> v ) => v.VisitThisExpression( this );
}

sealed class UnaryExpression : Expression {
    public readonly Token Operator;
    public readonly Expression Right;
    public UnaryExpression( Token op, Expression right ) { Operator = op; Right = right; }
    public R Accept<R>( Visitor<R> v ) => v.VisitUnaryExpression( this );
}

sealed class VariableExpression : Expression {
    public readonly Token Name;
    public VariableExpression( Token name ) => Name = name;
    public R Accept<R>( Visitor<R> v ) => v.VisitVariableExpression( this );
}

sealed class AssignmentExpression : Expression {
    public readonly Token Name;
    public readonly Expression Value;
    public AssignmentExpression( Token name, Expression value ) { Name = name; Value = value; }
    public R Accept<R>( Visitor<R> v ) => v.VisitAssignmentExpression( this );
}

} // namespace
