#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "scanner.h"
#include "object.h"
#include "vector.h"
#include "table.h"

#ifdef DEBUG_PRINT_CODE
#include "disassemble.h"
#endif

#define LOCAL_COUNT  UINT8_COUNT
#define GLOBAL_COUNT UINT8_COUNT
#define CONSTANT_MAX UINT8_MAX
#define JUMP_MAX     UINT16_MAX

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

typedef struct {
    Token name;
    int depth;
    bool is_const;
} Local;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
} FunctionType;

typedef struct {
    Token curr, prev;
    bool had_error;
    bool panic_mode;
    const char *file;
} Parser;

typedef struct Compiler {
    ObjFunction *fun;
    FunctionType type;
    Local *locals;//[LOCAL_COUNT];
    u32 local_count;
    int scope_depth;
    bool inside_loop;
    size_t loop_start;
    struct Compiler *enclosing;
} Compiler;

Parser parser;
Compiler *curr = NULL;
Table global_consts;



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
    return &curr->fun->chunk;
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

ObjString *token_to_string(Token *name) { return obj_copy_string(name->start, name->len); }



/* emitter */

static void emit_byte(u8 byte)
{
    chunk_write(curr_chunk(), byte, parser.prev.line);
}

static void emit_two(u8 b1, u8 b2)  { emit_byte(b1); emit_byte(b2); }
static void emit_return()           { emit_two(OP_NIL, OP_RETURN); }

static u8 make_constant(Value value)
{
    int constant = chunk_add_const(curr_chunk(), value);
    if (constant > CONSTANT_MAX) {
        error("too many constants in one chunk");
        return 0;
    }
    return (u8) constant;
}

static void emit_constant(Value value)
{
    emit_two(OP_CONSTANT, make_constant(value));
}

static size_t emit_branch(u8 instr)
{
    emit_byte(instr);
    emit_byte(0xFF);
    emit_byte(0xFF);
    return curr_chunk()->size - 2;
}

static void emit_loop(size_t loop_start)
{
    emit_byte(OP_BRANCH_BACK);
    size_t offset = curr_chunk()->size - loop_start + 2;
    if (offset > JUMP_MAX)
        error("loop body too large");
    emit_byte((offset >> 8) & 0xFF);
    emit_byte( offset       & 0xFF);
}



/* compiler */

static void compiler_init(Compiler *compiler, FunctionType type)
{
    compiler->enclosing = curr;
    compiler->fun = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->locals = ALLOCATE(Local, UINT24_COUNT);
    compiler->inside_loop = false;
    compiler->loop_start = 0;
    // we assign NULL to function first due to garbage collection
    compiler->fun = obj_make_fun();
    curr = compiler;
    if (type != TYPE_SCRIPT)
        curr->fun->name = obj_copy_string(parser.prev.start, parser.prev.len);

    Local *local = &curr->locals[curr->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.len = 0;
}

static ObjFunction *compiler_end()
{
    FREE_ARRAY(Local, curr->locals, UINT24_COUNT);
    emit_return();
    ObjFunction *fun = curr->fun;
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
        disassemble(curr_chunk(), fun->name != NULL ? fun->name->data : "<script>");
#endif
    curr = curr->enclosing;
    return fun;
}



/* scope and variable handling */

static void begin_scope()
{
    curr->scope_depth++;
}

static void end_scope()
{
    curr->scope_depth--;
    while (curr->local_count > 0
        && curr->locals[curr->local_count - 1].depth > curr->scope_depth) {
        emit_byte(OP_POP);
        curr->local_count--;
    }
}

static u8 make_ident_constant(Token *name)
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
    if (curr->local_count == UINT24_COUNT) {
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
            table_install(&global_consts, token_to_string(name), VALUE_MKNIL());
        else
            table_delete(&global_consts, token_to_string(name));
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
    if (curr->scope_depth == 0)
        return;
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

static u32 resolve_local(Compiler *compiler, Token *name, bool *found)
{
    *found = true;
    // backwards walk to find a variable with the same name as *name
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (local->depth == -1)
            error("can't read local variable in its own initializer");
        if (ident_equal(name, &local->name))
            return i;
    }
    *found = false;
    return 0;
}

static bool is_const_var(int arg, Token *name, bool local)
{
    if (local) {
        Local *local = &curr->locals[arg];
        return local->is_const;
    }
    Value v;
    return table_lookup(&global_consts, token_to_string(name), &v);
}




/* parser */

static ParseRule *get_rule(TokenType type);
static void stmt();
static void expr();
static void block();
static void expr_stmt();

static u8 parse_var(bool is_const, const char *errmsg)
{
    consume(TOKEN_IDENT, errmsg);
    declare_var(is_const);
    //     local?
    return curr->scope_depth > 0 ? 0 : make_ident_constant(&parser.prev);
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

static void function(FunctionType type)
{
    Compiler compiler;
    compiler_init(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "expected '(' after function name");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            curr->fun->arity++;
            if (curr->fun->arity > 255)
                error_curr("can't have more than 255 parameters");
            u8 constant = parse_var(false, "expected parameter name");
            define_var(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "expected ')' after function parameters");
    consume(TOKEN_LEFT_BRACE, "expected '{' before function body");
    block();

    ObjFunction *fun = compiler_end();
    emit_two(OP_CONSTANT, make_constant(VALUE_MKOBJ(fun)));
}

static void fun_decl()
{
    u8 global = parse_var(false, "expected function name");
    mark_initialized();
    function(TYPE_FUNCTION);
    define_var(global);
}

static void decl()
{
         if (match(TOKEN_VAR))   var_decl(false);
    else if (match(TOKEN_CONST)) var_decl(true);
    else if (match(TOKEN_FUN))   fun_decl();
    else                         stmt();
    if (parser.panic_mode)
        synchronize();
}

static void print_stmt()
{
    expr();
    consume(TOKEN_SEMICOLON, "expected ';' after value");
    emit_byte(OP_PRINT);
}

static void patch_branch(size_t offset)
{
    size_t jump = curr_chunk()->size - offset - 2;
    if (jump > JUMP_MAX)
        error("too much code to jump over");
    curr_chunk()->code[offset  ] = (jump >> 8) & 0xFF;
    curr_chunk()->code[offset+1] =  jump       & 0xFF;
}

static void if_stmt()
{
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'if'");
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after condition");
    size_t then_offset = emit_branch(OP_BRANCH_FALSE);
    emit_byte(OP_POP);
    stmt();
    size_t else_offset = emit_branch(OP_BRANCH);
    patch_branch(then_offset);
    emit_byte(OP_POP);
    if (match(TOKEN_ELSE))
        stmt();
    patch_branch(else_offset);
}

#define BEGIN_LOOP() \
    bool old_inside_loop = curr->inside_loop; \
    size_t old_loop_start = curr->loop_start; \
    curr->inside_loop = true;                 \
    curr->loop_start = loop_start;            \

#define END_LOOP() \
    curr->inside_loop = old_inside_loop; \
    curr->loop_start = old_loop_start;   \

static void while_stmt()
{
    size_t loop_start = curr_chunk()->size;
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'while'");
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after condition");
    size_t exit_offset = emit_branch(OP_BRANCH_FALSE);
    emit_byte(OP_POP);

    BEGIN_LOOP()
    stmt();
    END_LOOP()

    emit_loop(loop_start);
    patch_branch(exit_offset);
    emit_byte(OP_POP);
}

static void for_stmt()
{
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'for'");
         if (match(TOKEN_SEMICOLON)) ;
    else if (match(TOKEN_VAR))       var_decl(false);
    else if (match(TOKEN_CONST))     var_decl(true);
    else                             expr_stmt();

    size_t loop_start = curr_chunk()->size;
    size_t exit_offset = 0;
    if (!match(TOKEN_SEMICOLON)) {
        expr();
        consume(TOKEN_SEMICOLON, "expected ';' after loop condition");
        exit_offset = emit_branch(OP_BRANCH_FALSE);
        emit_byte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        size_t body_offset = emit_branch(OP_BRANCH);
        size_t increment_start = curr_chunk()->size;
        expr();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "expected ')' at end of 'for'");
        emit_loop(loop_start);
        loop_start = increment_start;
        patch_branch(body_offset);
    }

    BEGIN_LOOP()
    stmt();
    emit_loop(loop_start);
    END_LOOP()

    if (exit_offset != 0) {
        patch_branch(exit_offset);
        emit_byte(OP_POP);
    }

    end_scope();
}

#undef BEGIN_LOOP
#undef END_LOOP

static void return_stmt()
{
    if (curr->type == TYPE_SCRIPT)
        error("'return' statement at top level scope");
    if (match(TOKEN_SEMICOLON))
        emit_return();
    else {
        expr();
        consume(TOKEN_SEMICOLON, "expected semicolon after return expression");
        emit_byte(OP_RETURN);
    }
}

static void continue_stmt()
{
    if (!curr->inside_loop)
        error("continue statement not inside a loop");
    consume(TOKEN_SEMICOLON, "expected semicolon after 'continue'");
    emit_loop(curr->loop_start);
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        decl();
    consume(TOKEN_RIGHT_BRACE, "expected '}' at end of block");
}

static void expr_stmt()
{
    expr();
    consume(TOKEN_SEMICOLON, "expected ';' after value");
    emit_byte(OP_POP);
}

static void stmt()
{
         if (match(TOKEN_PRINT))  print_stmt();
    else if (match(TOKEN_IF))     if_stmt();
    else if (match(TOKEN_WHILE))  while_stmt();
    else if (match(TOKEN_FOR))    for_stmt();
    else if (match(TOKEN_RETURN)) return_stmt();
    else if (match(TOKEN_CONTINUE)) continue_stmt();
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

static void and_op(bool can_assign)
{
    size_t end_offset = emit_branch(OP_BRANCH_FALSE);
    emit_byte(OP_POP);
    parse_precedence(PREC_AND);
    patch_branch(end_offset);
}

static void or_op(bool can_assign)
{
    size_t else_offset = emit_branch(OP_BRANCH_FALSE);
    size_t end_offset  = emit_branch(OP_BRANCH);
    patch_branch(else_offset);
    emit_byte(OP_POP);
    parse_precedence(PREC_OR);
    patch_branch(end_offset);
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

static u8 arglist()
{
    u8 argc = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            if (argc == 255)
                error("function argument limit reached");
            else {
                expr();
                argc++;
            }
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "expected ')' after function arguments");
    return argc;
}

static void call(bool can_assign)
{
    u8 argc = arglist();
    emit_two(OP_CALL, argc);
}

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
    emit_constant(VALUE_MKOBJ(obj_copy_string(parser.prev.start + 1, parser.prev.len   - 2)));
}

static void grouping(bool can_assign)
{
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' at end of grouping expression");
}

static void named_var(Token name, bool can_assign)
{
    u8 getop, setop;
    bool local = false;
    bool found;
    int arg = resolve_local(curr, &name, &found);
    if (found) {
        getop = OP_GET_LOCAL;
        setop = OP_SET_LOCAL;
        local = true;
    } else {
        arg = make_ident_constant(&name);
        getop = OP_GET_GLOBAL;
        setop = OP_SET_GLOBAL;
    }
    if (can_assign && match(TOKEN_EQ)) {
        if (is_const_var(arg, &name, local)) {
            error("can't assign to const variable");
            return;
        }
        expr();
        if (local) {
            emit_byte(setop);
            emit_byte(arg & 0xFF);
            emit_byte((arg >> 8) & 0xFF);
            emit_byte((arg >> 16) & 0xFF);
        } else
            emit_two(setop, arg);
    } else {
        if (local) {
            emit_byte(getop);
            emit_byte(arg & 0xFF);
            emit_byte((arg >> 8) & 0xFF);
            emit_byte((arg >> 16) & 0xFF);
        } else
            emit_two(getop, arg);
    }
}

static void variable(bool can_assign)
{
    named_var(parser.prev, can_assign);
}

static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]  = { grouping,   call,   PREC_CALL },
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
    [TOKEN_AND]         = { NULL,       and_op, PREC_AND  },
    [TOKEN_CLASS]       = { NULL,       NULL,   PREC_NONE },
    [TOKEN_ELSE]        = { NULL,       NULL,   PREC_NONE },
    [TOKEN_FALSE]       = { literal,    NULL,   PREC_NONE },
    [TOKEN_FOR]         = { NULL,       NULL,   PREC_NONE },
    [TOKEN_FUN]         = { NULL,       NULL,   PREC_NONE },
    [TOKEN_IF]          = { NULL,       NULL,   PREC_NONE },
    [TOKEN_NIL]         = { literal,    NULL,   PREC_NONE },
    [TOKEN_OR]          = { NULL,       or_op,  PREC_OR   },
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



/* public functions */

ObjFunction *compile(const char *src, const char *filename)
{
    scanner_init(src);
    Compiler compiler;
    compiler_init(&compiler, TYPE_SCRIPT);
    parser.had_error  = false;
    parser.panic_mode = false;
    parser.file       = filename;

    advance();
    while (!match(TOKEN_EOF))
        decl();

    ObjFunction *fun = compiler_end();
    return parser.had_error ? NULL : fun;
}
