using System; using System.Collections.Generic; using NUnit.Framework; using static Lox.Scan.TokenType; namespace Lox.Scan {

// scanner (lexical grammar, ideally regular language which can be parsed via RegEx):
//      characters => lexemes => tokens
sealed class Scanner {
    readonly Dictionary<string,TokenType> keywords_ = new() {
        { "and", AND },
        { "class", CLASS },
        { "else", ELSE },
        { "false", FALSE },
        { "for", FOR },
        { "fun", FUN },
        { "if", IF },
        { "nil", NIL },
        { "or", OR },
        { "return", RETURN },
        { "super", SUPER },
        { "this", THIS },
        { "true", TRUE },
        { "var", VAR },
        { "while", WHILE },
    };

    // methods
    public List<Token> ScanTokens( params string[] source ) {
        // allocate token list
        List<Token> tokens = new();

        // for each line, for each token => scan!
        Token token;
        for( var i = 0; i < source.Length; i++ )
            for( var start = 0; start < source[i].Length; start+= token.Length.Max( 1 ) )
                if( (token = ScanToken( line: i + 1, start, source[i].AsSpan( start ) )).IsSyntax )
                    tokens.Add( ScanToken( line: i + 1, start, source[i].AsSpan( start ) ) );
                else if( token.IsUnexpectedCharacter )
                    ColorConsole.Error( line: i + 1, $"unexpected character: {token.Value}" );
        return tokens;
    }

    Token ScanToken( int line, int start, ReadOnlySpan<char> slice ) => slice[0] switch {
        // 1 character tokens
        '(' => new( LEFT_PAREN, "(", line, start, 1 ),
        ')' => new( RIGHT_PAREN, ")", line, start, 1 ),
        '{' => new( LEFT_BRACE, "{", line, start, 1 ),
        '}' => new( RIGHT_BRACE, "}", line, start, 1 ),
        ',' => new( COMMA, ",", line, start, 1 ),
        '.' => new( DOT, ".", line, start, 1 ),
        '-' => new( MINUS, "-", line, start, 1 ),
        '+' => new( PLUS, "+", line, start, 1 ),
        ';' => new( SEMICOLON, ";", line, start, 1 ),
        '*' => new( STAR, "*", line, start, 1 ),

        // 1 or 2 character tokens
        '=' when '=' == slice.TryGet( 1 ) => new( EQUAL_EQUAL, "==", line, start, 2 ),
        '=' => new( EQUAL, "=", line, start, 1 ),
        '!' when '=' == slice.TryGet( 1 ) => new( BANG_EQUAL, "!=", line, start, 2 ),
        '!' => new( BANG, "!", line, start, 1 ),    
        '<' when '=' == slice.TryGet( 1 ) => new( LESS_EQUAL, "<=", line, start, 2 ),
        '<' => new( LESS, "<", line, start, 1 ),
        '>' when '=' == slice.TryGet( 1 ) => new( GREATER_EQUAL, ">=", line, start, 2 ),
        '>' => new( GREATER, ">", line, start, 1 ),

        // comment or slash
        '/' when '/' == slice.TryGet( 1 ) => new( COMMENT, slice.ToString(), line, start, slice.Length ),
        '/' => new( SLASH, "/", line, start, 1 ),

        // string literal
        '"' => (slice = slice.Slice( 1 )).Contains( '"' ) ?
                new( STRING, (slice = slice.RemoveOnward( c => '"' == c )).ToString(), line, start, slice.Length + 2 ) :
                new( UNEXPECTED_CHARACTER, slice.ToString(), line, start, slice.Length + 1 ),

        // number literal
        _ when slice[0].IsDigit() => new( NUMBER,
            value: (slice = slice.KeepWhile( (c0, c1) => c0.IsDigit() | (('.' == c0) & c1.IsDigit()) )).ToDouble(),
            line, start, slice.Length ),

        // keyword or identifier
        _ when slice[0].IsAlphanumericOrUnderscore() =>
            new(
                keywords_.ContainsKey( (slice = slice.KeepWhile( c => c.IsAlphanumericOrUnderscore() | c.IsDigit() ) ).ToString() ) ? keywords_[slice.ToString()] : IDENTIFIER,
                slice.ToString(), line, start, slice.Length ),

        // whitespace
        _ when slice[0].IsWhitespace() => new( WHITESPACE, "", start, 1, (slice = slice.KeepWhile( c => c.IsWhitespace() ) ).Length ),

        // unexpected character
        _ => new( UNEXPECTED_CHARACTER, slice[0].ToString(), line, start, 1 )
    };

    // tests
    [Test] public static void Test() => Assert.AreEqual(
        new List<Token>() {
            new( VAR, "var", 0, 0, 3 ),
            new( IDENTIFIER, "x", 0, 4, 1 ),
            new( EQUAL, "=", 0, 6, 1 ),
            new( NUMBER, 5.0, 0, 8, 1 ),
            new( SEMICOLON, ";", 0, 9, 1 ),
            new( IF, "if", 1, 0, 2 ),
            new( LEFT_PAREN, "(", 1, 2, 1 ),
            new( IDENTIFIER, "x", 1, 4, 1 ),
            new( SLASH, "/", 1, 6, 1 ),
            new( NUMBER, 2.0, 1, 8, 1 ),
            new( GREATER_EQUAL, ">=", 1, 10, 2 ),
            new( NUMBER, 0.0, 1, 13, 1 ),
            new( RIGHT_PAREN, ")", 1, 15, 1 ),
            new( STRING, "hello world", 1, 17, 13 ),
            new( SEMICOLON, ";", 1, 30, 1 )},
        new Scanner().ScanTokens(
            "var x = 5; // initialize x to 5",
            "if( x / 2 >= 0 ) \"hello world\";" ) );
}

} // namespace
