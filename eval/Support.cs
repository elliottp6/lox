using System; using Lox.Scan; namespace Lox.Eval {

delegate object? Callable( Interpreter interpreter, object?[] args, Token closeParen );

sealed class RuntimeError : Exception {
    public readonly Token Token;
    public RuntimeError( Token token, string message ) : base( message ) { Token = token; }
}

sealed class Return : Exception {
    public readonly object? Value;
    public Return( object? value ) => Value = value;
}

} // namespace

