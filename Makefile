_files := Lox.class Scanner.class Token.class ASTPrinter.class \
		  Parser.class Interpreter.class \
		  Expr.class Stmt.class
files := $(patsubst %,build/lox/%,$(_files))
generated := lox/Expr.java lox/Stmt.java

all: build/jlox

build/jlox: build $(files)
	$(info Creating jar ...)
	@cd build && jar -cvfe jlox lox.Lox lox/*.class

build:
	mkdir -p build

build/tools/GenerateAST.class: tools/GenerateAST.java
	$(info Compiling Expr generator ...)
	@javac tools/GenerateAST.java -d build

$(generated): build/tools/GenerateAST.class
	$(info Running Expr generator ...)
	cd build && java tools.GenerateAST ../lox

build/lox/%.class: lox/%.java $(generated)
	$(info Compiling $< ...)
	@javac -d build $<

.PHONY: run test clean

run:
	java -jar build/jlox

test:
	java -jar build/jlox test/main.lox

clean:
	rm -r build lox/Expr.java
