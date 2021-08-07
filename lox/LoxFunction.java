package lox;

import java.util.List;

class LoxFunction implements LoxCallable {
    private final Stmt.Function decl;
    private final Environment closure;

    LoxFunction(Stmt.Function decl, Environment closure) {
        this.decl = decl;
        this.closure = closure;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment env = new Environment(closure);
        for (int i = 0; i < decl.params.size(); i++)
            env.define(decl.params.get(i).lexeme, arguments.get(i));
        try {
            interpreter.execBlock(decl.body, env);
        } catch (Return retval) {
            return retval.value;
        }
        return null;
    }

    @Override
    public int arity() {
        return decl.params.size();
    }

    @Override
    public String toString() {
        return "<fn " + decl.name.lexeme + ">";
    }
}
