package lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final Stmt.Function decl;
    private final Environment closure;
    private final boolean isCtor;

    LoxFunction(Stmt.Function decl, Environment closure,
                boolean isCtor) {
        this.decl = decl;
        this.closure = closure;
        this.isCtor = isCtor;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment env = new Environment(closure);
        for (int i = 0; i < decl.params.size(); i++)
            env.define(decl.params.get(i).lexeme, arguments.get(i));
        try {
            interpreter.execBlock(decl.body, env);
        } catch (Return retval) {
            if (isCtor)
                return closure.getAt(0, "this");
            return retval.value;
        }
        if (isCtor)
            return closure.getAt(0, "this");
        return null;
    }

    @Override
    public int arity() {
        return decl.params.size();
    }

    LoxFunction bind(LoxInstance instance) {
        Environment env = new Environment(closure);
        env.define("this", instance);
        return new LoxFunction(decl, env, isCtor);
    }

    @Override
    public String toString() {
        return "<fn " + decl.name.lexeme + ">";
    }
}
