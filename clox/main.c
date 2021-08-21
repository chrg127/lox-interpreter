#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "chunk.h"
#include "disassemble.h"
#include "vm.h"

static void repl()
{
    char line[1024];
    char *s;

    for (;;) {
        printf("> ");

        if (s = fgets(line, sizeof(line), stdin), s == NULL) {
            printf("\n");
            break;
        }

        vm_interpret(line, "stdin");
    }
}

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("error");
        exit(1);
    }

    fseek(file, 0l, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buf = malloc(size + 1);
    if (!buf) {
        perror("error");
        exit(1);
    }

    size_t bread = fread(buf, sizeof(char), size, file);
    if (bread < (size_t)size) {
        fprintf(stderr, "error: couldn't read file %s\n", path);
        exit(1);
    }

    buf[bread] = '\0';
    fclose(file);
    return buf;
}

static void run_file(const char *path)
{
    char *src = read_file(path);
    VMResult result = vm_interpret(src, path);
    free(src);

    if (result == VM_COMPILE_ERROR)
        exit(2);
    if (result == VM_RUNTIME_ERROR)
        exit(3);
}

int main(int argc, char *argv[])
{
    vm_init();

    if (argc == 1)
        repl();
    else if (argc == 2)
        run_file(argv[1]);
    else {
        fprintf(stderr, "usage: clox [path]\n");
        vm_free();
        exit(1);
    }

    vm_free();

    return 0;
}
