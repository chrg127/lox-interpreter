_files := Lox.class Scanner.class Token.class Expr.class ASTPrinter.class \
		  Parser.class Interpreter.class

files := $(patsubst %,build/lox/%,$(_files))

all: build/jlox

build:
	mkdir -p build

build/jlox: build $(files)
	$(info Creating jar ...)
	@cd build && jar -cvfe jlox lox.Lox lox/*.class

build/tools/GenerateAST.class: tools/GenerateAST.java
	$(info Compiling Expr generator ...)
	@javac tools/GenerateAST.java -d build

lox/Expr.java: build/tools/GenerateAST.class
	$(info Running Expr generator ...)
	cd build && java tools.GenerateAST ../lox

build/lox/%.class: lox/%.java lox/Expr.java
	$(info Compiling $< ...)
	@javac -d build $<

.PHONY: run clean

run:
	java -jar build/jlox

clean:
	rm -r build lox/Expr.java
