#pragma once

#include <stddef.h>

struct code;
struct inst;

void assemble_file();

size_t instsize64(struct inst *inst, struct code *code);
void assemble64(struct code *code, struct inst *inst, size_t lc);
