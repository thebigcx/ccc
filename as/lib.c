#include "lib.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

FILE *xfopen(const char *path, const char *access)
{
    FILE *f = fopen(path, access);
    if (!f)
    {
        fprintf(stderr, "as: %s: %s\n", path, strerror(errno));
        exit(-1);
    }

    return f;
}

void *memdup(void *mem, size_t n)
{
    return memcpy(malloc(n), mem, n);
}

// Better string to number - handles 0x10, 0b10, 020, etc
long xstrtonum(const char *str, char **end)
{
    int base = 10;
    if (*str == '0')
    {
        if      (*(str + 1) == 'x') { base = 16; str += 2; }
        else if (*(str + 1) == 'b') { base = 2;  str += 2; }
        else if (isdigit(*(str + 1))) { base = 8; str++; }
    }
    return strtoull(str, end, base);
}

void error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    exit(-1);
}

char *stresc(const char *str, char delim, size_t *len)
{
    const char *ostr = str;

    char buf[256] = { 0 }; // TODO: this is bad
    char *strp = buf;
    for (; *str && *str != delim; str++)
    {
        if (*str == '\\')
        {
            switch (*(++str))
            {
                case 'n': *strp++ = '\n'; break;
                case '"': *strp++ = '"'; break;
            }
        }
        else *strp++ = *str;
    }

    if (len)
        *len = (size_t)(str - ostr) + 1;

    return strdup(buf);
}

// Write string to file, including null character
void fwritestr(const char *str, FILE *file)
{
    fputs(str, file);
    fputc(0, file);
}
