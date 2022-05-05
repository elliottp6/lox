using System; using NUnit.Framework; namespace Lox {

static class NumberExtensions {
    // int
	public static int Clamp( this int n, int a, int b ) => Math.Clamp( n, a, b );
	public static bool Even( this int n ) => 0 == (n & 1);
	public static int Max( this int a, int b ) => Math.Max( a, b );
	public static int Min( this int a, int b ) => Math.Min( a, b );
	public static bool Odd( this int n ) => 1 == (n & 1);
	public static int Sign( this int n ) => Math.Sign( n );

    // long
	public static long Round( this long n, long interval ) => (n + (interval >> 1) * (1 | (n >> 63))) / interval * interval; // note that (1 | (n >> 63)) gives you -1 if negative, otherwise +1

	// double
	#pragma warning disable 1718
	public static double Abs( this double x ) => Math.Abs( x );
	public static double Ceil( this double x ) => Math.Ceiling( x );
	public static double Clamp( this double x, double a, double b ) => Math.Clamp( x, a, b );
	public static int CompareTo( this double a_primary, double b_primary, double a_secondary, double b_secondary ) { var c = a_primary.CompareTo( b_primary ); return 0 != c ? c : a_secondary.CompareTo( b_secondary ); }
	public static double Exp( this double x ) => Math.Exp( x );
	public static bool IsInfinity( this double x ) => double.IsInfinity( x );
	public static double Log( this double x ) => Math.Log( x );
	public static double Log2( this double x ) => Math.Log2( x );
	public static double Max( this double x, double y ) => Math.Max( x, y );
	public static double Max( this double x, double y, double z) => Math.Max( x, Math.Max( y, z ) );
	public static double Max( this double x, double y, double z, double w ) => Math.Max( x, Math.Max( y, Math.Max( z, w ) ) );
	public static double Min( this double x, double y ) => Math.Min( x, y );
	public static double Min( this double x, double y, double z ) => Math.Min( x, Math.Min( y, z ) );
	public static double Min( this double x, double y, double z, double w ) => Math.Min( x, Math.Min( y, Math.Min( z, w ) ) );
	public static bool Nan( this double x ) => x != x;
	public static bool Num( this double x ) => x == x;
	public static bool NegInf( this double x ) => double.IsNegativeInfinity( x );
	public static bool PosInf( this double x ) => double.IsPositiveInfinity( x );
	public static bool PosReal( this double x ) => (x > 0) & (x < double.PositiveInfinity);
	public static bool Real( this double x ) => 0 == 0 * x;
	public static double Round( this double x, int decimals = 0 ) => Math.Round( x, decimals );
	public static double Sqrt( this double x ) => Math.Sqrt( x );
	public static bool Within( this double x, double a, double b ) => (x >= a) & (a <= b);
	#pragma warning restore 1718
}

} // namespace
