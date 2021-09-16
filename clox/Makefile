programname := clox

# can be: debug, release
build := debug

_objs_main := chunk.o compiler.o disassemble.o memory.o main.o object.o \
			  scanner.o table.o value.o vm.o vector.o
libs :=
CC := gcc
CFLAGS := -I. -std=c11 -Wall -Wextra -pedantic -pipe \
		 -Wcast-align -Wcast-qual -Wpointer-arith -Wswitch \
		 -Wformat=2 -Wmissing-include-dirs -Wno-unused-parameter
flags_deps = -MMD -MP -MF $(@:.o=.d)

ifeq ($(build),debug)
    outdir := debug
    CFLAGS += -g -DDEBUG
else ifeq ($(build),reldeb)
    outdir := reldeb
    CFLAGS += -O2 -g -DNDEBUG
else
    outdir := release
    CFLAGS += -O3 -DNDEBUG
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

