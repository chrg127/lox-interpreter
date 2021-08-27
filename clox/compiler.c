#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "scanner.h"
#include "object.h"
#include "vector.h"

#ifdef DEBUG_PRINT_CODE
#include "disassemble.h"
#endif

typedef struct {
    u8 *data;
    size_t size;
    size_t cap;
} ConstGlobalArray;

static struct {
    Token curr, prev;
    bool had_error;
    bool panic_mode;
    const char *file;
    ConstGlobalArray global_consts;
} parser;

typedef struct {
    Token name;
    int depth;
    bool is_const;
} Local;

typedef struct {
    Local locals[UINT8_COUNT];
    int local_count;
    int scope_depth;
} Compiler;

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

// can_assign is used to avoid expressions such as 'a * b = c + d;'
typedef void (*ParseFn)(bool can_assign);
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence prec;
} ParseRule;

Chunk *compiling_chunk;
Compiler *curr = NULL;



/* error handling */

static void advance();

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

// on error, synchronize by reaching a statement boundary
static void synchronize()
{
    parser.panic_mode = false;
    while (parser.curr.type != TOKEN_EOF) {
        if (parser.prev.type == TOKEN_SEMICOLON)
            return;
        switch (parser.curr.type) {
        case TOKEN_CLASS: case TOKEN_FUN: case TOKEN_VAR:
        case TOKEN_FOR: case TOKEN_IF: case TOKEN_WHILE:
        case TOKEN_PRINT: case TOKEN_RETURN:
        case TOKEN_CONST:
            return;
        default:
            ;
        }
        advance();
    }
}



/* utilities */

static Chunk *curr_chunk()
{
    return compiling_chunk;
}

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

static bool check(TokenType type)
{
    return parser.curr.type == type;
}

static bool match(TokenType type)
{
    if (!check(type))
        return false;
    advance();
    return true;
}

VECTOR_DEFINE(ConstGlobalArray, u8, globarr, data)

size_t globarr_search(ConstGlobalArray *arr, u8 elem)
{
    for (size_t i = 0; i < arr->size; i++) {
        if (arr->data[i] == elem)
            return i;
    }
    return -1;
}

void globarr_delete(ConstGlobalArray *arr, u8 elem)
{
    size_t i = globarr_search(arr, elem);
    if (i == -1)
        return;
    memmove(&arr->data[i], &arr->data[i+1], arr->size - i);
    arr->size--;
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


/* scope handling */

static void begin_scope()
{
    curr->scope_depth++;
}

static void end_scope()
{
    curr->scope_depth--;
    while (curr->local_count > 0 && curr->locals[curr->local_count - 1].depth > curr->scope_depth) {
        emit_byte(OP_POP);
        curr->local_count--;
    }
}



/* parser */

static ParseRule *get_rule(TokenType type);
static void stmt();
static void decl();
static void expr();

static u8 ident_const(Token *name)
{
    return make_constant(VALUE_MKOBJ(obj_copy_string(name->start, name->len)));
}

static bool ident_equal(Token *a, Token *b)
{
    if (a->len != b->len)
        return false;
    return memcmp(a->start, b->start, a->len) == 0;
}

static void add_local(Token name, bool is_const)
{
    if (curr->local_count == UINT8_COUNT) {
        error("too many local variables in current block");
        return;
    }
    Local *local = &curr->locals[curr->local_count++];
    local->name = name;
    local->depth = -1;
    local->is_const = is_const;
}

static void declare_var(bool is_const)
{
    Token *name = &parser.prev;
    // global?
    if (curr->scope_depth == 0) {
        // first record that this global variable is const, then return
        if (is_const)
            globarr_write(&parser.global_consts, ident_const(name));
        else
            globarr_delete(&parser.global_consts, ident_const(name));
        return;
    }
    // check for declaration inside current scope
    for (int i = curr->local_count - 1; i >= 0; i--) {
        Local *local = &curr->locals[i];
        if (local->depth != -1 && local->depth < curr->scope_depth)
            break;
        if (ident_equal(name, &local->name))
            error("redeclaration of variable in the same scope");
    }
    add_local(*name, is_const);
}

static void mark_initialized()
{
    curr->locals[curr->local_count-1].depth = curr->scope_depth;
}

static void define_var(u8 global)
{
    if (curr->scope_depth > 0) {
        mark_initialized();
        return;
    }
    emit_two(OP_DEFINE_GLOBAL, global);
}

static u8 parse_var(bool is_const, const char *errmsg)
{
    consume(TOKEN_IDENT, errmsg);
    declare_var(is_const);
    //     local?
    return curr->scope_depth > 0 ? 0 : ident_const(&parser.prev);
}

static void var_decl(bool is_const)
{
    u8 global = parse_var(is_const, "expected variable name");
    if (match(TOKEN_EQ))
        expr();
    else
        emit_byte(OP_NIL);
    consume(TOKEN_SEMICOLON, "expected ';' after variable declaration");
    define_var(global);
}

static void decl()
{
    if (match(TOKEN_VAR))
        var_decl(false);
    else if (match(TOKEN_CONST))
        var_decl(true);
    else
        stmt();

    if (parser.panic_mode)
        synchronize();
}

static void print_stmt()
{
    expr();
    consume(TOKEN_SEMICOLON, "expected ';' after value");
    emit_byte(OP_PRINT);
}

static void expr_stmt()
{
    expr();
    consume(TOKEN_SEMICOLON, "expected ';' after value");
    emit_byte(OP_POP);
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        decl();
    consume(TOKEN_RIGHT_BRACE, "expected '}' at end of block");
}

static void stmt()
{
    if (match(TOKEN_PRINT))
        print_stmt();
    else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expr_stmt();
    }
}

static void parse_precedence(Precedence prec)
{
    advance();
    ParseFn prefix_rule = get_rule(parser.prev.type)->prefix;
    if (prefix_rule == NULL) {
        error("expected expression");
        return;
    }
    bool can_assign = prec <= PREC_ASSIGN;
    prefix_rule(can_assign);
    while (prec <= get_rule(parser.curr.type)->prec) {
        advance();
        ParseFn infix_rule = get_rule(parser.prev.type)->infix;
        infix_rule(can_assign);
        if (can_assign && match(TOKEN_EQ))
            error("invalid assignment target");
    }
}

static void expr()
{
    parse_precedence(PREC_ASSIGN);
}

static void binary(bool can_assign)
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

static void unary(bool can_assign)
{
    TokenType op = parser.prev.type;
    parse_precedence(PREC_UNARY);

    switch (op) {
    case TOKEN_BANG:  emit_byte(OP_NOT);    break;
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    default: return; // unreachable
    }
}

static void literal(bool can_assign)
{
    switch (parser.prev.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL:   emit_byte(OP_NIL);   break;
    case TOKEN_TRUE:  emit_byte(OP_TRUE);  break;
    default:          return; // unreachable
    }
}

static void number(bool can_assign)
{
    double value = strtod(parser.prev.start, NULL);
    emit_constant(VALUE_MKNUM(value));
}

static void string(bool can_assign)
{
    // emit_constant(VALUE_MKOBJ(make_string_nonowning((char *)parser.prev.start + 1, parser.prev.len - 2)));
    emit_constant(VALUE_MKOBJ(obj_copy_string(parser.prev.start + 1,
                                              parser.prev.len   - 2)));
}

static void grouping(bool can_assign)
{
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' at end of grouping expression");
}

static int resolve_local(Compiler *compiler, Token *name)
{
    // backwards walk to find a variable with the same name as *name
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (local->depth == -1)
            error("can't read local variable in its own initializer");
        if (ident_equal(name, &local->name))
            return i;
    }
    return -1;
}

static bool is_const_var(int arg, bool local)
{
    if (local) {
        Local *local = &curr->locals[arg];
        return local->is_const;
    } else
        return globarr_search(&parser.global_consts, arg) != -1;
}

static void named_var(Token name, bool can_assign)
{
    u8 getop, setop;
    bool local = false;
    int arg = resolve_local(curr, &name);
    if (arg != -1) {
        getop = OP_GET_LOCAL;
        setop = OP_SET_LOCAL;
        local = true;
    } else {
        arg = ident_const(&name);
        getop = OP_GET_GLOBAL;
        setop = OP_SET_GLOBAL;
    }
    if (can_assign && match(TOKEN_EQ)) {
        if (is_const_var(arg, local)) {
            error("can't assign to const variable");
            return;
        }
        expr();
        emit_two(setop, arg);
    } else
        emit_two(getop, arg);
}

static void variable(bool can_assign)
{
    named_var(parser.prev, can_assign);
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
    [TOKEN_IDENT]       = { variable,   NULL,   PREC_NONE },
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
    [TOKEN_CONST]       = { NULL,       NULL,   PREC_NONE },
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

static void compiler_init(Compiler *compiler)
{
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    curr = compiler;
}



/* public functions */

void compile_init()
{
    globarr_init(&parser.global_consts);
}

void compile_free()
{
    globarr_free(&parser.global_consts);
}

bool compile(const char *src, Chunk *chunk, const char *filename)
{
    scanner_init(src);
    Compiler compiler;
    compiler_init(&compiler);
    compiling_chunk = chunk;
    parser.had_error  = false;
    parser.panic_mode = false;
    parser.file       = filename;
    advance();

    while (!match(TOKEN_EOF))
        decl();

    consume(TOKEN_EOF, "expected end of expression");
    end_compiler();
    return !parser.had_error;
}

