using System; using Lox.Scan; namespace Lox.Eval {

sealed class RuntimeError : Exception {
    public readonly Token Token;
    public RuntimeError( Token token, string message ) : base( message ) { Token = token; }
}

delegate object? LoxCallable( Interpreter interpreter, object?[] args, Token closeParen );

sealed class Return : Exception {
    public readonly object? Value;
    public Return( object? value ) => Value = value;
}

sealed class LoxClass {
    public readonly string Name;
    public LoxClass( string name ) { Name = name; }
    public override string ToString() => Name;
}

sealed class LoxInstance {
    readonly LoxClass Class;
    public LoxInstance( LoxClass c ) => Class = c;   
    public override string ToString() => $"instance of {Class.Name}";
}

} // namespace

