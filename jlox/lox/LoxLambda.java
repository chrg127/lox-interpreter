package lox;

class LoxLambda implements LoxCallable {
    private final List<Token> params;
    private final List<Stmt> body;
    private final Environment closure;
    private final String name = "";
    private final boolean isCtor;

    public LoxFunction(List<Token> params, List<Stmt> body, Environment closure, boolean isCtor) {
        this.params = params;
        this.body = body;
        this.closure = closure;
        this.isCtor = isCtor;
    }

    public LoxFunction(String name, List<Token> params, List<Stmt> body, Environment closure, boolean isCtor) {
        LoxFunction(params, body, closure);
        this.name = name;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment env = new Environment(closure);
        for (int i = 0; i < params.size(); i++)
            env.define(params.get(i).lexeme, arguments.get(i));
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
        return params.size();
    }

    LoxFunction bind(LoxInstance instance) {
        Environment env = new Environment(closure);
        env.define("this", instance);
        return new LoxFunction(decl, env, isCtor);
    }

    @Override
    public String toString() {
        return name.isEmpty() ? "<lambda>" : "<fn " + name + ">";
    }
}
