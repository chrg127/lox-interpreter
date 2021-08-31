programname := clox

# can be: debug, release
build := debug

_objs_main := chunk.o compiler.o disassemble.o memory.o main.o object.o \
			  scanner.o table.o value.o vm.o vector.o
libs :=
CC := gcc
CFLAGS := -I. -std=c11 -Wall -Wextra -pipe \
		 -Wcast-align -Wcast-qual -Wpointer-arith -Wswitch \
		 -Wformat=2 -Wmissing-include-dirs -Wno-unused-parameter
flags_deps = -MMD -MP -MF $(@:.o=.d)

ifeq ($(build),debug)
    outdir := debug
    CFLAGS += -g -DDEBUG -DDEBUG_PRINT_CODE -DDEBUG_TRACE_EXECUTION
else
    outdir := release
    CFLAGS += -O3
endif

objs_main := $(patsubst %,$(outdir)/%,$(_objs_main))

all: $(outdir) $(outdir)/$(programname)

$(outdir)/$(programname): $(objs_main)
	$(info Linking $@ ...)
	$(CC) $(objs_main) -o $@ $(libs)

-include $(outdir)/*.d

$(outdir)/%.o: %.c
	$(info Compiling $< ...)
	@$(CC) $(CFLAGS) $(flags_deps) -c $< -o $@

$(outdir):
	mkdir -p $(outdir)

.PHONY: clean tests

clean:
	rm -rf $(outdir)

