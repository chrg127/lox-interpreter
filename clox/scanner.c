#include "scanner.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

struct {
    const char *start;
    const char *curr;
    size_t line;
} scanner;

static bool at_end()
{
    return *scanner.curr == '\0';
}

static char advance()
{
    scanner.curr++;
    return scanner.curr[-1];
}

static char peek()
{
    return *scanner.curr;
}

static char peek_next()
{
    return at_end() ? '\0' : scanner.curr[1];
}

static bool match(char expected)
{
    if (at_end())
        return false;
    if (*scanner.curr != expected)
        return false;
    scanner.curr++;
    return true;
}
static Token make_token(TokenType type)
{
    Token token = {
        .type  = type,
        .start = scanner.start,
        .len   = scanner.curr - scanner.start,
        .line  = scanner.line,
    };
    return token;
}

static Token error_token(const char *msg)
{
    Token token = {
        .type  = TOKEN_ERROR,
        .start = msg,
        .len   = strlen(msg),
        .line  = scanner.line,
    };
    return token;
}

static void skip_whitespace()
{
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ': case '\r': case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peek_next() == '/') {
                while (peek() != '\n' && !at_end())
                    advance();
            } else if (peek_next() == '*') {
                while (!(peek() == '*' && peek_next() == '/') && !at_end()) {
                    if (peek() == '\n')
                        scanner.line++;
                    advance();
                }
                // get rid of the '*/'
                advance();
                advance();
            } else
                return;
            break;
        default:
            return;
        }
    }
}

static Token string()
{
    while (peek() != '"' && !at_end()) {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }
    if (at_end())
        return error_token("unterminated string");
    advance();
    return make_token(TOKEN_STRING);
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

static Token number()
{
    while (is_digit(peek()))
        advance();
    if (peek() == '.' && is_digit(peek_next())) {
        advance();
        while (is_digit(peek()))
            advance();
    }
    return make_token(TOKEN_NUMBER);
}

static TokenType check_keyword(int start, int len, const char *rest,
    TokenType type)
{
    if (scanner.curr - scanner.start == start + len
     && memcmp(scanner.start + start, rest, len) == 0)
        return type;
    return TOKEN_IDENT;
}

static TokenType ident_type()
{
    switch (scanner.start[0]) {
    case 'a': return check_keyword(1, 2, "nd",   TOKEN_AND);
    case 'b': return check_keyword(1, 4, "reak", TOKEN_BREAK);
    case 'c':
        if (scanner.curr - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'o':
                if (scanner.curr - scanner.start > 3 && scanner.start[2] == 'n') {
                    switch (scanner.start[3]) {
                    case 't': return check_keyword(4, 4, "inue", TOKEN_CONTINUE);
                    case 's': return check_keyword(4, 1, "t", TOKEN_CONST);
                    }
                }
                break;
            case 'l': return check_keyword(2, 3, "ass", TOKEN_CLASS);
            case 'a': return check_keyword(2, 2, "se",  TOKEN_CASE);
            }
        }
        break;
    case 'd': return check_keyword(1, 6, "efault", TOKEN_DEFAULT);
    case 'e': return check_keyword(1, 3, "lse",  TOKEN_ELSE);
    case 'f':
        if (scanner.curr - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return check_keyword(2, 1, "r",   TOKEN_FOR);
            case 'u': return check_keyword(2, 1, "n",   TOKEN_FUN);
            }
        }
        break;
    case 'i': return check_keyword(1, 1, "f",     TOKEN_IF);
    case 'l': return check_keyword(1, 5, "ambda", TOKEN_LAMBDA);
    case 'n': return check_keyword(1, 2, "il",    TOKEN_NIL);
    case 'o': return check_keyword(1, 1, "r",     TOKEN_OR);
    case 'p': return check_keyword(1, 4, "rint",  TOKEN_PRINT);
    case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        if (scanner.curr - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 't': return check_keyword(2, 4, "atic", TOKEN_STATIC);
            case 'u': return check_keyword(2, 3, "per",  TOKEN_SUPER);
            case 'w': return check_keyword(2, 4, "itch", TOKEN_SWITCH);
            }
        }
        break;
    case 't':
        if (scanner.curr - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
            case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v': return check_keyword(1, 2, "ar",   TOKEN_VAR);
    case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENT;
}

static Token ident()
{
    while (is_alpha(peek()) || is_digit(peek()))
        advance();
    return make_token(ident_type());
}

void scanner_init(const char *src)
{
    scanner.start = src;
    scanner.curr  = src;
    scanner.line  = 1;
}

Token scan_token()
{
    skip_whitespace();
    scanner.start = scanner.curr;
    if (at_end())
        return make_token(TOKEN_EOF);
    char c = advance();
    if (is_alpha(c))
        return ident();
    if (is_digit(c))
        return number();
    switch (c) {
    case '(': return make_token(TOKEN_LEFT_PAREN);
    case ')': return make_token(TOKEN_RIGHT_PAREN);
    case '{': return make_token(TOKEN_LEFT_BRACE);
    case '}': return make_token(TOKEN_RIGHT_BRACE);
    case ';': return make_token(TOKEN_SEMICOLON);
    case ',': return make_token(TOKEN_COMMA);
    case '.': return make_token(TOKEN_DOT);
    case '-': return make_token(TOKEN_MINUS);
    case '+': return make_token(TOKEN_PLUS);
    case '/': return make_token(TOKEN_SLASH);
    case '*': return make_token(TOKEN_STAR);
    case '?': return make_token(TOKEN_QMARK);
    case ':': return make_token(TOKEN_DCOLON);
    case '!': return make_token(match('=') ? TOKEN_BANG_EQ    : TOKEN_BANG);
    case '=': return make_token(match('=') ? TOKEN_EQ_EQ      : TOKEN_EQ);
    case '<': return make_token(match('=') ? TOKEN_LESS_EQ    : TOKEN_LESS);
    case '>': return make_token(match('=') ? TOKEN_GREATER_EQ : TOKEN_GREATER);
    case '"': return string();
    }
    return error_token("unexpected character");
}

