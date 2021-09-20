package lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private static class Variable {
        boolean declared;
        int index;
        Variable(boolean declared, int index) {
            this.declared = declared;
            this.index = index;
        }
    }

    private static class Scope {
        Map<String, Variable> vars = new HashMap<>();
        int index = 0;

        void putVar(String name, boolean declared) {
            vars.put(name, new Variable(declared, index));
            index++;
        }

        void updateVar(String name, boolean declared) {
            int index = vars.get(name).index;
            vars.put(name, new Variable(declared, index));
        }
    }

    private enum FunctionType { NONE, FUNCTION, CTOR, METHOD, GETTER, STATIC }
    private enum ClassType    { NONE, CLASS, SUBCLASS }

    private final Interpreter interpreter;
    private final Stack<Scope> scopes = new Stack<>();
    private FunctionType currFunc = FunctionType.NONE;
    private ClassType currClass = ClassType.NONE;
    // used to keep track of bad 'break' statements
    private boolean insideWhile = false;
    // used to keep track of any unused variables
    private final Map<String, Token> unused = new HashMap<>();

    public Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    private void resolve(Stmt stmt) { stmt.accept(this); }
    private void resolve(Expr expr) { expr.accept(this); }

    public void resolve(List<Stmt> statements) {
        for (var stmt : statements)
            resolve(stmt);
    }

    private void beginScope() { scopes.push(new Scope()); }
    private void endScope()   { scopes.pop(); }

    private void declare(Token name) {
        if (scopes.isEmpty())
            return;
        if (scopes.peek().vars.containsKey(name.lexeme)) {
            Lox.error(name, "redefinition of variable " + name.lexeme + " in the same scope");
            return;
        }
        scopes.peek().putVar(name.lexeme, false);
        unused.put(name.lexeme, name);
    }

    private void define(Token name) {
        if (scopes.isEmpty())
            return;
        scopes.peek().updateVar(name.lexeme, true);
    }

    private void resolveLocal(Expr expr, Token name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            var scope = scopes.get(i);
            if (scope.vars.containsKey(name.lexeme)) {
                interpreter.resolve(
                    expr,
                    scopes.size() - 1 - i,
                    scope.vars.get(name.lexeme).index
                );
                return;
            }
        }
    }

    private void resolveFunction(List<Token> params, List<Stmt> body, FunctionType type) {
        FunctionType enclosing = currFunc;
        currFunc = type;
        boolean prevWhile = insideWhile;
        insideWhile = false;
        beginScope();
        for (var param : params) {
            declare(param);
            define(param);
        }
        resolve(body);
        endScope();
        currFunc = enclosing;
        insideWhile = prevWhile;
    }

    public void printUnusedVariables() {
        for (var variable : unused.values())
            Lox.warning(variable.line, "unused variable '" + variable.lexeme + "'");
    }

    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        ClassType enclosing = currClass;
        currClass = ClassType.CLASS;
        boolean prevWhile = insideWhile;
        insideWhile = false;
        declare(stmt.name);
        define(stmt.name);

        // resolve static functions here, before any scope created for 'super' and 'this'
        // TODO: resolve bug with global variables
        for (var s : stmt.statics)
            resolveFunction(s.params, s.body, FunctionType.STATIC);

        if (stmt.superclass != null) {
            if (stmt.name.lexeme.equals(stmt.superclass.name.lexeme))
                Lox.error(stmt.superclass.name, "can't inherit from itself");
            currClass = ClassType.SUBCLASS;
            resolve(stmt.superclass);
            beginScope();
            scopes.peek().putVar("super", true);
        }

        beginScope();
        scopes.peek().putVar("this", true);
        for (var method : stmt.methods) {
            FunctionType decl = method.name.lexeme.equals("init") ?  FunctionType.CTOR : FunctionType.METHOD;
            resolveFunction(method.params, method.body, decl);
        }
        for (var getter : stmt.getters)
            resolveFunction(getter.params, getter.body, FunctionType.GETTER);
        endScope();

        if (stmt.superclass != null)
            endScope();
        currClass = enclosing;
        insideWhile = prevWhile;
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        declare(stmt.name);
        define(stmt.name);
        resolveFunction(stmt.params, stmt.body, FunctionType.FUNCTION);
        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        declare(stmt.name);
        if (stmt.initializer != null)
            resolve(stmt.initializer);
        define(stmt.name);
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        resolve(stmt.cond);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null)
            resolve(stmt.elseBranch);
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        if (currFunc == FunctionType.NONE)
            Lox.error(stmt.keyword, "invalid return statement on global scope");
        if (stmt.value != null) {
            if (currFunc == FunctionType.CTOR)
                Lox.error(stmt.keyword, "can't return value from constructor");
            resolve(stmt.value);
        }
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        boolean prevWhile = insideWhile;
        insideWhile = true;
        resolve(stmt.cond);
        resolve(stmt.body);
        insideWhile = prevWhile;
        return null;
    }

    @Override
    public Void visitBreakStmt(Stmt.Break stmt) {
        if (!insideWhile)
            Lox.error(stmt.keyword, "break statement outside looping construct");
        return null;
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    @Override
    public Void visitBinaryExpr(Expr.Binary expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitAssignExpr(Expr.Assign expr) {
        resolve(expr.value);
        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitLogicalExpr(Expr.Logical expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitUnaryExpr(Expr.Unary expr) {
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitCallExpr(Expr.Call expr) {
        resolve(expr.callee);
        for (var arg : expr.arguments)
            resolve(arg);
        return null;
    }

    @Override
    public Void visitLambdaExpr(Expr.Lambda expr) {
        resolveFunction(expr.params, expr.body, FunctionType.FUNCTION);
        return null;
    }

    @Override
    public Void visitGetExpr(Expr.Get expr) {
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitSetExpr(Expr.Set expr) {
        resolve(expr.value);
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitVariableExpr(Expr.Variable expr) {
        if (!scopes.isEmpty()) {
            var variable = scopes.peek().vars.get(expr.name.lexeme);
            if (variable != null && variable.declared == false)
                Lox.error(expr.name, "can't read local variable in its own initializer");
        }
        unused.remove(expr.name.lexeme);
        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitThisExpr(Expr.This expr) {
        if (currClass == ClassType.NONE)
            Lox.error(expr.keyword, "can't use 'this' outside a class");
        resolveLocal(expr, expr.keyword);
        return null;
    }

    @Override
    public Void visitSuperExpr(Expr.Super expr) {
        if (currClass == ClassType.NONE)
            Lox.error(expr.keyword, "can't use 'super' outside class");
        else if (currClass != ClassType.SUBCLASS)
            Lox.error(expr.keyword, "can't use 'super' in a class without superclass");
        resolveLocal(expr, expr.keyword);
        return null;
    }

    @Override
    public Void visitGroupingExpr(Expr.Grouping expr) {
        resolve(expr.expression);
        return null;
    }

    @Override
    public Void visitLiteralExpr(Expr.Literal expr) {
        return null;
    }
}
