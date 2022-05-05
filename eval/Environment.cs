using System.Collections.Generic; using Lox.Scan; namespace Lox.Eval {

sealed class Environment {
    readonly Dictionary<string,object?> identifiers_ = new();
    readonly Environment? parent_;

    // constructor
    public Environment( Environment? parent ) => parent_ = parent;

    // methods
    public Environment Global() => null == parent_ ? this : parent_.Global();

    public List<string> List() {
        var results = new List<string>( identifiers_.Count );
        foreach( var keyValue in identifiers_ ) results.Add( keyValue.Key );
        return results;
    }

    public void Define( Token name, object? value ) {
        if( !identifiers_.TryAdd( (string)name.Value, value ) ) throw new RuntimeError( name, $"Variable already defined '{name.Value}'" );
    }

    public object? Get( Token name ) {
        if( !identifiers_.TryGetValue( (string)name.Value, out var value ) ) {
            if( null == parent_ ) throw new RuntimeError( name, $"Undefined variable '{name.Value}'" );
            return parent_.Get( name );
        }
        return value;
    }

    public void Set( Token name, object? value ) {
        if( !identifiers_.ContainsKey( (string)name.Value ) ) {
            if( null == parent_ ) throw new RuntimeError( name, $"Undefined varaible '{name.Value}'" );
            parent_.Set( name, value );
        }
        identifiers_[(string)name.Value] = value;
    }
}

} // namespace
