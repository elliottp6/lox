using System; using System.Collections.Generic; using System.Reflection; using NUnit.Framework; namespace Lox {

static class Tester {
    // runs all the NUnit tests in this assembly
    public static void RunTests() {
        foreach( var type in Assembly.GetExecutingAssembly().GetTypes() ) {
            // get tests for this type
            var tests = type.GetTests();
            if( 0 == tests.Count ) continue;

            // print the name of this type
            Console.ForegroundColor = ConsoleColor.Blue;
            Console.Write( type.Name );

            // run each test
            foreach( var test in tests ) {
                // print name of test
                Console.ForegroundColor = ConsoleColor.Green;
                Console.Write( " " + test.Name );

                // invoke the test
                Console.ResetColor();
                test.Invoke( null, null );
            }

            // write a newline after the last test for this type
            Console.WriteLine();
        }
    }

    // private helper to gets a list of all tests for a given type
    static List<MethodInfo> GetTests( this Type t ) {
        var tests = new List<MethodInfo>();
        foreach( var method in t.GetMethods( BindingFlags.Public | BindingFlags.Static ) )
            if( null != method.GetCustomAttribute<TestAttribute>() )
                tests.Add( method );
        return tests;
    }
}

} // namespace
