#pragma once

#include <stdint.h>
#include <stddef.h>

struct code;
struct inst;

void emit8(uint8_t v);
void emit16(uint16_t v);
void emit32(uint32_t v);
void emit64(uint64_t v);
void emit(int size, uint64_t v);

struct inst *searchi(struct code *code);

void assemble_file();

size_t instsize(struct inst *inst, struct code *code);
void assemble(struct code *code, struct inst *inst, size_t lc);
