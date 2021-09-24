#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "util.h"

static void repl()
{
    char line[1024];
    char *s;

    for (;;) {
        printf("> ");

        // exit condition
        if (s = fgets(line, sizeof(line), stdin), s == NULL) {
            printf("\n");
            break;
        }

        // interpret only if line isn't empty
        if (!(s[0] == '\n' && s[1] == '\0'))
            vm_interpret(line, "stdin");
    }
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
        fprintf(stderr, "usage: clox [file]\n");
        vm_free();
        return 1;
    }

    vm_free();

    return 0;
}
