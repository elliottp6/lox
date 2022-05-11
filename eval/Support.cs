using System; using System.Collections.Generic; using Lox.Scan; namespace Lox.Eval {

sealed class RuntimeError : Exception {
    public readonly Token Token;
    public RuntimeError( Token token, string message ) : base( message ) { Token = token; }
}

delegate object? LoxCallable( object?[] args, Token closeParen );

sealed class Return : Exception {
    public readonly object? Value;
    public Return( object? value ) => Value = value;
}

sealed class LoxClass {
    public readonly string Name;
    public Dictionary<string,LoxCallable?> Methods;
    public LoxClass( string name, Dictionary<string,LoxCallable?> methods ) { Name = name; Methods = methods; }
    public override string ToString() => Name;
}

sealed class LoxInstance {
    readonly LoxClass Class;
    readonly Dictionary<string,object?> Fields = new();

    // constructor
    public LoxInstance( LoxClass c ) => Class = c;

    // method
    public object? Get( Token name ) {
        if( Fields.TryGetValue( (string)name.Value, out var value ) ) return value;
        if( Class.Methods.TryGetValue( (string)name.Value, out var method ) ) return method;
        throw new RuntimeError( name, $"undefined field '{name.Value}'" );
    }

    public object? Set( Token name, object? value ) {
        Fields[(string)name.Value] = value;
        return value;
    }

    // overrides
    public override string ToString() => $"instance of {Class.Name}";
}

} // namespace

