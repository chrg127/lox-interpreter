package lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import static lox.Token.Type.*;

class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();
    private int start = 0, curr = 0, line = 1;
    private static final Map<String, Token.Type> keywords;

    static {
        keywords = new HashMap<>();
        keywords.put("and",    AND);
        keywords.put("class",  CLASS);
        keywords.put("else",   ELSE);
        keywords.put("false",  FALSE);
        keywords.put("and",    AND);
        keywords.put("class",  CLASS);
        keywords.put("else",   ELSE);
        keywords.put("false",  FALSE);
        keywords.put("fun",    FUN);
        keywords.put("for",    FOR);
        keywords.put("if",     IF);
        keywords.put("nil",    NIL);
        keywords.put("or",     OR);
        keywords.put("print",  PRINT);
        keywords.put("return", RETURN);
        keywords.put("super",  SUPER);
        keywords.put("this",   THIS);
        keywords.put("true",   TRUE);
        keywords.put("var",    VAR);
        keywords.put("while",  WHILE);
    }

    Scanner(String source) {
        this.source = source;
    }

    public List<Token> scanTokens() {
        while (!isAtEnd()) {
            start = curr;
            scanToken();
        }
        tokens.add(new Token(EOF, "", null, line));
        return tokens;
    }

    private boolean isAtEnd() {
        return curr >= source.length();
    }

    private void scanToken() {
        char c = advance();
        switch (c) {
        case '(': addToken(LEFT_PAREN);     break;
        case ')': addToken(RIGHT_PAREN);    break;
        case '{': addToken(LEFT_BRACE);     break;
        case '}': addToken(RIGHT_BRACE);    break;
        case ',': addToken(COMMA);          break;
        case '.': addToken(DOT);            break;
        case '-': addToken(MINUS);          break;
        case '+': addToken(PLUS);           break;
        case ';': addToken(SEMICOLON);      break;
        case '*': addToken(STAR);           break;
        case '&': addToken(BITAND);         break;
        case '|': addToken(BITOR);          break;
        //case '?': addToken(QMARK);          break;
        //case ':': addToken(DCOLON);         break;

        case '!': addToken(match('=') ? BANG_EQ : BANG);        break;
        case '=': addToken(match('=') ? EQ_EQ : EQ);            break;
        case '<': addToken(match('=') ? LESS_EQ : LESS);        break;
        case '>': addToken(match('=') ? GREATER_EQ : GREATER);  break;

        case '/':
            if (match('/')) {
                while (peek() != '\n' && !isAtEnd())
                    advance();
            } else {
                addToken(SLASH);
            }
            break;

        // whitespace: ignore
        case ' ': case '\r': case '\t':
            break;

        case '\n':
            line++;
            break;

        case '"':
            string();
            break;

        default:
            if (isDigit(c))
                number();
            else if (isAlpha(c))
                ident();
            else
                Lox.error(line, "unexpected character");
            break;
        }
    }

    private char advance() {
        return source.charAt(curr++);
    }

    private void addToken(Token.Type type) {
        addToken(type, null);
    }

    private void addToken(Token.Type type, Object literal) {
        String text = source.substring(start, curr);
        tokens.add(new Token(type, text, literal, line));
    }

    private char peek() {
        if (isAtEnd())
            return '\0';
        return source.charAt(curr);
    }

    private char peekNext() {
        if (curr + 1 >= source.length())
            return '\0';
        return source.charAt(curr + 1);
    }

    private boolean match(char expected) {
        if (isAtEnd())
            return false;
        if (source.charAt(curr) != expected)
            return false;
        curr++;
        return true;
    }

    private void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n')
                line++;
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "unterminated string");
            return;
        }

        advance();
        String value = source.substring(start + 1, curr - 1);
        addToken(STRING, value);
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }

    private void number() {
        while (isDigit(peek()))
            advance();
        // fractional part
        if (peek() == '.' && isDigit(peekNext())) {
            advance();
            while (isDigit(peek()))
                advance();
        }
        addToken(NUMBER, Double.parseDouble(source.substring(start, curr)));
    }

    private void ident() {
        while (isAlphaNumeric(peek()))
            advance();
        String text = source.substring(start, curr);
        Token.Type type = keywords.get(text);
        if (type == null)
            type = IDENT;
        addToken(type);
    }
}
