#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "scanner.h"
#include "object.h"

#ifdef DEBUG_PRINT_CODE
#include "disassemble.h"
#endif

static struct {
    Token curr, prev;
    bool had_error;
    bool panic_mode;
    const char *file;
} parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,    // =
    PREC_CONDEXPR,  // ?:
    PREC_OR,        // or
    PREC_AND,       // and
    PREC_EQ,        // == !=
    PREC_CMP,       // < > <= >=
    PREC_TERM,      // + -
    PREC_FACTOR,    // * /
    PREC_UNARY,     // ! -
    PREC_CALL,      // . ()
    PREC_PRIMARY,
} Precedence;

typedef void (*ParseFn)();
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence prec;
} ParseRule;

Chunk *compiling_chunk;

static Chunk *curr_chunk()
{
    return compiling_chunk;
}



/* error handling */

static void error_at(Token *token, const char *msg)
{
    if (parser.panic_mode)
        return;
    parser.panic_mode = true;
    fprintf(stderr, "%s:%d: parse error", parser.file, token->line);
    switch (token->type) {
    case TOKEN_EOF: fprintf(stderr, " at end"); break;
    case TOKEN_ERROR: break;
    default: fprintf(stderr, " at '%.*s'", token->len, token->start); break;
    }
    fprintf(stderr, ": %s\n", msg);
    parser.had_error = true;
}

static void error(const char *msg)      { error_at(&parser.prev, msg); }
static void error_curr(const char *msg) { error_at(&parser.curr, msg); }



/* utilities */

static void advance()
{
    parser.prev = parser.curr;
    for (;;) {
        parser.curr = scan_token();
        if (parser.curr.type != TOKEN_ERROR)
            break;
        error_curr(parser.curr.start);
    }
}

static void consume(TokenType type, const char *msg)
{
    if (parser.curr.type == type) {
        advance();
        return;
    }
    error_curr(msg);
}



/* emitter */

static void emit_byte(u8 byte)
{
    chunk_write(curr_chunk(), byte, parser.prev.line);
}

static void emit_two(u8 b1, u8 b2)  { emit_byte(b1); emit_byte(b2); }
static void emit_return()           { emit_byte(OP_RETURN); }

static u8 make_constant(Value value)
{
    int constant = chunk_add_const(curr_chunk(), value);
    if (constant > UINT8_MAX) {
        error("too many constants in one chunk");
        return 0;
    }
    return (u8) constant;
}

static void emit_constant(Value value)
{
    // could also be:
    // chunk_write_const(curr_chunk(), value, parser.prev.line);
    emit_two(OP_CONSTANT, make_constant(value));
}



/* parser */

static ParseRule *get_rule(TokenType type);

static void parse_precedence(Precedence prec)
{
    advance();
    ParseFn prefix_rule = get_rule(parser.prev.type)->prefix;
    if (prefix_rule == NULL) {
        error("expected expression");
        return;
    }
    prefix_rule();
    while (prec <= get_rule(parser.curr.type)->prec) {
        advance();
        ParseFn infix_rule = get_rule(parser.prev.type)->infix;
        infix_rule();
    }
}

static void expr()
{
    parse_precedence(PREC_ASSIGN);
}

static void binary()
{
    TokenType op = parser.prev.type;
    ParseRule *rule = get_rule(op);
    parse_precedence((Precedence)(rule->prec + 1));

    switch (op) {
    case TOKEN_BANG_EQ: emit_two(OP_EQ, OP_NOT); break;
    case TOKEN_EQ_EQ:   emit_byte(OP_EQ); break;
    case TOKEN_GREATER: emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQ: emit_two(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:    emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQ: emit_two(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:    emit_byte(OP_ADD); break;
    case TOKEN_MINUS:   emit_byte(OP_SUB); break;
    case TOKEN_STAR:    emit_byte(OP_MUL); break;
    case TOKEN_SLASH:   emit_byte(OP_DIV); break;
    default: return; // unreachable
    }
}

// static void condexpr()
// {
//     // emit OP_TEST
//     // call to parse_precedence will emit left branch
//     parse_precedence(PREC_CONDEXPR);
//     consume(TOKEN_DCOLON, "expected ':' after left branch of '?'");
//     parse_precedence(PREC_CONDEXPR);
// }

static void unary()
{
    TokenType op = parser.prev.type;
    parse_precedence(PREC_UNARY);

    switch (op) {
    case TOKEN_BANG:  emit_byte(OP_NOT);    break;
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    default: return; // unreachable
    }
}

static void literal()
{
    switch (parser.prev.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL:   emit_byte(OP_NIL);   break;
    case TOKEN_TRUE:  emit_byte(OP_TRUE);  break;
    default:          return; // unreachable
    }
}

static void number()
{
    double value = strtod(parser.prev.start, NULL);
    emit_constant(VALUE_MKNUM(value));
}

static void string()
{
    // emit_constant(VALUE_MKOBJ(make_string_nonowning((char *)parser.prev.start + 1, parser.prev.len - 2)));
    emit_constant(VALUE_MKOBJ(obj_copy_string(parser.prev.start + 1,
                                              parser.prev.len   - 2)));
}

static void grouping()
{
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' at end of grouping expression");
}

static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]  = { grouping,   NULL,   PREC_NONE },
    [TOKEN_RIGHT_PAREN] = { NULL,       NULL,   PREC_NONE },
    [TOKEN_LEFT_BRACE]  = { NULL,       NULL,   PREC_NONE },
    [TOKEN_RIGHT_BRACE] = { NULL,       NULL,   PREC_NONE },
    [TOKEN_COMMA]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_DOT]         = { NULL,       NULL,   PREC_NONE },
    [TOKEN_MINUS]       = { unary,      binary, PREC_TERM },
    [TOKEN_PLUS]        = { NULL,       binary, PREC_TERM },
    [TOKEN_SEMICOLON]   = { NULL,       NULL,   PREC_NONE },
    [TOKEN_SLASH]       = { NULL,       binary, PREC_FACTOR },
    [TOKEN_STAR]        = { NULL,       binary, PREC_FACTOR },
    [TOKEN_QMARK]       = { NULL,       NULL,   PREC_NONE }, //condexpr,   PREC_CONDEXPR },
    [TOKEN_DCOLON]      = { NULL,       NULL,   PREC_NONE },
    [TOKEN_BANG]        = { unary,      NULL,   PREC_NONE   },
    [TOKEN_BANG_EQ]     = { NULL,       binary, PREC_EQ     },
    [TOKEN_EQ]          = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_EQ_EQ]       = { NULL,       binary, PREC_EQ     },
    [TOKEN_GREATER]     = { NULL,       binary, PREC_CMP    },
    [TOKEN_GREATER_EQ]  = { NULL,       binary, PREC_CMP    },
    [TOKEN_LESS]        = { NULL,       binary, PREC_CMP    },
    [TOKEN_LESS_EQ]     = { NULL,       binary, PREC_CMP    },
    [TOKEN_IDENT]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_STRING]      = { string,     NULL,   PREC_NONE },
    [TOKEN_NUMBER]      = { number,     NULL,   PREC_NONE },
    [TOKEN_AND]         = { NULL,       NULL,   PREC_NONE },
    [TOKEN_CLASS]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_ELSE]        = { NULL,       NULL,   PREC_NONE },
    [TOKEN_FALSE]       = { literal,    NULL,   PREC_NONE },
    [TOKEN_FOR]         = { NULL,       NULL,   PREC_NONE },
    [TOKEN_FUN]         = { NULL,       NULL,   PREC_NONE },
    [TOKEN_IF]          = { NULL,       NULL,   PREC_NONE },
    [TOKEN_NIL]         = { literal,    NULL,   PREC_NONE },
    [TOKEN_OR]          = { NULL,       NULL,   PREC_NONE },
    [TOKEN_PRINT]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_RETURN]      = { NULL,       NULL,   PREC_NONE },
    [TOKEN_SUPER]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_THIS]        = { NULL,       NULL,   PREC_NONE },
    [TOKEN_TRUE]        = { literal,    NULL,   PREC_NONE },
    [TOKEN_VAR]         = { NULL,       NULL,   PREC_NONE },
    [TOKEN_WHILE]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_ERROR]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_EOF]         = { NULL,       NULL,   PREC_NONE },
};

static ParseRule *get_rule(TokenType type)
{
    return &rules[type];
}

static void end_compiler()
{
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
        disassemble(curr_chunk(), "code");
#endif
}



/* public functions */

bool compile(const char *src, Chunk *chunk, const char *filename)
{
    scanner_init(src);
    compiling_chunk = chunk;
    parser.had_error  = false;
    parser.panic_mode = false;
    parser.file       = filename;
    advance();
    expr();
    consume(TOKEN_EOF, "expected end of expression");
    end_compiler();
    return !parser.had_error;
}

