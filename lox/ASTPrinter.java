package lox;

import java.util.List;

class ASTPrinter implements Expr.Visitor<String>,
                            Stmt.Visitor<String> {
    String run(Stmt stmt) {
        return stmt.accept(this);
    }

    String print(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitCallExpr(Expr.Call expr) {
        var builder = new StringBuilder();
        builder.append("(").append(expr.callee.accept(this));
        for (var e : expr.arguments) {
            builder.append(" ");
            builder.append(e.accept(this));
        }
        builder.append(")");
        return builder.toString();
    }

    @Override
    public String visitGetExpr(Expr.Get expr) {
        return parenthesize("(get " + expr.name.lexeme + " ", expr.object);
    }

    @Override
    public String visitSetExpr(Expr.Set expr) {
        return parenthesize("(set " + expr.name.lexeme + " ", expr.object,
                expr.value);
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null)
            return "nil";
        return expr.value.toString();
    }

    @Override
    public String visitThisExpr(Expr.This expr) {
        return "this";
    }

    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    @Override
    public String visitVariableExpr(Expr.Variable expr) {
        return expr.name.lexeme;
    }

    @Override
    public String visitAssignExpr(Expr.Assign expr) {
        return parenthesize("= " + expr.name.lexeme, expr.value);
    }

    @Override
    public String visitLogicalExpr(Expr.Logical expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitExpressionStmt(Stmt.Expression stmt) {
        return parenthesize("expr", stmt.expression);
    }

    @Override
    public String visitFunctionStmt(Stmt.Function stmt) {
        var builder = new StringBuilder();
        builder.append("(fun ").append(stmt.name.lexeme).append(" (args ");
        for (var param : stmt.params)
            builder.append(param.lexeme).append(" ");
        builder.append(") (body ");
        for (var bodystmt : stmt.body)
            builder.append(bodystmt.accept(this)).append(" ");
        builder.append("))");
        return builder.toString();
    }

    @Override
    public String visitPrintStmt(Stmt.Print stmt) {
        return parenthesize("print", stmt.expression);
    }

    @Override
    public String visitReturnStmt(Stmt.Return stmt) {
        return parenthesize("return", stmt.value);
    }

    @Override
    public String visitVarStmt(Stmt.Var stmt) {
        return parenthesize("vardecl " + stmt.name.lexeme, stmt.initializer);
    }

    @Override
    public String visitBlockStmt(Stmt.Block block) {
        var builder = new StringBuilder();
        builder.append("(block ");
        for (var stmt : block.statements)
            builder.append(stmt.accept(this)).append(" ");
        return builder.append(")").toString();
    }

    @Override
    public String visitClassStmt(Stmt.Class stmt) {
        var builder = new StringBuilder();
        builder.append("(class ");
        for (var f : stmt.methods)
            builder.append(f.accept(this)).append(" ");
        return builder.append(")").toString();
    }

    @Override
    public String visitIfStmt(Stmt.If stmt) {
        String thenBranch = run(stmt.thenBranch);
        String elseBranch = run(stmt.elseBranch);
        String cond = print(stmt.cond);
        return "(if " + cond + " " + thenBranch + " " + elseBranch + ")";
    }

    @Override
    public String visitWhileStmt(Stmt.While stmt) {
        return "(while " + print(stmt.cond) + " " + run(stmt.body) + ")";
    }

    private String parenthesize(String name, Expr... exprs) {
        var builder = new StringBuilder();
        builder.append("(").append(name);
        for (var expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this));
        }
        builder.append(")");
        return builder.toString();
    }
}
