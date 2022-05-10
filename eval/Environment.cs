using System; using System.Collections.Generic; using Lox.Scan; namespace Lox.Eval {

sealed class Environment {
    readonly Dictionary<string,object?> identifiers_ = new();
    readonly Environment? parent_;

    // constructor
    public Environment( Environment? parent ) => parent_ = parent;

    // methods
    public Environment Global() => null == parent_ ? this : parent_.Global();

    public List<string> List() {
        List<string> results = new( identifiers_.Count );
        foreach( var keyValue in identifiers_ ) results.Add( keyValue.Key );
        return results;
    }

    public void Define( Token name, object? value ) {
        if( !identifiers_.TryAdd( (string)name.Value, value ) )
            throw new RuntimeError( name, $"Variable already defined '{name.Value}'" );
    }

    public object? Get( Token name, int scope ) {
        // TODO: remove: Console.WriteLine( $"{name} looking from scope {scope}" );

        // if not in the right scope, defer to parent
        if( scope > 0 ) {
            if( null == parent_ ) throw new RuntimeError( name, $"variable scope exceeds global -- error in Resolver" );
            return parent_?.Get( name, scope - 1 );
        }

        // otherwise, it should be in this scope
        if( !identifiers_.TryGetValue( (string)name.Value, out var value ) ) throw new RuntimeError( name, $"Undefined variable '{name.Value}'" );
        return value;
    }

    public void Set( Token name, object? value, int scope ) {
        // if not in right scope, defer to parent
        if( scope > 0 ) {
            if( null == parent_ ) throw new RuntimeError( name, $"variable scope exceeds global -- error in Resolver" );
            parent_?.Set( name, value, scope - 1 );
            return;
        }

        // otherwise, it should be in this scope
        if( !identifiers_.ContainsKey( (string)name.Value ) ) throw new RuntimeError( name, $"Undefined varaible '{name.Value}'" );
        identifiers_[(string)name.Value] = value;
    }
}

} // namespace
