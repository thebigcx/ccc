#pragma once

#include <stdio.h>

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

char *readfile(FILE *file);