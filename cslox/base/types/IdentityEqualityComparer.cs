using System.Collections.Generic; using System.Runtime.CompilerServices; namespace Lox {

sealed class IdentityEqualityComparer<T> : IEqualityComparer<T> where T : class {
    public int GetHashCode( T value ) => RuntimeHelpers.GetHashCode( value );
    public bool Equals( T? left, T? right ) => left == right;
}

} // namespace
