#ifndef SCANNER_H_INCLUDED
#define SCANNER_H_INCLUDED

typedef enum {
  // single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  // one or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQ, TOKEN_EQ, TOKEN_EQ_EQ,
  TOKEN_GREATER, TOKEN_GREATER_EQ, TOKEN_LESS, TOKEN_LESS_EQ,

  // literals.
  TOKEN_IDENT, TOKEN_STRING, TOKEN_NUMBER,

  // keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int len;
    int line;
} Token;

void scanner_init(const char *src);
Token scan_token();

#endif
