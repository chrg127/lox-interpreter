package lox;

import java.util.HashMap;
import java.util.Map;

class Environment {
    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    public Environment() {
        enclosing = null;
    }

    public Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    void define(String name, Object value) {
        values.put(name, value);
    }

    Object get(Token name) {
        if (values.containsKey(name.lexeme))
            return values.get(name.lexeme);
        else if (enclosing == null)
            throw new RuntimeError(name,
                    "undefined variable '" + name.lexeme + "'.");
        return enclosing.get(name);
    }

    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }
        if (enclosing == null)
            throw new RuntimeError(name,
                    "undefined variable '" + name.lexeme + "'");
        enclosing.assign(name, value);
    }
}
