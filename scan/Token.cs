using static Lox.Scan.TokenType; namespace Lox.Scan {

enum TokenType : int {
    // ignorable tokens
    WHITESPACE = 0, // note that this is the default value
    COMMENT = 1, // this is also ignored
    UNEXPECTED_CHARACTER = 2, // these are warned against, but then ignored
    
    // 1 character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // 1 or 2 character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // literals
    NUMBER, STRING, IDENTIFIER,

    // keywords
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    RETURN, SUPER, THIS, TRUE, VAR, WHILE,
}

readonly struct Token {
    public readonly TokenType Type;
    public readonly object Value;
    public readonly int Line, Start, Length;
 
    // constructor
    public Token( TokenType type, object value, int line, int start, int length ) {
        Type = type; Value = value; Line = line; Start = start; Length = length;
    }

    // properties
    public bool IsSyntax => Type > UNEXPECTED_CHARACTER;
    public bool IsUnexpectedCharacter => UNEXPECTED_CHARACTER == Type;
    
    // overrides
    public override string ToString() => $"{Type}, {Value}, {Line}, {Start}, {Length}";
}

} // namespace
