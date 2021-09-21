package lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final List<Token> params;
    private final List<Stmt> body;
    private final Environment closure;
    private String name;
    private final boolean isCtor;

    public LoxFunction(List<Token> params, List<Stmt> body, Environment closure, boolean isCtor) {
        this.params = params;
        this.body = body;
        this.closure = closure;
        this.isCtor = isCtor;
        this.name = "";
    }

    public LoxFunction(Stmt.Function decl, Environment closure, boolean isCtor) {
        this(decl.params, decl.body, closure, isCtor);
        this.name = decl.name.lexeme;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment env = new Environment(closure);
        for (int i = 0; i < params.size(); i++)
            env.define(params.get(i).lexeme, arguments.get(i));
        try {
            interpreter.execBlock(body, env);
        } catch (Return retval) {
            if (isCtor) {
                assert closure != null;
                return closure.getByNameAt(0, "this");
            }
            return retval.value;
        }
        if (isCtor) {
            assert closure != null;
            return closure.getByNameAt(0, "this");
        }
        return null;
    }

    @Override
    public int arity() {
        return params.size();
    }

    LoxFunction bind(LoxInstance instance) {
        Environment env = new Environment(closure);
        env.define("this", instance);
        return new LoxFunction(params, body, env, isCtor);
    }

    @Override
    public String toString() {
        return "<fn " + (name.isEmpty() ? "<lambda>" : name) + ">";
    }
}
