package tools;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAST {
    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.err.println("usage: generate_ast [dir]");
            System.exit(1);
        }
        String dir = args[0];
        defineAst(dir, "Expr", Arrays.asList(
            "Assign     : Token name, Expr value",
            "Binary     : Expr left, Token operator, Expr right",
            "Call       : Expr callee, Token paren, List<Expr> arguments",
            "Get        : Expr object, Token name",
            "Set        : Expr object, Token name, Expr value",
            "Grouping   : Expr expression",
            "Literal    : Object value",
            "Logical    : Expr left, Token operator, Expr right",
            "This       : Token keyword",
            "Unary      : Token operator, Expr right",
            "Variable   : Token name"
        ));
        defineAst(dir, "Stmt", Arrays.asList(
            "Expression : Expr expression",
            "Function   : Token name, List<Token> params, List<Stmt> body",
            "If         : Expr cond, Stmt thenBranch, Stmt elseBranch",
            "Print      : Expr expression",
            "Return     : Token keyword, Expr value",
            "Block      : List<Stmt> statements",
            "Class      : Token name, List<Stmt.Function> methods",
            "Var        : Token name, Expr initializer",
            "While      : Expr cond, Stmt body"
        ));
    }

    private static void defineAst(String dir, String base,
            List<String> types) throws IOException {
        String path = dir + "/" + base + ".java";
        var writer = new PrintWriter(path, "UTF-8");
        writer.println("package lox;");
        writer.println();
        writer.println("import java.util.List;");
        writer.println();
        writer.println("abstract class " + base + " {");

        defineVisitor(writer, base, types);

        for (String type : types) {
            String klass = type.split(":")[0].trim();
            String fields = type.split(":")[1].trim();
            defineType(writer, base, klass, fields);
        }

        writer.println();
        writer.println("    abstract <T> T accept(Visitor<T> visitor);");
        writer.println("}");
        writer.close();
    }

    private static void defineType(PrintWriter writer, String base,
            String klass, String fields) {
        String[] fieldList = fields.split(", ");

        writer.println("    static class " + klass + " extends " +
                base + " {");

        // fields
        for (var f : fieldList) {
            writer.println("        final " + f + ";");
        }
        writer.println();

        // constructor
        writer.println("        " + klass + "(" + fields + ") {");
        for (var f : fieldList) {
            String name = f.split(" ")[1];
            writer.println("            this." + name + " = " + name + ";");
        }
        writer.println("        }");

        // visitor
        writer.println();
        writer.println("        @Override");
        writer.println("        <T> T accept(Visitor<T> visitor) {");
        writer.println("            return visitor.visit" + klass + base + "(this);");
        writer.println("        }");
        writer.println("    }");
        writer.println();
    }

    private static void defineVisitor(PrintWriter writer, String base,
            List<String> types) {
        writer.println("    interface Visitor<T> {");

        for (String type : types) {
            String name = type.split(":")[0].trim();
            writer.println("        T visit" + name + base + "(" +
                           name + " " + base.toLowerCase() + ");");
        }
        writer.println("    }");
        writer.println();
    }

}
