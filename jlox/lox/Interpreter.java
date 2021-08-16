package lox;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.stream.Collectors;

class Interpreter implements Expr.Visitor<Object>,
                             Stmt.Visitor<Void> {
    private Environment env = new Environment();
    private Resolver resolver;

    static class LocalInfo {
        int dist, index;
        LocalInfo(int dist, int index) { this.dist = dist; this.index = index; }
    }

    private final Map<Expr, LocalInfo> vars = new HashMap<>();

    public Interpreter() {
        // at this point, env must be the global environment
        env.define("clock", new LoxCallable() {
            @Override
            public int arity() { return 0; }
            public Object call(Interpreter interpreter, List<Object> arguments) {
                return (double)System.currentTimeMillis() / 1000.0;
            }
            @Override
            public String toString() { return "<native fn clock>"; }
        });
        var builtins = new ArrayList<String>();
        builtins.add("clock");
        resolver = new Resolver(this, builtins);
    }

    public Resolver getResolver() {
        return resolver;
    }

    public void interpret(List<Stmt> statements) {
        try {
            for (var stmt : statements)
                exec(stmt);
        } catch (Break brk) {
            Lox.runtimeError(brk.keyword, "'break' outside of looping construct");
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    public String interpretOne(Stmt.Expression stmt) {
        try {
            return stringify(eval(stmt.expression));
        } catch (Break brk) {
            Lox.runtimeError(brk.keyword, "'break' outside of looping construct");
            return "nil";
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
            return "nil";
        }
    }

    public void resolve(Expr expr, int depth, int index) {
        vars.put(expr, new LocalInfo(depth, index));
    }

    public void execBlock(List<Stmt> statements, Environment env) {
        Environment prev = this.env;
        try {
            this.env = env;
            for (var stmt : statements)
                exec(stmt);
        } finally {
            this.env = prev;
        }
    }

    private void exec(Stmt stmt) {
        stmt.accept(this);
    }

    public Object eval(Expr expr) {
        return expr.accept(this);
    }

    private boolean isTruthy(Object object) {
        return object == null ? false : object instanceof Boolean ? (boolean) object : true;
    }

    private boolean isEqual(Object a, Object b) {
        return a == null ? b == null : a.equals(b);
    }

    private String stringify(Object object) {
        if (object == null)
            return "nil";
        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0"))
                text = text.substring(0, text.length() - 2);
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

    private Object doBitwiseOp(Object left, Object right, Token.Type op) {
        double ln = (double)left, rn = (double)right;
        int ln2 = (int)ln, rn2 = (int)rn;
        switch (op) {
        case BITAND: return (double)(ln2 & rn2);
        case BITOR:  return (double)(ln2 | rn2);
        default:     return (double)0.0;
        }
    }

    private LocalInfo lookupVariableInfo(Token name, Expr expr) {
        var info = vars.get(expr);
        if (info == null)
            throw new RuntimeError(name, "undefined variable '" + name.lexeme + "'");
        return info;
    }

    private Object lookupVariable(Token name, Expr expr) {
        var info = lookupVariableInfo(name, expr);
        return env.getAt(info.dist, info.index, name);
    }

    @Override
    public Void visitClassStmt(Stmt.Class stmt) {
        Object superclass = null;
        if (stmt.superclass != null) {
            superclass = eval(stmt.superclass);
            if (!(superclass instanceof LoxClass))
                throw new RuntimeError(stmt.superclass.name, "superclass must be a class");
        }

        int classIndex = env.declare(stmt.name.lexeme);
        if (stmt.superclass != null) {
            env = new Environment(env);
            env.define("super", superclass);
        }

        var methods = stmt.methods.stream().collect(Collectors.toMap(
                    m -> m.name.lexeme,
                    m -> new LoxFunction(m, env, m.name.lexeme.equals("init"))));

        var statics = stmt.statics.stream().collect(Collectors.toMap(
                    s -> s.name.lexeme,
                    s -> new LoxFunction(s, env, false)));

        var getters = stmt.getters.stream().collect(Collectors.toMap(
                    g -> g.name.lexeme,
                    g -> new LoxFunction(g, env, false)));

        LoxClass klass = new LoxClass(stmt.name.lexeme, (LoxClass) superclass, methods, statics, getters);
        if (superclass != null)
            env = env.enclosing;

        env.assign(classIndex, klass);
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        LoxFunction function = new LoxFunction(stmt, env, false);
        env.define(stmt.name.lexeme, function);
        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        int varIndex = env.declare(stmt.name.lexeme);
        if (stmt.initializer != null)
            env.assign(varIndex, eval(stmt.initializer));
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        eval(stmt.expression);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        if (isTruthy(eval(stmt.cond)))
            exec(stmt.thenBranch);
        else if (stmt.elseBranch != null)
            exec(stmt.elseBranch);
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
    public Void visitWhileStmt(Stmt.While stmt) {
        while (isTruthy(eval(stmt.cond))) {
            try {
                exec(stmt.body);
            } catch (Break brk) {
                return null;
            }
        }
        return null;
    }

    @Override
    public Void visitBreakStmt(Stmt.Break stmt) {
        throw new Break(stmt.keyword);
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        execBlock(stmt.statements, new Environment(env));
        return null;
    }



    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left  = eval(expr.left);
        Object right = eval(expr.right);
        switch (expr.operator.type) {
        case PLUS:
            if (left instanceof Double && right instanceof Double)
                return (double)left + (double)right;
            if (left instanceof String && right instanceof String)
                return (String)left + (String)right;
            throw new RuntimeError(expr.operator, "operands must be two numbers or two strings");
        case MINUS: checkNums(expr.operator, left, right); return (double)left - (double)right;
        case STAR:  checkNums(expr.operator, left, right); return (double)left * (double)right;
        case SLASH:
            checkNums(expr.operator, left, right);
            if ( ((double)right) == 0.0)
                throw new RuntimeError(expr.operator, "dividing by zero");
            return (double)left / (double)right;
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
        return null; // unreachable
    }

    @Override
    public Object visitAssignExpr(Expr.Assign expr) {
        var info = lookupVariableInfo(expr.name, expr);
        Object value = eval(expr.value);
        env.assignAt(info.dist, info.index, value);
        return value;
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
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = eval(expr.right);
        switch (expr.operator.type) {
        case MINUS:
            checkNum(expr.operator, right);
            return - (double) right;
        case BANG:
            return !isTruthy(right);
        }
        return null; // unreachable
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
            throw new RuntimeError(expr.paren, "expected " + function.arity() +
                                               " " + (function.arity() == 1 ? "argument" : "arguments") +
                                               " but got " + args.size());
        return function.call(this, args);
    }

    @Override
    public Object visitLambdaExpr(Expr.Lambda expr) {
        return new LoxFunction(expr.params, expr.body, env, false);
    }

    @Override
    public Object visitGetExpr(Expr.Get expr) {
        Object obj = eval(expr.object);
        if (obj instanceof LoxInstance)
            return ((LoxInstance) obj).get(expr.name, this);
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

    @Override
    public Object visitThisExpr(Expr.This expr) {
        return lookupVariable(expr.keyword, expr);
    }

    @Override
    public Object visitSuperExpr(Expr.Super expr) {
        var info = lookupVariableInfo(expr.keyword, expr);
        LoxClass superclass = (LoxClass) env.getAt(info.dist, info.index, expr.keyword);
        LoxInstance obj = (LoxInstance) env.getByNameAt(info.dist - 1, "this");
        LoxFunction method = superclass.findMethod(expr.method.lexeme);
        if (method == null)
            throw new RuntimeError(expr.method, "undefined property '" + expr.method.lexeme + "'");
        return method.bind(obj);
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return eval(expr.expression);
    }

    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }
}
