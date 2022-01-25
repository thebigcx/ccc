#pragma once

#include <stdint.h>

#include "inst.h"

struct codeop
{
    uint64_t type, val;
    struct sib sib;
    const char *sym;
};

struct code
{
    char *mnem;
    struct codeop op1, op2;
};

struct code parse_code(const char *str);
