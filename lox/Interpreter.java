package lox;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.stream.Collectors;

class Interpreter implements Expr.Visitor<Object>,
                             Stmt.Visitor<Void> {
    private Environment env = new Environment();
    final Environment globals = env;
    private final Map<Expr, Integer> locals = new HashMap<>();

    public Interpreter() {
        globals.define("clock", new LoxCallable() {
            @Override
            public int arity() { return 0; }
            public Object call(Interpreter interpreter,
                               List<Object> arguments) {
                return (double)System.currentTimeMillis() / 1000.0;
            }
            @Override
            public String toString() { return "<native fn clock>"; }
        });
    }

    public void interpret(List<Stmt> statements) {
        try {
            for (var stmt : statements) {
                execute(stmt);
            }
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }

    @Override
    public Object visitLogicalExpr(Expr.Logical expr) {
        Object left = eval(expr.left);
        if (expr.operator.type == Token.Type.OR) {
            if (isTruthy(left))
                return left;
        } else {
            if (!isTruthy(left))
                return left;
        }
        return eval(expr.right);
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return eval(expr.expression);
    }

    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = eval(expr.right);

        switch (expr.operator.type) {
        case MINUS:
            checkNum(expr.operator, right);
            return - (double) right;
        case BANG:
            return !isTruthy(right);
        }

        // unreachable
        return null;
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left  = eval(expr.left);
        Object right = eval(expr.right);

        switch (expr.operator.type) {
        case MINUS: checkNums(expr.operator, left, right); return (double)left - (double)right;
        case SLASH: checkNums(expr.operator, left, right); return (double)left / (double)right;
        case STAR:  checkNums(expr.operator, left, right); return (double)left * (double)right;
        case PLUS:
            if (left instanceof Double && right instanceof Double)
                return (double)left + (double)right;
            if (left instanceof String && right instanceof String)
                return (String)left + (String)right;
            throw new RuntimeError(expr.operator, "operands must be two numbers or two strings");
        case BITAND:     checkNums(expr.operator, left, right); return doBitwiseOp(left, right, expr.operator.type);
        case BITOR:      checkNums(expr.operator, left, right); return doBitwiseOp(left, right, expr.operator.type);
        case GREATER:    checkNums(expr.operator, left, right); return (double)left >  (double)right;
        case GREATER_EQ: checkNums(expr.operator, left, right); return (double)left >= (double)right;
        case LESS:       checkNums(expr.operator, left, right); return (double)left <  (double)right;
        case LESS_EQ:    checkNums(expr.operator, left, right); return (double)left <= (double)right;
        case BANG_EQ:    return !isEqual(left, right);
        case EQ_EQ:      return  isEqual(left, right);
        case COMMA:      return right;
        }

        // unreachable
        return null;
    }

    @Override
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = eval(expr.callee);
        var args = expr.arguments.stream()
                            .map(a -> eval(a))
                            .collect(Collectors.toList());
        if (!(callee instanceof LoxCallable))
            throw new RuntimeError(expr.paren, "can only call functions and classes");
        LoxCallable function = (LoxCallable)callee;
        if (args.size() != function.arity())
            throw new RuntimeError(expr.paren, "expected " + function.arity() + "arguments, but got " + args.size());
        return function.call(this, args);
    }

    @Override
    public Object visitGetExpr(Expr.Get expr) {
        Object obj = eval(expr.object);
        if (obj instanceof LoxInstance)
            return ((LoxInstance) obj).get(expr.name);
        throw new RuntimeError(expr.name, "only instances have properties");
    }

    @Override
    public Object visitSetExpr(Expr.Set expr) {
        Object obj = eval(expr.object);
        if (!(obj instanceof LoxInstance))
            throw new RuntimeError(expr.name, "only instances have fields.");
        Object value = eval(expr.value);
        ((LoxInstance) obj).set(expr.name, value);
        return value;
    }

    @Override
    public Object visitVariableExpr(Expr.Variable expr) {
        return lookupVariable(expr.name, expr);
    }

    private Object lookupVariable(Token name, Expr expr) {
        Integer dist = locals.get(expr);
        if (dist != null)
            return env.getAt(dist, name.lexeme);
        else
            return globals.get(name);
    }

    @Override
    public Object visitAssignExpr(Expr.Assign expr) {
        Object value = eval(expr.value);
        Integer dist = locals.get(expr);
        if (dist != null)
            env.assignAt(dist, expr.name, value);
        else
            globals.assign(expr.name, value);
        return value;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        eval(stmt.expression);
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        LoxFunction function = new LoxFunction(stmt, env);
        env.define(stmt.name.lexeme, function);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        if (isTruthy(eval(stmt.cond)))
            execute(stmt.thenBranch);
        else if (stmt.elseBranch != null)
            execute(stmt.elseBranch);
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        Object value = eval(stmt.expression);
        System.out.println(stringify(value));
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = stmt.value == null ? null : eval(stmt.value);
        throw new Return(value);
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        Object value = stmt.initializer == null ? null
                                                : eval(stmt.initializer);
        env.define(stmt.name.lexeme, value);
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        while (isTruthy(eval(stmt.cond))) {
            execute(stmt.body);
        }
        return null;
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        execBlock(stmt.statements, new Environment(env));
        return null;
    }

    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        env.define(stmt.name.lexeme, null);
        LoxClass klass = new LoxClass(stmt.name.lexeme);
        env.assign(stmt.name, klass);
        return null;
    }

    private void execute(Stmt stmt) {
        stmt.accept(this);
    }

    void resolve(Expr expr, int depth) {
        locals.put(expr, depth);
    }

    void execBlock(List<Stmt> statements, Environment env) {
        Environment prev = this.env;
        try {
            this.env = env;
            for (var stmt : statements)
                execute(stmt);
        } finally {
            this.env = prev;
        }
    }

    private Object eval(Expr expr) {
        return expr.accept(this);
    }

    private boolean isTruthy(Object object) {
        if (object == null)
            return false;
        if (object instanceof Boolean)
            return (boolean) object;
        return true;
    }

    private boolean isEqual(Object a, Object b) {
        if (a == null)
            return b == null;
        return a.equals(b);
    }

    private String stringify(Object object) {
        if (object == null)
            return "nil";
        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }
        return object.toString();
    }

    private void checkNum(Token operator, Object operand) {
        if (operand instanceof Double)
            return;
        throw new RuntimeError(operator, "operand must be a number");
    }

    private void checkNums(Token operator, Object left, Object right) {
        if (left instanceof Double && right instanceof Double)
            return;
        throw new RuntimeError(operator, "operands must be numbers.");
    }

    private Object doBitwiseOp(Object left, Object right, Token.Type op)
    {
        double ln = (double)left, rn = (double)right;
        int ln2 = (int)ln, rn2 = (int)rn;
        switch (op) {
        case BITAND: return (double)(ln2 & rn2);
        case BITOR:  return (double)(ln2 | rn2);
        default:     return (double)0.0;
        }
    }
}
