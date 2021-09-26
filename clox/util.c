#include "util.h"

char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return NULL;

    fseek(file, 0l, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buf = malloc(size + 1);
    if (!buf)
        return NULL;

    size_t bread = fread(buf, sizeof(char), size, file);
    if (bread < (size_t)size)
        return NULL;

    buf[bread] = '\0';
    fclose(file);
    return buf;
}
