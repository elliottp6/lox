using System; using System.Text; using NUnit.Framework; using static System.Math; namespace Lox {

static class StringExtensions {
	// ReadOnlySpan<char>
	public static ReadOnlySpan<char> Between( this ReadOnlySpan<char> s, char start, char end ) { int i = s.IndexOf( start ), j = s.IndexOf( end ); return (i >= 0) & (j > i) ? s.Clip( i + 1, j - 1 ) : default; }
	public static ReadOnlySpan<char> BetweenLast( this ReadOnlySpan<char> s, char start, char end ) { int i = s.LastIndexOf( start ), j = s.LastIndexOf( end ); return (i >= 0) & (j > i) ? s.Clip( i + 1, j - 1 ) : default; }
	public static int IndexOf( this ReadOnlySpan<char> s, char c, int @default ) { var i = s.IndexOf( c ); return i >= 0 ? i : @default; }
	public static int IndexOf( this ReadOnlySpan<char> s, ReadOnlySpan<char> c, int @default ) { var i = s.IndexOf( c ); return i >= 0 ? i : @default; }
	public static int LastIndexOf( this ReadOnlySpan<char> s, char c, int @default ) { var i = s.LastIndexOf( c ); return i >= 0 ? i : @default; }
	public static int LastIndexOf( this ReadOnlySpan<char> s, ReadOnlySpan<char> c, int @default ) { var i = s.LastIndexOf( c ); return i >= 0 ? i : @default; }
	public static ReadOnlySpan<char> InSplit( ref this ReadOnlySpan<char> s, char c ) { var i = s.IndexOf( c, s.Length ); var left = s.Slice( 0, i ); s = s.Slice( Min( i + 1, s.Length ) ); return left; }
	public static ReadOnlySpan<char> InSplit( ref this ReadOnlySpan<char> s, ReadOnlySpan<char> c ) { var i = s.IndexOf( c ); if( i < 0 ) { c = s; s = default; return c; } i+=c.End(); var left = s.Slice( 0, i ); s = s.Slice( Min( i + 1, s.Length ) ); return left; }
	public static ReadOnlySpan<char> LeftOf( this ReadOnlySpan<char> s, char c ) => s.Slice( 0, s.IndexOf( c, s.Length ) );
	public static ReadOnlySpan<char> RightOf( this ReadOnlySpan<char> s, char c ) => s.Slice( s.IndexOf( c, -1 ) + 1 );
	public static ReadOnlySpan<char> LeftOfLast( this ReadOnlySpan<char> s, char c ) => s.Slice( 0, s.LastIndexOf( c, s.Length ) );
	public static ReadOnlySpan<char> RightOfLast( this ReadOnlySpan<char> s, char c ) => s.Slice( s.LastIndexOf( c, -1 ) + 1 );
	public static double TryDouble( this ReadOnlySpan<char> s, double @default = double.NaN ) => double.TryParse( s, out var r ) ? r : @default;

	// char
    public static bool IsAlphanumericOrUnderscore( this char c ) => ((c >= 'a') & (c <= 'z')) | ((c >= 'A') & (c <= 'Z')) | (c == '_') | char.IsDigit( c );
	public static bool IsDigit( this char c ) => char.IsDigit( c );
	public static bool IsWhitespace( this char c ) => char.IsWhiteSpace( c );

	// string
	public static (string First, string Second) Bisect( this string s, char by ) { var r = s.Split( new char[] { by }, 2 ); return (r[0], r[1]); }
	public static bool ContainsRemove( this string contains, ref string s ) { var result = s.Contains( contains ); if( result ) s = s.Replace( contains, "" ); return result; }
	public static double ToDouble( this ReadOnlySpan<char> s ) => double.Parse( s );
	public static string Remove( this string s, string r ) => s.Replace( r, "" );
	public static string Remove( this string s, params string[] rs ) { foreach( var r in rs ) s = s.Remove( r ); return s; }
	public static string RemoveEnd( this string s, string end ) => s.ReplaceEnd( end, "" );
	public static string RemoveStart( this string s, string start ) => s.ReplaceStart( start, "" );
	public static string Repeat( this string s, int n ) => new StringBuilder( s.Length * n ).AppendJoin( s, new string[n + 1] ).ToString();
	public static string ReplaceEnd( this string s, string end, string replace ) => s.EndsWith( end ) ? s.Substring( 0, s.LastIndexOf( end ) ) + replace : s;
	public static string ReplaceStart( this string s, string start, string replace ) => s.StartsWith( start ) ? s.Substring( s.IndexOf( start ) + start.Length ) : s;
	public static string RightOf( this string s, char c ) => s.Substring( s.IndexOf( c ) + 1 );
	public static string LeftOf( this string s, char c ) { var i = s.IndexOf( c ); return i > 0 ? s.Substring( 0, i ) : s; }

	// InSplit = in place split, where returned value is left-side of split, and caller is mutated to contain right-side of split
	[Test] public static void InSplit() {
		var s = "hi,there,105".AsSpan();
		Assert.AreEqual( "hi", s.InSplit( ',' ).ToString() );
		Assert.AreEqual( "there", s.InSplit( ',' ).ToString() );
		Assert.AreEqual( "105", s.InSplit( ',' ).ToString() );
		Assert.AreEqual( 0, s.Length );
	}
}

} // namespace
