package lox;

import java.util.Arrays;
import java.util.List;
import java.util.ArrayList;
import java.util.function.Supplier;
import static lox.Token.Type.*;

class Parser {
    private static class ParseError extends RuntimeException {}

    private final List<Token> tokens;
    private int curr = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    public List<Stmt> parse() {
        List<Stmt> stmts = new ArrayList<>();
        while (!isAtEnd()) {
            stmts.add(decl());
        }
        return stmts;
    }

    private Stmt decl() {
        try {
            if (match(VAR)) return varDecl();
            if (match(CLASS)) return classDecl();
            if (match(FUN)) return function("function");
            return stmt();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    private Stmt varDecl() {
        Token name = consume(IDENT, "expected variable name");
        Expr initializer = null;
        if (match(EQ)) {
            initializer = expression();
        }
        consume(SEMICOLON, "expected ';' after variable declaration");
        return new Stmt.Var(name, initializer);
    }

    private Stmt classDecl() {
        Token name = consume(IDENT, "expected class name");
        Expr.Variable superclass = null;
        if (match(LESS)) {
            consume(IDENT, "expected superclass name after '<'");
            superclass = new Expr.Variable(previous());
        }
        consume(LEFT_BRACE, "expected '{' before class body");
        List<Stmt.Function> methods = new ArrayList<>();
        while (!check(RIGHT_BRACE) && !isAtEnd())
            methods.add(function("method"));
        consume(RIGHT_BRACE, "expected '}' after class body");
        return new Stmt.Class(name, superclass, methods);
    }

    private Stmt stmt() {
        if (match(FOR))        return forStmt();
        if (match(IF))         return ifStmt();
        if (match(PRINT))      return printStmt();
        if (match(RETURN))     return returnStmt();
        if (match(WHILE))      return whileStmt();
        if (match(LEFT_BRACE)) return new Stmt.Block(block());
        return expressionStmt();
    }

    private Stmt forStmt() {
        consume(LEFT_PAREN, "expected '(' after 'for'");
        Stmt init = match(SEMICOLON) ? null
                  : match(VAR)       ? varDecl()
                  :                    expressionStmt();
        Expr cond = check(SEMICOLON) ? null
                                     : expression();
        consume(SEMICOLON, "expected ';' after loop condition");
        Expr increment = check(RIGHT_PAREN) ? null
                                            : expression();
        consume(RIGHT_PAREN, "expected ')' after for clause");
        Stmt body = stmt();

        // desugaring
        if (increment != null) {
            body = new Stmt.Block(
                Arrays.asList(body, new Stmt.Expression(increment))
            );
        }
        if (cond == null)
            cond = new Expr.Literal(true);
        body = new Stmt.While(cond, body);
        if (init != null)
            body = new Stmt.Block(Arrays.asList(init, body));

        return body;
    }

    private Stmt ifStmt() {
        consume(LEFT_PAREN, "expected '(' after 'if'");
        Expr cond = expression();
        consume(RIGHT_PAREN, "expected ')' after if condition");
        Stmt thenBranch = stmt();
        Stmt elseBranch = match(ELSE) ? stmt() : null;
        return new Stmt.If(cond, thenBranch, elseBranch);
    }


    private Stmt printStmt() {
        Expr value = expression();
        consume(SEMICOLON, "expected ';' after expression");
        return new Stmt.Print(value);
    }

    private Stmt returnStmt() {
        Token keyword = previous();
        Expr value = null;
        if (!check(SEMICOLON))
            value = expression();
        consume(SEMICOLON, "expected ';' after return value");
        return new Stmt.Return(keyword, value);
    }

    private Stmt whileStmt() {
        consume(LEFT_PAREN, "expected '(' after 'while'");
        Expr cond = expression();
        consume(RIGHT_PAREN, "expected ')' after condition");
        Stmt body = stmt();
        return new Stmt.While(cond, body);
    }

    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<>();
        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            statements.add(decl());
        }
        consume(RIGHT_BRACE, "expected '}' after block");
        return statements;
    }

    private Stmt expressionStmt() {
        Expr expr = expression();
        consume(SEMICOLON, "expected ';' after expression");
        return new Stmt.Expression(expr);
    }

    private Stmt.Function function(String kind) {
        Token name = consume(IDENT, "expected " + kind + " name");
        consume(LEFT_PAREN, "expected '(' after " + kind + " name");
        List<Token> params = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                if (params.size() >= 255)
                    error(peek(), "can't have more than 255 parameters");
                params.add(consume(IDENT, "expected parameter name"));
            } while(match(COMMA));
        }
        consume(RIGHT_PAREN, "expected ')' after parameters");
        consume(LEFT_BRACE, "expected '{' before " + kind + " body");
        List<Stmt> body = block();
        return new Stmt.Function(name, params, body);
    }

    private Expr expression() {
        return assignment();
    }

    private Expr binaryOp(Supplier<Expr> operandExpr, Token.Type... types) {
        Expr expr = operandExpr.get();
        while (match(types)) {
            Token op = previous();
            Expr right = operandExpr.get();
            expr = new Expr.Binary(expr, op, right);
        }
        return expr;
    }

    private Expr comma() {
        return binaryOp(this::assignment, COMMA);
    }

    private Expr assignment() {
        Expr expr = or();

        if (match(EQ)) {
            Token eq = previous();
            Expr value = assignment();
            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable)expr).name;
                return new Expr.Assign(name, value);
            } else if (expr instanceof Expr.Get) {
                Expr.Get get = (Expr.Get) expr;
                return new Expr.Set(get.object, get.name, value);
            }
            error(eq, "invalid assignment target");
        }
        return expr;
    }

    private Expr or() {
        Expr expr = and();
        while (match(OR)) {
            Token op = previous();
            Expr right = and();
            expr = new Expr.Logical(expr, op, right);
        }
        return expr;
    }

    private Expr and() {
        Expr expr = equality();
        while (match(AND)) {
            Token op = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, op, right);
        }
        return expr;
    }

    private Expr equality() {
        return binaryOp(this::comparison, BANG_EQ, EQ_EQ);
    }

    private Expr comparison() {
        return binaryOp(this::term, GREATER, GREATER_EQ, LESS, LESS_EQ);
    }

    private Expr term() {
        return binaryOp(this::factor, MINUS, PLUS, BITAND, BITOR);
    }

    private Expr factor() {
        return binaryOp(this::unary, SLASH, STAR);
    }

    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token op = previous();
            Expr right = unary();
            return new Expr.Unary(op, right);
        }
        return call();
    }

    private Expr call() {
        Expr expr = primary();
        while (true) {
            if (match(LEFT_PAREN))
                expr = finishCall(expr);
            else if (match(DOT)) {
                Token name = consume(IDENT, "expected property name after '.'");
                expr = new Expr.Get(expr, name);
            } else
                break;
        }
        return expr;
    }

    private Expr finishCall(Expr callee) {
        List<Expr> args = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                if (args.size() >= 255)
                    error(peek(), "can't have more than 255 arguments");
                args.add(expression());
            } while (match(COMMA));
        }
        Token paren = consume(RIGHT_PAREN, "expected ')' after arguments");
        return new Expr.Call(callee, paren, args);
    }

    private Expr primary() {
        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE))  return new Expr.Literal(true);
        if (match(NIL))   return new Expr.Literal(null);
        if (match(THIS))  return new Expr.This(previous());
        if (match(IDENT)) return new Expr.Variable(previous());
        if (match(NUMBER, STRING))
            return new Expr.Literal(previous().literal);
        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "expected ')' after expression");
            return new Expr.Grouping(expr);
        }
        if (match(SUPER)) {
            Token keyword = previous();
            consume(DOT, "expected '.' after 'super'");
            Token method = consume(IDENT, "expected superclass method name");
            return new Expr.Super(keyword, method);
        }
        throw error(peek(), "expected primary expression");
    }

    private boolean match(Token.Type... types) {
        for (var type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    private Token consume(Token.Type type, String message) {
        if (check(type))
            return advance();
        throw error(peek(), message);
    }

    private boolean check(Token.Type type) {
        if (isAtEnd())
            return false;
        return peek().type == type;
    }

    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON)
                return;
            switch (peek().type) {
            case CLASS: case FUN:   case VAR:   case FOR:
            case IF:    case WHILE: case PRINT: case RETURN:
                return;
            }
            advance();
        }
    }

    private Token advance() {
        if (!isAtEnd())
            curr++;
        return previous();
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(curr);
    }

    private Token previous() {
        return tokens.get(curr - 1);
    }
}

