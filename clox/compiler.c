#include "compiler.h"

#include <stdio.h>
#include "scanner.h"

void compile(const char *src)
{
    scanner_init(src);
    int line = -1;
    for (;;) {
        Token token = scan_token();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else
            printf("   | ");
        printf("%2d '%.*s'\n", token.type, token.len, token.start);
        if (token.type == TOKEN_EOF)
            break;
    }
}

