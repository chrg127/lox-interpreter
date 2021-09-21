package lox;

import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

class Environment {
    public final Environment enclosing;
    private final List<String> names = new ArrayList<>();
    private final List<Object> values = new ArrayList<>();
    private final Set<Integer> uninitialized = new HashSet<>();

    public Environment()                      { enclosing = null; }
    public Environment(Environment enclosing) { this.enclosing = enclosing; }

    private Object uncheckedGet(int index) {
        return values.get(index);
    }

    private Environment ancestor(int dist) {
        Environment env = this;
        for (int i = 0; i < dist; i++)
            env = env.enclosing;
        return env;
    }



    public int declare(String name) {
        int i = values.size();
        uninitialized.add(i);
        values.add(null);
        names.add(name);
        return i;
    }

    public void assign(int index, Object value) {
        uninitialized.remove(index);
        values.set(index, value);
    }

    public Object get(int index, Token name) {
        if (uninitialized.contains(index))
            throw new RuntimeError(name, "uninitialized variable access");
        return uncheckedGet(index);
    }

    public void define(String optName, Object value) {
        values.add(value);
        names.add(optName);
    }

    public Object getAt(int dist, int index, Token name) {
        return ancestor(dist).get(index, name);
    }

    public void assignAt(int dist, int index, Object value) {
        ancestor(dist).assign(index, value);
    }

    // fix-up for 'this' variables. (or, you could say, a hack)
    public Object getByNameAt(int dist, String name) {
        var env = ancestor(dist);
        int i = env.names.indexOf(name);
        assert i != -1;
        var v = env.values.get(i);
        return uncheckedGet(i);
    }
}
