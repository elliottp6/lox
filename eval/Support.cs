using System; using System.Collections.Generic; using Lox.Scan; namespace Lox.Eval {

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
    readonly Dictionary<string,object?> Fields = new();

    // constructor
    public LoxInstance( LoxClass c ) => Class = c;

    // method
    public object? Get( Token name ) {
        if( !Fields.TryGetValue( (string)name.Value, out var value ) )
            throw new RuntimeError( name, $"Undefined property '{name.Value}'" );
        return value;
    }

    public object? Set( Token name, object? value ) {
        Fields[(string)name.Value] = value;
        return value;
    }

    // overrides
    public override string ToString() => $"instance of {Class.Name}";
}

} // namespace

