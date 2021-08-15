package lox;

import java.util.List;

abstract class Stmt {
    interface Visitor<T> {
        T visitClassStmt(Class stmt);
        T visitFunctionStmt(Function stmt);
        T visitVarStmt(Var stmt);
        T visitExpressionStmt(Expression stmt);
        T visitIfStmt(If stmt);
        T visitPrintStmt(Print stmt);
        T visitReturnStmt(Return stmt);
        T visitWhileStmt(While stmt);
        T visitBreakStmt(Break stmt);
        T visitBlockStmt(Block stmt);
    }

    static class Class extends Stmt {
        final Token name;
        final Expr.Variable superclass;
        final List<Stmt.Function> methods;
        final List<Stmt.Function> statics;

        Class(Token name, Expr.Variable superclass, List<Stmt.Function> methods, List<Stmt.Function> statics) {
            this.name = name;
            this.superclass = superclass;
            this.methods = methods;
            this.statics = statics;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitClassStmt(this);
        }
    }

    static class Function extends Stmt {
        final Token name;
        final List<Token> params;
        final List<Stmt> body;

        Function(Token name, List<Token> params, List<Stmt> body) {
            this.name = name;
            this.params = params;
            this.body = body;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitFunctionStmt(this);
        }
    }

    static class Var extends Stmt {
        final Token name;
        final Expr initializer;

        Var(Token name, Expr initializer) {
            this.name = name;
            this.initializer = initializer;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitVarStmt(this);
        }
    }

    static class Expression extends Stmt {
        final Expr expression;

        Expression(Expr expression) {
            this.expression = expression;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitExpressionStmt(this);
        }
    }

    static class If extends Stmt {
        final Expr cond;
        final Stmt thenBranch;
        final Stmt elseBranch;

        If(Expr cond, Stmt thenBranch, Stmt elseBranch) {
            this.cond = cond;
            this.thenBranch = thenBranch;
            this.elseBranch = elseBranch;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitIfStmt(this);
        }
    }

    static class Print extends Stmt {
        final Expr expression;

        Print(Expr expression) {
            this.expression = expression;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitPrintStmt(this);
        }
    }

    static class Return extends Stmt {
        final Token keyword;
        final Expr value;

        Return(Token keyword, Expr value) {
            this.keyword = keyword;
            this.value = value;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitReturnStmt(this);
        }
    }

    static class While extends Stmt {
        final Expr cond;
        final Stmt body;

        While(Expr cond, Stmt body) {
            this.cond = cond;
            this.body = body;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitWhileStmt(this);
        }
    }

    static class Break extends Stmt {
        final Token keyword;

        Break(Token keyword) {
            this.keyword = keyword;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitBreakStmt(this);
        }
    }

    static class Block extends Stmt {
        final List<Stmt> statements;

        Block(List<Stmt> statements) {
            this.statements = statements;
        }

        @Override
        <T> T accept(Visitor<T> visitor) {
            return visitor.visitBlockStmt(this);
        }
    }


    abstract <T> T accept(Visitor<T> visitor);
}
