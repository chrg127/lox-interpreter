package lox;

import java.util.List;
import java.util.function.Function;

class ASTPrinter implements Expr.Visitor<String>, Stmt.Visitor<String> {
    private Function<Expr, String> EXPR = e -> printExpr(e);
    private Function<Stmt, String> STMT = s -> printStmt(s);
    private Function<Token, String> TOKEN = t -> t.lexeme;

    public String printStmt(Stmt stmt) { return stmt.accept(this); }
    public String printExpr(Expr expr) { return expr.accept(this); }

    private <T> String parenthesize(String name, Function<T, String> fn, T... ts) {
        var builder = new StringBuilder();
        builder.append("(");
        if (!name.isEmpty())
            builder.append(name);
        for (var t : ts) {
            String s = fn.apply(t);
            if (s.isEmpty())
                continue;
            builder.append(" ").append(s);
        }
        return builder.append(")").toString();
    }

    private <T> String parenthesizeList(String name, Function<T, String> fn, List<T> ts) {
        @SuppressWarnings("unchecked")
        T[] arr = (T[]) new Object[0];
        return parenthesize(name, fn, ts.toArray(arr));
    }



    // statements
    @Override
    public String visitClassStmt(Stmt.Class stmt) {
        return parenthesize("class " + stmt.name.lexeme, a -> a,
                stmt.superclass == null ? "" : parenthesize("superclass", EXPR, stmt.superclass),
                parenthesizeList("statics", e -> e.accept(this), stmt.statics),
                parenthesizeList("methods", e -> e.accept(this), stmt.methods),
                parenthesizeList("getters", e -> e.accept(this), stmt.getters));
    }

    @Override
    public String visitFunctionStmt(Stmt.Function stmt) {
        return parenthesize("fun " + stmt.name.lexeme, a -> a,
                parenthesizeList("args", TOKEN, stmt.params),
                parenthesizeList("body", STMT, stmt.body));
    }

    @Override public String visitVarStmt(Stmt.Var stmt)               { return parenthesize("var " + stmt.name.lexeme, EXPR, stmt.initializer); }
    @Override public String visitExpressionStmt(Stmt.Expression stmt) { return parenthesize("expr-stmt", EXPR, stmt.expression); }

    @Override
    public String visitIfStmt(Stmt.If stmt) {
        var builder = new StringBuilder();
        builder.append("(if ").append(printExpr(stmt.cond)).append(" ").append(printStmt(stmt.thenBranch));
        if (stmt.elseBranch != null)
            builder.append(" ").append(stmt.elseBranch);
        return builder.append(")").toString();
    }

    @Override public String visitPrintStmt(Stmt.Print stmt)           { return parenthesize("print", EXPR, stmt.expression); }
    @Override public String visitReturnStmt(Stmt.Return stmt)         { return parenthesize("return", EXPR, stmt.value); }
    @Override public String visitWhileStmt(Stmt.While stmt)           { return "(while " + printExpr(stmt.cond) + " " + printStmt(stmt.body) + ")"; }
    @Override public String visitBreakStmt(Stmt.Break stmt)           { return "(break)"; }
    @Override public String visitBlockStmt(Stmt.Block block)          { return parenthesizeList("{}", STMT, block.statements); }

    // expressions
    @Override public String visitBinaryExpr(Expr.Binary expr)         { return parenthesize(expr.operator.lexeme, EXPR, expr.left, expr.right); }
    @Override public String visitAssignExpr(Expr.Assign expr)         { return parenthesize("= " + expr.name.lexeme, EXPR, expr.value); }
    @Override public String visitLogicalExpr(Expr.Logical expr)       { return parenthesize(expr.operator.lexeme, EXPR, expr.left, expr.right); }
    @Override public String visitUnaryExpr(Expr.Unary expr)           { return parenthesize(expr.operator.lexeme, EXPR, expr.right); }
    @Override public String visitCallExpr(Expr.Call expr)             { return parenthesizeList(printExpr(expr.callee), EXPR, expr.arguments); }

    @Override
    public String visitLambdaExpr(Expr.Lambda expr) {
        return parenthesize("lambda", a -> a,
                parenthesizeList("args", TOKEN, expr.params),
                parenthesizeList("body", STMT, expr.body));
    }

    @Override public String visitGetExpr(Expr.Get expr)               { return parenthesize("get " + expr.name.lexeme, EXPR, expr.object); }
    @Override public String visitSetExpr(Expr.Set expr)               { return parenthesize("set " + expr.name.lexeme, EXPR, expr.object, expr.value); }
    @Override public String visitVariableExpr(Expr.Variable expr)     { return expr.name.lexeme; }
    @Override public String visitThisExpr(Expr.This expr)             { return "this"; }
    @Override public String visitSuperExpr(Expr.Super expr)           { return "(super " + expr.method.lexeme + ")"; }
    @Override public String visitGroupingExpr(Expr.Grouping expr)     { return parenthesize("group", EXPR, expr.expression); }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) return "nil";
        if (expr.value instanceof String) return "\"" + expr.value.toString() + "\"";
        return expr.value.toString();
    }
}
