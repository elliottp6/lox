using System; using System.Text; using System.Collections.Generic; using System.Runtime.InteropServices; using static System.Math; namespace Lox {

static class CollectionExtensions {
	// Array
	public static T[] Add<T>( this T[] a, T[] b ) { var c = new T[a.Length + b.Length]; Array.Copy( a, c, a.Length ); Array.Copy( b, 0, c, a.Length, b.Length ); return c; }
	public static int End<T>( this T[] a ) => a.Length - 1;
	public static void Inmap<T>( this T[] a, Func<T,T> f ) { for( var i = 0; i < a.Length; i++ ) a[i] = f( a[i] ); }
	public static T Last<T>( this T[] a ) => a[a.Length - 1];
	public static string ToStringEach<T>( this ReadOnlySpan<T> a, char sep = ' ' ) { var b = new StringBuilder(); for( var i = 0; i < a.Length; i++ ) { if( i > 0 ) b.Append( sep ); b.Append( a[i] ); } return b.ToString(); }
	//public static T TryGet<T>( this T[] a, int i, T @default = default ) => (i >= 0) & (i < a.Length) ? a[i] : @default;

	// Dictionary
	//public static T TryGetValue<K,T>( this Dictionary<K,T> d, K key, T @default = default ) => d.TryGetValue( key, out var value ) ? value : @default;
	
	// List
	public static ReadOnlySpan<T> Clip<T>( this List<T> s, int start, int end ) => s.Readspan().Slice( start, end - start + 1 );
	public static bool Empty<T>( this List<T> a ) => 0 == a.Count;
	public static int End<T>( this List<T> a ) => a.Count - 1;
	public static string ToStringEach<T>( this List<T> a, char sep = ' ' ) => ToStringEach( a.Readspan(), sep );
	public static bool TryPeek<T>( this List<T> a, out T value ) { if( 0 == a.Count ) { value = default!; return false; } value = a[a.Count - 1]; return true; }
	public static bool Sorted<T>( this List<T> a ) where T : IComparable<T> { for( var i = 1; i < a.Count; i++ ) if( -1 == a[i].CompareTo( a[i - 1] ) ) return false; return true; } // returns true if the list is sorted
	public static Span<T> Span<T>( this List<T> a ) => CollectionsMarshal.AsSpan( a );
	public static ReadOnlySpan<T> Readspan<T>( this List<T> a ) => CollectionsMarshal.AsSpan( a );
	public static ReadOnlySpan<T> Slice<T>( this List<T> a, int start ) => CollectionsMarshal.AsSpan( a ).Slice( start );
	public static T ClampedGet<T>( this List<T> a, int i ) => a[Clamp( i, 0, a.End() )];
	public static bool Contains<T>( this List<T> a, Func<T,bool> match ) { foreach( var x in a ) if( match( x ) ) return true; return false; }
	public static void AddRange<T>( this List<T> a, ReadOnlySpan<T> b ) { foreach( var x in b ) a.Add( x ); }
	public static T GetOrNew<T>( this List<T> a, int i ) where T : new() { if( i < a.Count ) return a[i]; var t = new T(); a.Add( t ); return t; }
	public static void Inmap<T>( this List<T> a, Func<T,T> f ) => a.Span().Inmap( f );
	public static T Last<T>( this List<T> a ) => a[a.Count - 1];
	public static T Peek<T>( this List<T> a ) => a[a.Count - 1];
	public static T Penultimate<T>( this List<T> a ) => a[a.Count - 2];
	public static T Pop<T>( this List<T> a ) => a.PopAt( a.End() );
	public static T PopAt<T>( this List<T> a, int i ) { var value = a[i]; a.RemoveAt( i ); return value; }
	public static T? TryGet<T>( this List<T> a, int i, T? @default = default ) => (i >= 0) & (i < a.Count) ? a[i] : @default;
	public static void Push<T>( this List<T> a, T b ) => a.Add( b );
	public static void RemoveLast<T>( this List<T> a ) => a.RemoveAt( a.End() );
	public static void Set<T>( this List<T> a, T value ) { a.Clear(); a.Add( value ); }
	public static void Set<T>( this List<T> a, List<T> values ) { a.Clear(); a.AddRange( values ); }
	public static void Shrink<T>( this List<T> a, int count ) => a.RemoveRange( count, a.Count - count );
	
	// ReadOnlySpan
	public static ReadOnlySpan<T> Clip<T>( this ReadOnlySpan<T> s, int start, int end ) => s.Slice( start, end - start + 1 );
	public static bool Contains<T>( this ReadOnlySpan<T> a, Func<T,bool> match ) { for( var i = 0; i < a.Length; i++ ) if( match( a[i] ) ) return true; return false; }
	public static int End<T>( this ReadOnlySpan<T> a ) => a.Length - 1;
	//public static T Find<T>( this ReadOnlySpan<T> s, Func<T,bool> match ) { foreach( var t in s ) if( match( t ) ) return t; return default; }
	public static T Last<T>( this ReadOnlySpan<T> a ) => a[a.Length - 1];
	public static bool Sorted<T>( this ReadOnlySpan<T> a ) where T : IComparable<T> { for( var i = 1; i < a.Length; i++ ) if( -1 == a[i].CompareTo( a[i - 1] ) ) return false; return true; }
	public static T? TryGet<T>( this ReadOnlySpan<T> a, int i, T? @default = default ) => (i >= 0) & (i < a.Length) ? a[i] : @default;
	public static ReadOnlySpan<T> RemoveAfter<T>( this ReadOnlySpan<T> a, T item, int start = 0 ) where T : IEquatable<T> => a.Slice( 0, start + a.Slice( start ).IndexOf( item ) + 1 );
	public static ReadOnlySpan<T> RemoveOnward<T>( this ReadOnlySpan<T> a, Func<T,bool> match, int start = 0 ) { for( var i = start; i < a.Length; i++ ) if( match( a[i] ) ) return a.Slice( 0, i ); return a; }
	public static ReadOnlySpan<T> KeepWhile<T>( this ReadOnlySpan<T> a, Func<T,bool> match, int start = 0 ) { for( var i = start; i < a.Length; i++ ) if( !match( a[i] ) ) return a.Slice( 0, i ); return a; }
	public static ReadOnlySpan<T> KeepWhile<T>( this ReadOnlySpan<T> a, Func<T,T?,bool> match, int start = 0 ) { for( var i = start; i < a.Length; i++ ) if( !match( a[i], a.TryGet( i + 1 ) ) ) return a.Slice( 0, i ); return a; }

	// Span
	public static void Inmap<T>( this Span<T> a, Func<T,T> f ) { for( var i = 0; i < a.Length; i++ ) a[i] = f( a[i] ); }
	public static int End<T>( this Span<T> a ) => a.Length - 1;

	// StringBuilder
	public static int End( this StringBuilder a ) => a.Length - 1;
}

} // namespace
