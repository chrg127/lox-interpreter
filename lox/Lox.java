package lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Lox {
    private static final Interpreter interpreter = new Interpreter();
    private static boolean hadError = false;
    private static boolean hadRuntimeError = false;
    private static String currFilename = "";

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("usage: jlox [script]");
            System.exit(1);
        } else if (args.length == 1)
            runFile(args[0]);
        else
            runPrompt();
    }

    public static void error(int line, String message) {
        report(line, "error", "", message);
        hadError = true;
    }

    public static void error(Token token, String message) {
        if (token.type == Token.Type.EOF)
            report(token.line, "error", "at end", message);
        else
            report(token.line, "error", "at '" + token.lexeme + "'", message);
        hadError = true;
    }

    public static void runtimeError(RuntimeError error) {
        report(error.token.line, "runtime error", "", error.getMessage());
        hadRuntimeError = true;
    }



    private static void runFile(String path) throws IOException {
        currFilename = path;
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));
        if (hadError)
            System.exit(2);
        if (hadRuntimeError)
            System.exit(3);
    }

    private static void runPrompt() throws IOException {
        currFilename = "stdin";
        var input = new InputStreamReader(System.in);
        var reader = new BufferedReader(input);

        for (;;) {
            System.out.print(">>> ");
            String line = reader.readLine();
            if (line == null)
                break;
            run(line);
            hadError = false;
        }
    }

    private static void run(String source) {
        var scanner = new lox.Scanner(source);
        List<Token> tokens = scanner.scanTokens();
        var parser = new Parser(tokens);
        List<Stmt> statements = parser.parse();
        if (hadError)
            return;
        // printAST(statements);
        Resolver resolver = new Resolver(interpreter);
        resolver.resolve(statements);
        if (hadError)
            return;
        interpreter.interpret(statements);
    }

    private static void printAST(List<Stmt> statements) {
        var printer = new ASTPrinter();
        for (var stmt : statements)
            System.out.println(printer.printStmt(stmt));
    }

    private static void report(int line, String errtype, String where, String message) {
        System.err.print(currFilename + ":" + line + ": " + errtype + ": ");
        if (!where.isEmpty())
            System.err.print(where + ": ");
        System.err.println(message);
    }
}
