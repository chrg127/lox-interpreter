package lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
    private LoxClass klass;
    private final Map<String, Object> fields = new HashMap<>();

    LoxInstance(LoxClass klass) {
        this.klass = klass;
    }

    Object get(Token name, Interpreter interpreter) {
        if (klass.hasGetter(name.lexeme))
            return klass.getGetterResult(name.lexeme, interpreter, this);
        if (fields.containsKey(name.lexeme))
            return fields.get(name.lexeme);
        LoxFunction method = klass.findMethod(name.lexeme);
        if (method != null)
            return method.bind(this);
        throw new RuntimeError(name, "undefined property '" + name.lexeme + "'");
    }

    void set(Token name, Object value) {
        if (klass.hasGetter(name.lexeme))
            throw new RuntimeError(name, "can't set a value to getter '" + name.lexeme + "'");
        fields.put(name.lexeme, value);
    }

    @Override
    public String toString() {
        return klass.name + " instance";
    }
}
