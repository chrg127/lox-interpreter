#ifndef SCANNER_H_INCLUDED
#define SCANNER_H_INCLUDED

#include <stddef.h>

typedef enum {
  // single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
  TOKEN_QMARK, TOKEN_DCOLON,
  TOKEN_LEFT_SQUARE, TOKEN_RIGHT_SQUARE,
  TOKEN_3DOTS,

  // one or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQ, TOKEN_EQ, TOKEN_EQ_EQ,
  TOKEN_GREATER, TOKEN_GREATER_EQ, TOKEN_LESS, TOKEN_LESS_EQ,

  // literals.
  TOKEN_IDENT, TOKEN_STRING, TOKEN_NUMBER,

  // keywords.
  TOKEN_AND, TOKEN_BREAK, TOKEN_CLASS, TOKEN_CASE, TOKEN_CONST, TOKEN_CONTINUE,
  TOKEN_DEFAULT, TOKEN_ELSE, TOKEN_FALSE, TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_INCLUDE,
  TOKEN_LAMBDA, TOKEN_NIL, TOKEN_OPERATOR, TOKEN_OR, TOKEN_PRINT, TOKEN_RETURN,
  TOKEN_STATIC, TOKEN_SUPER, TOKEN_SWITCH, TOKEN_THIS, TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int len;
    int line;
} Token;

typedef struct Scanner {
    const char *start;
    const char *curr;
    size_t line;
    struct Scanner *enclosing;
} Scanner;

void scanner_init(Scanner *scanner, const char *src);
void scanner_end();
Token scan_token();

#endif
