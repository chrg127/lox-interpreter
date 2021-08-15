package lox;

import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
    final String name;
    final LoxClass superclass;
    private final Map<String, LoxFunction> methods;

    LoxClass(String name, LoxClass superclass, Map<String, LoxFunction> methods, Map<String, LoxFunction> statics) {
        super(new LoxClass(name, statics));
        this.name = name;
        this.superclass = superclass;
        this.methods = methods;
    }

    // this ctor creates a metaclass.
    LoxClass(String name, Map<String, LoxFunction> methods) {
        super(null);
        this.name = name;
        this.superclass = null;
        this.methods = methods;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> args) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction ctor = findMethod("init");
        if (ctor != null)
            ctor.bind(instance).call(interpreter, args);
        return instance;
    }

    @Override
    public int arity() {
        LoxFunction ctor = findMethod("init");
        return ctor == null ? 0 : ctor.arity();
    }

    LoxFunction findMethod(String name) {
        if (methods.containsKey(name))
            return methods.get(name);
        if (superclass != null)
            return superclass.findMethod(name);
        return null;
    }

    @Override
    public String toString() {
        return name;
    }
}
