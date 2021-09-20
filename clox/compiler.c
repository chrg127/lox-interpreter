#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "chunk.h"
#include "scanner.h"
#include "object.h"
#include "table.h"
#include "memory.h"
#include "debug.h"
#include "list.h"

#ifdef DEBUG_PRINT_CODE
#include "disassemble.h"
#endif

#define LOCAL_COUNT     UINT16_COUNT
#define UPVALUE_COUNT   LOCAL_COUNT
#define GLOBAL_COUNT    UINT16_COUNT
#define CONSTANT_COUNT  UINT16_MAX
#define JUMP_MAX        UINT16_MAX

typedef enum {
    PREC_NONE,
    PREC_COMMA,    // ,
    PREC_ASSIGN,    // =
    PREC_OR,        // or
    PREC_AND,       // and
    PREC_EQ,        // == !=
    PREC_CMP,       // < > <= >=
    PREC_TERM,      // + -
    PREC_FACTOR,    // * /
    PREC_UNARY,     // ! -
    PREC_CALL,      // . ()
    PREC_LAMBDA,    // lambda
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
    bool is_captured;
} Local;

typedef struct {
    u16 index;
    bool is_local;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_CTOR,
    TYPE_METHOD,
    TYPE_STATIC,
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
    int local_count;
    int scope_depth;
    struct Compiler *enclosing;
    Local *locals;              // array of LOCAL_COUNT size, heap allocated
    Upvalue *upvalues;          // array of UPVALUE_COUNT size, heap allocated; count for upvalues is kept in fun
} Compiler;

typedef struct LoopCompiler {
    size_t start;
    VecSizet break_offsets;
    struct LoopCompiler *enclosing;
} LoopCompiler;

typedef struct ClassCompiler {
    bool has_super;
    struct ClassCompiler *enclosing;
} ClassCompiler;

Parser parser;
Compiler *curr            = NULL;
ClassCompiler *curr_class = NULL;
LoopCompiler *curr_loop   = NULL;
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
        case TOKEN_FOR:   case TOKEN_IF:  case TOKEN_WHILE:
        case TOKEN_PRINT: case TOKEN_RETURN:
        case TOKEN_CONST:
            return;
        default: // this is to silence switch warnings
            ;
        }
        advance();
    }
}



/* utilities */

static Chunk *curr_chunk() { return &curr->fun->chunk; }

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

static Token synthetic_token(const char *text)
{
    Token token = {
        .start = text,
        .len = strlen(text),
    };
    return token;
}



/* emitter */

static void emit_byte(u8 byte)
{
    chunk_write(curr_chunk(), byte, parser.prev.line);
}

static void emit_two(u8 b1, u8 b2)          { emit_byte(b1); emit_byte(b2); }
static void emit_three(u8 b1, u8 b2, u8 b3) { emit_byte(b1); emit_byte(b2); emit_byte(b3); }
static void emit_u16(u8 b1, u16 b2)         { emit_three(b1, b2 & 0xFF, (b2 >> 8) & 0xFF); }

static void emit_return()
{
    if (curr->type == TYPE_CTOR)
        emit_u16(OP_GET_LOCAL, 0);
    else
        emit_byte(OP_NIL);
    emit_byte(OP_RETURN);
}

static u16 make_constant(Value value)
{
    size_t constant = chunk_add_const(curr_chunk(), value);
    if (constant > CONSTANT_COUNT) {
        error("too many constants in one chunk");
        return 0;
    }
    return (u16) constant;
}

static void emit_constant(Value value)
{
    u16 offset = make_constant(value);
    if (offset < 0xFF)
        emit_two(OP_CONSTANT, offset & 0xFF);
    else
        emit_u16(OP_CONSTANT_LONG, offset);
}

static size_t emit_branch(u8 instr)
{
    emit_three(instr, 0xFF, 0xFF);
    return curr_chunk()->code.size - 2;
}

static void emit_loop(size_t loop_start)
{
    emit_byte(OP_BRANCH_BACK);
    size_t offset = curr_chunk()->code.size - loop_start + 2;
    if (offset > JUMP_MAX)
        error("loop body too large");
    emit_byte( offset       & 0xFF);
    emit_byte((offset >> 8) & 0xFF);
}



/* compiler */

static void compiler_init(Compiler *compiler, FunctionType type, Token *name)
{
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    // we assign NULL to function first due to garbage collection
    compiler->fun = NULL;
    compiler->fun = obj_make_fun();
    // these arrays are allocated manually so that the garbage collector is not triggered on these
    compiler->locals   = calloc(LOCAL_COUNT,   sizeof(Local));
    compiler->upvalues = calloc(UPVALUE_COUNT, sizeof(Upvalue));

    LIST_APPEND(compiler, curr, enclosing);

    if (type != TYPE_SCRIPT)
        curr->fun->name = obj_copy_string(name->start, name->len);

    Local *local = &curr->locals[curr->local_count++];
    local->depth       = 0;
    local->is_captured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.len   = 4;
    } else {
        local->name.start = "";
        local->name.len   = 0;
    }
}

static ObjFunction *compiler_end()
{
    emit_return();
    ObjFunction *fun = curr->fun;
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
        disassemble(curr_chunk(), fun->name != NULL ? fun->name->data : "<script>");
#endif
    curr = curr->enclosing;
    return fun;
}

static void compiler_free(Compiler *compiler)
{
    free(compiler->locals);
    free(compiler->upvalues);
}



/* scope and variable handling */

static void begin_scope() { curr->scope_depth++; }

static void end_scope()
{
    curr->scope_depth--;
    while (curr->local_count > 0 && curr->locals[curr->local_count - 1].depth > curr->scope_depth) {
        if (curr->locals[curr->local_count - 1].is_captured)
            emit_byte(OP_CLOSE_UPVALUE);
        else
            emit_byte(OP_POP);
        curr->local_count--;
    }
}

static u16 make_ident_constant(Token *name)
{
    return make_constant(VALUE_MKOBJ(token_to_string(name)));
}

static bool ident_equal(Token *a, Token *b)
{
    return a->len == b->len && memcmp(a->start, b->start, a->len) == 0;
}

static void add_local(Token name, bool is_const)
{
    if (curr->local_count == LOCAL_COUNT) {
        error("too many local variables in current block");
        return;
    }
    Local *local = &curr->locals[curr->local_count++];
    local->name        = name;
    local->depth       = -1;
    local->is_const    = is_const;
    local->is_captured = false;
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

static void define_var(u16 global)
{
    if (curr->scope_depth > 0) {
        mark_initialized();
        return;
    }
    emit_u16(OP_DEFINE_GLOBAL, global);
}

static int resolve_local(Compiler *compiler, Token *name)
{
    // backwards walk to find a variable with the same name as *name
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (ident_equal(name, &local->name)) {
            if (local->depth == -1)
                error("can't read local variable in its own initializer");
            return i;
        }
    }
    return -1;
}

static int add_upvalue(Compiler *compiler, u8 index, bool is_local)
{
    int count = compiler->fun->upvalue_count;

    // check if we already have a similar upvalue
    for (int i = 0; i < count; i++) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->is_local == is_local)
            return i;
    }

    if (count == UPVALUE_COUNT) {
        error("too many closure variables in function");
        return 0;
    }

    compiler->upvalues[count].is_local = is_local;
    compiler->upvalues[count].index    = index;
    return compiler->fun->upvalue_count++;
}

static int resolve_upvalue(Compiler *compiler, Token *name)
{
    if (compiler->enclosing == NULL)
        return -1;
    int local = resolve_local(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].is_captured = true;
        return add_upvalue(compiler, (u8) local, true);
    }
    int upvalue = resolve_upvalue(compiler->enclosing, name);
    if (upvalue != -1)
        return add_upvalue(compiler, (u8) upvalue, false);
    return -1;
}



/* parser */

static ParseRule *get_rule(TokenType type);
static void stmt();
static void expr();
static void assignment();
static void block();
static void expr_stmt();
static void variable(bool can_assign);

static u16 parse_var(bool is_const, const char *errmsg)
{
    consume(TOKEN_IDENT, errmsg);
    declare_var(is_const);
    //     local?
    return curr->scope_depth > 0 ? 0 : make_ident_constant(&parser.prev);
}

static void var_decl(bool is_const)
{
    u16 global = parse_var(is_const, "expected variable name");
    if (match(TOKEN_EQ))
        assignment();
    else
        emit_byte(OP_NIL);
    consume(TOKEN_SEMICOLON, "expected ';' after variable declaration");
    define_var(global);
}

static void function(FunctionType type, Token *name)
{
    Compiler compiler;
    compiler_init(&compiler, type, name);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "expected '(' after function name");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            curr->fun->arity++;
            if (curr->fun->arity > 255)
                error_curr("can't have more than 255 parameters");
            u16 constant = parse_var(false, "expected parameter name");
            define_var(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "expected ')' after function parameters");
    consume(TOKEN_LEFT_BRACE, "expected '{' before function body");
    block();

    ObjFunction *fun = compiler_end();

    if (fun->upvalue_count == 0) {
        emit_constant(VALUE_MKOBJ(fun));
        return;
    }

    emit_u16(OP_CLOSURE, make_constant(VALUE_MKOBJ(fun)));
    for (int i = 0; i < fun->upvalue_count; i++)
        emit_u16((u8)compiler.upvalues[i].is_local, compiler.upvalues[i].index);

    compiler_free(&compiler);
}

static void fun_decl()
{
    u16 global = parse_var(false, "expected function name");
    mark_initialized();
    function(TYPE_FUNCTION, &parser.prev);
    define_var(global);
}

static void named_var(Token name, bool can_assign)
{
    u8 getop, setop;
    bool is_const;
    int arg = resolve_local(curr, &name);

    if (arg != -1) {
        getop = OP_GET_LOCAL;
        setop = OP_SET_LOCAL;
        is_const = curr->locals[arg].is_const;
    } else if (arg = resolve_upvalue(curr, &name), arg != -1) {
        getop = OP_GET_UPVALUE;
        setop = OP_SET_UPVALUE;
        is_const = false;
    } else {
        arg = make_ident_constant(&name);
        getop = OP_GET_GLOBAL;
        setop = OP_SET_GLOBAL;
        Value v;
        is_const = table_lookup(&global_consts, token_to_string(&name), &v);
    }

    if (can_assign && match(TOKEN_EQ)) {
        if (is_const) {
            error("can't assign to const variable");
            return;
        }
        assignment();
        emit_u16(setop, arg);
    } else
        emit_u16(getop, arg);
}

static void method()
{
    bool is_static = match(TOKEN_STATIC);
    consume(TOKEN_IDENT, "expected method name");
    u16 constant = make_ident_constant(&parser.prev);
    FunctionType type = is_static ? TYPE_STATIC : TYPE_METHOD;
    if (!is_static && parser.prev.len == 4 && memcmp(parser.prev.start, "init", 4) == 0)
        type = TYPE_CTOR;
    function(type, &parser.prev);
    emit_u16(!is_static ? OP_METHOD : OP_STATIC, constant);
}

static void class_decl()
{
    consume(TOKEN_IDENT, "expected class name");
    Token class_name = parser.prev;
    u16 name_constant = make_ident_constant(&parser.prev);
    declare_var(false);
    emit_u16(OP_CLASS, name_constant);
    define_var(name_constant);

    ClassCompiler compiler;
    compiler.has_super = false;
    LIST_APPEND(&compiler, curr_class, enclosing);

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENT, "expected superclass name after '<'");
        variable(false);
        if (ident_equal(&class_name, &parser.prev))
            error("a class can't inherit from itself");
        begin_scope();
        add_local(synthetic_token("super"), true);
        define_var(0);
        named_var(class_name, false);
        emit_byte(OP_INHERIT);
    }

    named_var(class_name, false);
    consume(TOKEN_LEFT_BRACE, "expected '{' before class body");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        method();
    consume(TOKEN_RIGHT_BRACE, "expected '}' after class body");
    emit_byte(OP_POP);

    if (compiler.has_super)
        end_scope();

    curr_class = curr_class->enclosing;
}

static void decl()
{
         if (match(TOKEN_VAR))   var_decl(false);
    else if (match(TOKEN_CONST)) var_decl(true);
    else if (match(TOKEN_FUN))   fun_decl();
    else if (match(TOKEN_CLASS)) class_decl();
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
    size_t jump = curr_chunk()->code.size - offset - 2;
    if (jump > JUMP_MAX)
        error("too much code to jump over");
    curr_chunk()->code.data[offset  ] =  jump       & 0xFF;
    curr_chunk()->code.data[offset+1] = (jump >> 8) & 0xFF;
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

static void while_stmt()
{
    LoopCompiler compiler;
    LIST_APPEND(&compiler, curr_loop, enclosing);
    vec_size_t_init(&compiler.break_offsets);
    compiler.start = curr_chunk()->code.size;

    consume(TOKEN_LEFT_PAREN, "expected '(' after 'while'");
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after condition");
    size_t exit = emit_branch(OP_BRANCH_FALSE);
    emit_byte(OP_POP);

    stmt();

    emit_loop(compiler.start);
    patch_branch(exit);
    emit_byte(OP_POP);
    for (size_t i = 0; i < compiler.break_offsets.size; i++)
        patch_branch(compiler.break_offsets.data[i]);
    vec_size_t_free(&compiler.break_offsets);
    curr_loop = curr_loop->enclosing;
}

static void for_stmt()
{
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "expected '(' after 'for'");
         if (match(TOKEN_SEMICOLON)) ;
    else if (match(TOKEN_VAR))       var_decl(false);
    else if (match(TOKEN_CONST))     var_decl(true);
    else                             expr_stmt();

    LoopCompiler compiler;
    LIST_APPEND(&compiler, curr_loop, enclosing);
    vec_size_t_init(&compiler.break_offsets);
    compiler.start = curr_chunk()->code.size;
    size_t exit = 0;

    if (!match(TOKEN_SEMICOLON)) {
        expr();
        consume(TOKEN_SEMICOLON, "expected ';' after loop condition");
        exit = emit_branch(OP_BRANCH_FALSE);
        emit_byte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        size_t body_offset = emit_branch(OP_BRANCH);
        size_t increment_start = curr_chunk()->code.size;
        expr();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "expected ')' at end of 'for'");
        emit_loop(compiler.start);
        compiler.start = increment_start;
        patch_branch(body_offset);
    }

    stmt();

    emit_loop(compiler.start);
    if (exit != 0) {
        patch_branch(exit);
        emit_byte(OP_POP);
    }
    for (size_t i = 0; i < compiler.break_offsets.size; i++)
        patch_branch(compiler.break_offsets.data[i]);
    vec_size_t_free(&compiler.break_offsets);
    end_scope();
    curr_loop = curr_loop->enclosing;
}

static void return_stmt()
{
    if (curr->type == TYPE_SCRIPT)
        error("'return' statement at top level scope");
    if (match(TOKEN_SEMICOLON))
        emit_return();
    else {
        if (curr->type == TYPE_CTOR)
            error("can't return value from constructor");
        expr();
        consume(TOKEN_SEMICOLON, "expected semicolon after return expression");
        emit_byte(OP_RETURN);
    }
}

static void continue_stmt()
{
    if (curr_loop == NULL)
        error("continue statement not inside a loop");
    consume(TOKEN_SEMICOLON, "expected semicolon after 'continue'");
    emit_loop(curr_loop->start);
}

static void break_stmt()
{
    if (curr_loop == NULL)
        error("break statement not inside a loop");
    consume(TOKEN_SEMICOLON, "expected semicolon after 'break'");
    size_t offset = emit_branch(OP_BRANCH);
    vec_size_t_write(&curr_loop->break_offsets, offset);
}

static void switch_stmt()
{
    consume(TOKEN_LEFT_PAREN,  "expected '(' after switch");
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' after expression");

    consume(TOKEN_LEFT_BRACE, "expected '{' after ')'");
    VecSizet case_offsets;
    vec_size_t_init(&case_offsets);
    size_t offset = 0;
    while (!match(TOKEN_RIGHT_BRACE) && !match(TOKEN_DEFAULT)) {
        if (offset != 0)
            patch_branch(offset);
        consume(TOKEN_CASE, "expected 'case'");
        expr();
        consume(TOKEN_DCOLON, "expected ':' after expression");
        emit_byte(OP_EQ);
        offset = emit_branch(OP_BRANCH_FALSE);
        emit_byte(OP_POP);
        while (!check(TOKEN_CASE) && !check(TOKEN_RIGHT_BRACE) && !check(TOKEN_DEFAULT))
            stmt();
        vec_size_t_write(&case_offsets, emit_branch(OP_BRANCH));
    }
    if (offset != 0)
        patch_branch(offset);

    if (parser.prev.type == TOKEN_DEFAULT) {
        consume(TOKEN_DCOLON, "expected ':' after 'default'");
        while (!match(TOKEN_RIGHT_BRACE))
            stmt();
    }

    for (size_t i = 0; i < case_offsets.size; i++)
        patch_branch(case_offsets.data[i]);
    vec_size_t_free(&case_offsets);
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
    if (parser.prev.type != TOKEN_SEMICOLON) {
        consume(TOKEN_SEMICOLON, "expected ';' after value");
        emit_byte(OP_POP);
    }
}

static void stmt()
{
         if (match(TOKEN_PRINT))    print_stmt();
    else if (match(TOKEN_IF))       if_stmt();
    else if (match(TOKEN_WHILE))    while_stmt();
    else if (match(TOKEN_FOR))      for_stmt();
    else if (match(TOKEN_RETURN))   return_stmt();
    else if (match(TOKEN_CONTINUE)) continue_stmt();
    else if (match(TOKEN_BREAK))    break_stmt();
    else if (match(TOKEN_SWITCH))   switch_stmt();
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
    parse_precedence(PREC_COMMA);
}

static void assignment()
{
    parse_precedence(PREC_ASSIGN);
}

static void comma(bool can_assign)
{
    emit_byte(OP_POP);
    parse_precedence(PREC_COMMA);
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
                assignment();
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

static void dot(bool can_assign)
{
    consume(TOKEN_IDENT, "expected property name after '.'");
    u16 name = make_ident_constant(&parser.prev);
    if (can_assign && match(TOKEN_EQ)) {
        assignment();
        emit_u16(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        u8 argc = arglist();
        emit_u16(OP_INVOKE, name);
        emit_byte(argc);
    } else
        emit_u16(OP_GET_PROPERTY, name);
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
    // this, I believe, is the only place where we can make use of SSO
    emit_constant(obj_make_ssostring(parser.prev.start + 1, parser.prev.len - 2));
}

static void grouping(bool can_assign)
{
    expr();
    consume(TOKEN_RIGHT_PAREN, "expected ')' at end of grouping expression");
}

static void variable(bool can_assign)
{
    named_var(parser.prev, can_assign);
}

static void this_op(bool can_assign)
{
    if (curr_class == NULL) {
        error("can't use 'this' outside of a class");
        return;
    }
    variable(false);
}

static void super_op(bool can_assign)
{
    if (curr_class == NULL)
        error("'super' outside a class");
    else if (!curr_class->has_super)
        error("'super' inside class without superclass");
    consume(TOKEN_DOT, "expected '.' after 'super'");
    consume(TOKEN_IDENT, "expected superclass method name");
    u16 name = make_ident_constant(&parser.prev);
    named_var(synthetic_token("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        u8 argc = arglist();
        named_var(synthetic_token("super"), false);
        emit_u16(OP_SUPER_INVOKE, name);
        emit_byte(argc);
    } else {
        named_var(synthetic_token("super"), false);
        emit_u16(OP_GET_SUPER, name);
    }
}

static void lambda(bool can_assign)
{
    Token name = synthetic_token("lambda");
    function(TYPE_FUNCTION, &name);
}

static void semicolon(bool can_assign) {}

static ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]  = { grouping,   call,   PREC_CALL   },
    [TOKEN_RIGHT_PAREN] = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_LEFT_BRACE]  = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_RIGHT_BRACE] = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_COMMA]       = { NULL,       comma,  PREC_COMMA  },
    [TOKEN_DOT]         = { NULL,       dot,    PREC_CALL   },
    [TOKEN_MINUS]       = { unary,      binary, PREC_TERM   },
    [TOKEN_PLUS]        = { NULL,       binary, PREC_TERM   },
    [TOKEN_SEMICOLON]   = { semicolon,  NULL,   PREC_NONE   },
    [TOKEN_SLASH]       = { NULL,       binary, PREC_FACTOR },
    [TOKEN_STAR]        = { NULL,       binary, PREC_FACTOR },
    [TOKEN_QMARK]       = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_DCOLON]      = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_BANG]        = { unary,      NULL,   PREC_NONE   },
    [TOKEN_BANG_EQ]     = { NULL,       binary, PREC_EQ     },
    [TOKEN_EQ]          = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_EQ_EQ]       = { NULL,       binary, PREC_EQ     },
    [TOKEN_GREATER]     = { NULL,       binary, PREC_CMP    },
    [TOKEN_GREATER_EQ]  = { NULL,       binary, PREC_CMP    },
    [TOKEN_LESS]        = { NULL,       binary, PREC_CMP    },
    [TOKEN_LESS_EQ]     = { NULL,       binary, PREC_CMP    },
    [TOKEN_IDENT]       = { variable,   NULL,   PREC_NONE   },
    [TOKEN_STRING]      = { string,     NULL,   PREC_NONE   },
    [TOKEN_NUMBER]      = { number,     NULL,   PREC_NONE   },
    [TOKEN_AND]         = { NULL,       and_op, PREC_AND    },
    [TOKEN_CLASS]       = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_ELSE]        = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_FALSE]       = { literal,    NULL,   PREC_NONE   },
    [TOKEN_FOR]         = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_FUN]         = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_LAMBDA]      = { lambda,     NULL,   PREC_LAMBDA },
    [TOKEN_IF]          = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_NIL]         = { literal,    NULL,   PREC_NONE   },
    [TOKEN_OR]          = { NULL,       or_op,  PREC_OR     },
    [TOKEN_PRINT]       = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_RETURN]      = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_SUPER]       = { super_op,   NULL,   PREC_NONE   },
    [TOKEN_THIS]        = { this_op,    NULL,   PREC_NONE   },
    [TOKEN_TRUE]        = { literal,    NULL,   PREC_NONE   },
    [TOKEN_VAR]         = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_WHILE]       = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_ERROR]       = { NULL,       NULL,   PREC_NONE   },
    [TOKEN_EOF]         = { NULL,       NULL,   PREC_NONE   },
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
    compiler_init(&compiler, TYPE_SCRIPT, /* token = */ NULL);
    parser.had_error  = false;
    parser.panic_mode = false;
    parser.file       = filename;

    advance();
    while (!match(TOKEN_EOF))
        decl();

    ObjFunction *fun = compiler_end();
    compiler_free(&compiler);
    return parser.had_error ? NULL : fun;
}

void compiler_mark_roots()
{
    Compiler *compiler = curr;
    while (compiler != NULL) {
        gc_mark_obj((Obj *) compiler->fun);
        compiler = compiler->enclosing;
    }
}
