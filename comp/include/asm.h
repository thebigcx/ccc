#pragma once

#include "type.h"

#include <stddef.h>
#include <stdint.h>

struct sym;
struct ast;

size_t asm_sizeof(struct type t);
/*int asm_addrof(struct sym *sym, int r);
int asm_load(struct sym *sym, int r);
int asm_store(struct sym *sym, int r);
int asm_storederef(struct ast *ast, int r1, int r2);
int asm_loadderef(struct ast *ast, int r);

// Binary operations
int asm_add(int r1, int r2);
int asm_sub(int r1, int r2);
int asm_mul(int r1, int r2);
int asm_div(int r1, int r2);
int asm_mod(int r1, int r2);
int asm_shl(int r1, int r2);
int asm_shr(int r1, int r2);
int asm_bitand(int r1, int r2);
int asm_bitor(int r1, int r2);
int asm_bitxor(int r1, int r2);

// Comparison
int asm_cmpandset(int r1, int r2, int op);
int asm_cmpandjmp(int r1, int r2, int op, int lbl);

// Unary operations
int asm_lognot(int r);
int asm_bitnot(int r);
int asm_neg(int r);
int asm_intlit(uint64_t val, int r);*/
