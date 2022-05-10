using System; using System.Text; namespace Lox {

static class ColorConsole {
    public static void Error( int line, string message ) =>
        WriteLine( $"[{line}] {message}", ConsoleColor.Red );

    public static void WriteLine( string s, ConsoleColor foregroundColor ) {
        var backup = Console.ForegroundColor;
        Console.ForegroundColor = foregroundColor;
        Console.WriteLine( s );
        Console.ForegroundColor = backup;
    }

    // TODO: finish this! https://stackoverflow.com/questions/31996519/listen-on-esc-while-reading-console-line
    public static bool CancelableReadLine( StringBuilder buffer ) {
        var offset = Console.CursorLeft;
        for( buffer.Clear();; ) {
            // read key
            var keyInfo = Console.ReadKey( intercept: true );
            switch( keyInfo.Key ) {
                // escape terminates w/ 'false'
                case ConsoleKey.Escape: Console.WriteLine(); return false;

                // enter terminate w/ 'true'
                case ConsoleKey.Enter: Console.WriteLine(); return true;

                // left-arrow moves cursor left (if we can)
                case ConsoleKey.LeftArrow: if( Console.CursorLeft - offset > 0 ) Console.CursorLeft--; break;

                // right-arrow moves cursor right (if we can)
                case ConsoleKey.RightArrow: if( Console.CursorLeft - offset < buffer.Length ) Console.CursorLeft++; break;

                // backspace removes character prior to cursor
                case ConsoleKey.Backspace:
                    if( 0 == Console.CursorLeft ) break;
                    var i = --Console.CursorLeft - offset;
                    buffer.Remove( i, 1 );
                    //Console.Write( ' ', )
                    break;
            }

            // add to buffer
            buffer.Append( keyInfo.KeyChar );
            Console.Write( keyInfo.KeyChar );
        }
    }
}

} // namespace
