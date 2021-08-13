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

    public void declare(String name) {
        values.put(name, null);
        uninitialized.add(name);
    }

    public boolean declared(String name) {
        return values.containsKey(name);
    }

    private Object uncheckedGet(String name) {
        return values.get(name);
    }

    public Object getValue(Token name) {
        if (uninitialized.contains(name.lexeme))
            throw new RuntimeError(name, "uninitialized variable access");
        return uncheckedGet(name.lexeme);
    }

    public void setValue(String name, Object value) {
        uninitialized.remove(name);
        values.put(name, value);
    }

    // declares and set a variable in one go
    public void define(String name, Object value) {
        values.put(name, value);
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
        ancestor(dist).setValue(name.lexeme, value);
    }

    public Object get(Token name) {
        if (declared(name.lexeme))
            return getValue(name);
        else if (enclosing == null)
            throw new RuntimeError(name, "undefined variable '" + name.lexeme + "'.");
        return enclosing.get(name);
    }

    public void assign(Token name, Object value) {
        if (declared(name.lexeme)) {
            setValue(name.lexeme, value);
            return;
        } else if (enclosing == null)
            throw new RuntimeError(name, "undefined variable '" + name.lexeme + "'");
        enclosing.assign(name, value);
    }
}
