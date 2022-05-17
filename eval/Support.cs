using System; using System.Collections.Generic; using Lox.Scan; namespace Lox.Eval {

sealed class RuntimeError : Exception {
    public readonly Token Token;
    public RuntimeError( Token token, string message ) : base( message ) { Token = token; }
}

delegate object? LoxUnboundFunction( object?[] args, Token closeParen, Environment environment );
delegate object? LoxFunction( object?[] args, Token closeParen );
delegate LoxFunction LoxMethod( LoxInstance instance );

sealed class Return : Exception {
    public readonly object? Value;
    public Return( object? value ) => Value = value;
}

sealed class LoxClass {
    public readonly string Name;
    public readonly LoxClass? Super;
    public Dictionary<string,LoxMethod> Methods;
    public LoxClass( string name, LoxClass? super, Dictionary<string,LoxMethod> methods ) { Name = name; Super = super; Methods = methods; }
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
        if( Class.Methods.TryGetValue( (string)name.Value, out var method ) ) return method( this );
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

