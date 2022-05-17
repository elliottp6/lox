using System; using System.Collections.Generic; using Lox.Scan; namespace Lox.Eval {

sealed class RuntimeError : Exception {
    public readonly Token Token;
    public RuntimeError( Token token, string message ) : base( message ) { Token = token; }
}

delegate object? LoxUnboundFunction( object?[] args, Token closeParen, Environment environment );
delegate object? LoxFunction( object?[] args, Token closeParen );
delegate LoxFunction LoxMethod( LoxInstance instance );

sealed class LoxReturn : Exception {
    public readonly object? Value;
    public LoxReturn( object? value ) => Value = value;
}

sealed class LoxClass {
    public readonly string Name;
    public readonly LoxClass? Super;
    public Dictionary<string,LoxMethod> Methods;

    // constructor
    public LoxClass( string name, LoxClass? super, Dictionary<string,LoxMethod> methods ) { Name = name; Super = super; Methods = methods; }

    // methods
    public bool TryGetMethod( string name, out LoxMethod? method, bool checkSuper = true ) {
        if( Methods.TryGetValue( name, out method ) ) return true;
        if( checkSuper && null != Super && Super.TryGetMethod( name, out method ) ) return true;
        return false;
    }

    public override string ToString() => Name;
}

sealed class LoxInstance {
    readonly LoxClass Class;
    readonly Dictionary<string,object?> Fields = new();

    // constructor
    public LoxInstance( LoxClass c ) => Class = c;

    // method
    public object? Get( Token token ) {
        var name = (string)token.Value;
        if( Fields.TryGetValue( name, out var value ) ) return value;
        if( Class.TryGetMethod( name, out var method ) ) return method!( this ); // bind method to this to create a functional closure over 'this'
        throw new RuntimeError( token, $"undefined field '{name}'" );
    }

    public object? Set( Token name, object? value ) {
        Fields[(string)name.Value] = value;
        return value;
    }

    // overrides
    public override string ToString() => $"instance of {Class.Name}";
}

} // namespace

