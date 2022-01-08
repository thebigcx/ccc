#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

struct modrm
{
    uint8_t mod, reg, rm;
    int used;
};

struct sib
{
    uint8_t scale, index, base;
    int used;
};

struct code;

int assemble(FILE *out, struct code **code, size_t codecnt);
