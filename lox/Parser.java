package lox;

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
            if (match(VAR))
                return varDecl();
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

    private Stmt stmt() {
        if (match(PRINT))
            return printStmt();
        return expressionStmt();
    }

    private Stmt printStmt() {
        Expr value = expression();
        consume(SEMICOLON, "expected ';' after expression");
        return new Stmt.Print(value);
    }

    private Stmt expressionStmt() {
        Expr expr = expression();
        consume(SEMICOLON, "expected ';' after expression");
        return new Stmt.Expression(expr);
    }

    private Expr expression() {
        return comma();
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
        return binaryOp(this::ternary, COMMA);
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
        return primary();
    }

    private Expr primary() {
        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE))  return new Expr.Literal(true);
        if (match(NIL))   return new Expr.Literal(null);
        if (match(NUMBER, STRING))
            return new Expr.Literal(previous().literal);
        if (match(IDENT))
            return new Expr.Variable(previous());
        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "expected ')' after expression");
            return new Expr.Grouping(expr);
        }
        throw error(peek(), "expected primary expression");
    }

    private Expr ternary() {
        Expr expr = equality();
        if (match(QMARK)) {
            Token op = previous();
            Expr left = expression();
            if (!match(DCOLON))
                throw error(peek(), "expected ':' operator");
            Expr right = equality();
            return new Expr.Ternary(op, expr, left, right);
        }
        return expr;
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

