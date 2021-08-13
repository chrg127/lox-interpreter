package lox;

import java.util.Map;
import java.util.Set;
import java.util.HashMap;
import java.util.HashSet;

class Environment {
    public final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();
    private final Set<String> uninitialized = new HashSet<>();

    public Environment()                      { enclosing = null; }
    public Environment(Environment enclosing) { this.enclosing = enclosing; }

    void declare(String name) {
        values.put(name, null);
        uninitialized.add(name);
    }

    boolean declared(String name) {
        return values.containsKey(name);
    }

    Object uncheckedGet(String name) {
        return values.get(name);
    }

    void uncheckedSet(String name, Object value) {
        uninitialized.remove(name);
        values.put(name, value);
    }

    Object getValue(Token name) {
        if (!declared(name.lexeme))
            throw new RuntimeError(name, "undefined variable");
        if (uninitialized.contains(name.lexeme))
            throw new RuntimeError(name, "uninitialized variable access");
        return uncheckedGet(name.lexeme);
    }

    void setValue(Token name, Object value) {
        if (!declared(name.lexeme))
            throw new RuntimeError(name, "undefined variable");
        uncheckedSet(name.lexeme, value);
    }

    // declares and set a variable in one go
    public void define(String name, Object value) {
        uncheckedSet(name, value);
    }




    private Environment ancestor(int dist) {
        Environment env = this;
        for (int i = 0; i < dist; i++)
            env = env.enclosing;
        return env;
    }

    public Object getAt(int dist, String name) {
        return ancestor(dist).uncheckedGet(name);
    }

    public void assignAt(int dist, Token name, Object value) {
        ancestor(dist).uncheckedSet(name.lexeme, value);
    }

    Object get(Token name) {
        if (declared(name.lexeme))
            return getValue(name);
        else if (enclosing == null)
            throw new RuntimeError(name, "undefined variable '" + name.lexeme + "'.");
        return enclosing.get(name);
    }

    void assign(Token name, Object value) {
        if (declared(name.lexeme)) {
            uncheckedSet(name.lexeme, value);
            return;
        } else if (enclosing == null)
            throw new RuntimeError(name, "undefined variable '" + name.lexeme + "'");
        enclosing.assign(name, value);
    }
}
