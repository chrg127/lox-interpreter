#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "util.h"

static char line[BUFSIZ];
static bool show_bytecode = false;

static void repl()
{
    char *s;

    for (;;) {
        printf(">>> ");

        // exit condition
        if (s = fgets(line, sizeof(line), stdin), s == NULL) {
            printf("\n");
            break;
        }

        // interpret only if line isn't empty
        if (!(s[0] == '\n' && s[1] == '\0'))
            vm_interpret(line, "stdin", show_bytecode);
    }
}

static void run_file(const char *path)
{
    char *src = read_file(path);
    if (!src) {
        fprintf(stderr, "error while opening file \"%s\": ", path);
        perror("");
        exit(1);
    }

    VMResult result = vm_interpret(src, path, show_bytecode);
    free(src);

    if (result == VM_COMPILE_ERROR)
        exit(2);
    if (result == VM_RUNTIME_ERROR)
        exit(3);
}

void usage(int status)
{
    fprintf(stderr, "usage: lox [file]\n"
                    "valid flags:\n"
                    "   -h: show this help text\n"
                    "   -s: show bytecode output\n");
    vm_free();
    exit(status);
}

int main(int argc, char *argv[])
{
    vm_init();

    char *file = NULL;
    while (++argv, --argc > 0) {
        if (argv[0][0] == '-' && strlen(argv[0]) == 2) {
            switch (argv[0][1]) {
            case 'h':
                usage(0);
                break;
            case 's':
                show_bytecode = true;
                break;
            default:
                fprintf(stderr, "error: unrecognized flag: -%c", argv[0][1]);
            }
            continue;
        }

        if (file != NULL)
            usage(1);
        file = argv[0];
    }

    if (!file)
        repl();
    else
        run_file(file);

    vm_free();

    return 0;
}
