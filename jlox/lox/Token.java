package lox;

public class Token {
    public enum Type {
        // single character
        LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
        COMMA, DOT, PLUS, MINUS, SEMICOLON, SLASH, STAR,
        BITAND, BITOR,
        QMARK, DCOLON,

        // one or two characters
        BANG, BANG_EQ, EQ, EQ_EQ, GREATER, GREATER_EQ, LESS, LESS_EQ,

        // literals
        IDENT, STRING, NUMBER,

        // keywords
        AND, BREAK, CLASS, ELSE, FALSE, FUN, FOR, IF, LAMBDA,
        NIL, OR, PRINT, RETURN, SUPER, THIS, TRUE, STATIC, VAR, WHILE,

        EOF
    }

    final Type type;
    final String lexeme;
    final Object literal;
    final int line;

    Token(Type type, String lexeme, Object literal, int line) {
        this.type = type;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line;
    }

    public String toString() {
        return type + " " + lexeme + (literal == null ? "" : " " + literal);
    }
}
