#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline char *read_file(const char *path)
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

#endif
