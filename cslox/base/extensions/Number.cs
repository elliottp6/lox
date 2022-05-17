using System; namespace Lox {

static class NumberExtensions {
    // int
	public static int Max( this int a, int b ) => Math.Max( a, b );
	public static int Min( this int a, int b ) => Math.Min( a, b );

	// double
	public static double Round( this double x, int decimals = 0 ) => Math.Round( x, decimals );
}

} // namespace
