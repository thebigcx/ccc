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

struct inst *searchi(struct code *code, struct inst *tbl, size_t len);

void assemble_file();

// 64-bit
struct inst *searchi64(struct code *code);
size_t instsize64(struct inst *inst, struct code *code);
void assemble64(struct code *code, struct inst *inst, size_t lc);

// 16-bit
struct inst *searchi16(struct code *code);
size_t instsize16(struct inst *inst, struct code *code);
void assemble16(struct code *code, struct inst *inst, size_t lc);
