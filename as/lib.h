#pragma once

#include <stdio.h>

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

FILE *xfopen(const char *path, const char *access);
void *memdup(void *mem, size_t n);
long xstrtonum(const char *str, char **end);
void error(const char *format, ...);
char *stresc(const char *str, char delim, size_t *len);
void fwritestr(const char *str, FILE *file);
