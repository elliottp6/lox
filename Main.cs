using System; using System.IO; using Lox.Scan; using Lox.Syntax; using Lox.Parse; using Lox.Eval; namespace Lox {

static class MainClass {
    static void Main( params string[] args ) {
        var command = args.Length > 0 ? args[0] : "debug";
        switch( command ) {
            // this is for putting whatever code you want for debugging purposes
            case "debug": {
                // put code here
                Main( "eval", "scripts/7_class.lox" );
                return;
            }

            // runs all tests in the system
            case "test": { Tester.RunTests(); return; }

            // runs a specific file
            case "eval": {
                // check that we have a 'path' argument
                if( args.Length < 2 ) {
                    Console.WriteLine( "which file to eval?" );
                    return;
                }

                // try to read file
                var path = args[1];
                string[] source;
                try {
                    source = File.ReadAllLines( path );
                } catch( Exception e ) {
                    Console.WriteLine( e.Message );
                    return;
                }

                // execute
                Eval( source );
                return;
            }

            // runs an interactive read-eval-print-loop
            case "prompt": {
                Console.WriteLine( "Press the Escape (Esc) key to quit:" );
                for(;;) {
                    Console.Write( "> " );
                    var line = Console.ReadLine();
                    if( null == line ) return;
                    //if( !Console2.CancelableReadLine( buffer ) ) return;
                    Eval( line );
                }
            }

            // unrecognized command
            default: {           
                Console.WriteLine( $"unrecognized command \"{command}\" (expected: debug, test, eval, prompt)" );
                return;
            }
        }
    }

    public static void Eval( params string[] source ) {
        // scan (lexical analysis)
        ColorConsole.WriteLine( "--scan--", ConsoleColor.DarkBlue );
        Scanner scanner = new();
        var tokens = scanner.ScanTokens( source );
        if( tokens.Count > 0 ) ColorConsole.WriteLine( tokens.ToStringEach( '\n' ), ConsoleColor.DarkBlue );

        // parse (syntactic analysis)
        ColorConsole.WriteLine( "--parse--", ConsoleColor.Magenta );
        Parser parser = new( tokens );
        var statements = parser.Parse( catchParseErrors: false );
        new Printer().Print( statements );
        if( parser.Errors > 0 ) return;

        // resolve (semantic analysis)
        Interpreter interpreter = new();
        Resolver resolver = new( interpreter );
        resolver.Resolve( statements );

        // evaluate
        ColorConsole.WriteLine( "--eval--", ConsoleColor.DarkYellow );
        interpreter.Interpret( statements, catchRuntimeExceptions: false );
    }
}

} // namespace
