#pragma once

#include <stdint.h>
#include <stddef.h>

// op format:
// | - | - | - | - | - | - | - | - |
//     | q | l | w | b | i | m | r |

#define OP_REG (1 << 0)
#define OP_MEM (1 << 1)
#define OP_IMM (1 << 2)
#define OP_TYPEM  (0b111)

#define OP_RM  (OP_REG | OP_MEM)

#define ISREG(op) ((op & OP_TYPEM) & OP_REG)
#define ISMEM(op) ((op & OP_TYPEM) & OP_MEM)
#define ISRM(op)  (ISREG(op) || ISMEM(op))
#define ISIMM(op) ((op & OP_TYPEM) & OP_IMM)

#define OP_SIZE8  (1 << 3)
#define OP_SIZE16 (1 << 4)
#define OP_SIZE32 (1 << 5)
#define OP_SIZE64 (1 << 6)
#define OP_CTLREG (1 << 7)
#define OP_DBGREG (1 << 8)
#define OP_SIZEM  (0b111111 << 3)

// All excluding 8-bit
#define OP_SZEX8 (OP_SIZE16 | OP_SIZE32 | OP_SIZE64)

#define OP_ALLSZ (OP_SIZE8 | OP_SZEX8)

#define ISREGSZ(op, sz) ((op & OP_REG) && ((op & OP_SIZEM) & sz))

#define REG_AX  0b0000
#define REG_CX  0b0001
#define REG_DX  0b0010
#define REG_BX  0b0011
#define REG_SP  0b0100
#define REG_BP  0b0101
#define REG_SI  0b0110
#define REG_DI  0b0111

#define REG_R8  0b1000
#define REG_R9  0b1001
#define REG_R10 0b1010
#define REG_R11 0b1011
#define REG_R12 0b1100
#define REG_R13 0b1101
#define REG_R14 0b1110
#define REG_R15 0b1111

#define REG_CR0 0b0000
#define REG_CR1 0b0001
#define REG_CR2 0b0010
#define REG_CR3 0b0011
#define REG_CR4 0b0100
#define REG_CR5 0b0101
#define REG_CR6 0b0110
#define REG_CR7 0b0111

#define REG_ES  0b0000
#define REG_CS  0b0001
#define REG_SS  0b0010
#define REG_DS  0b0011
#define REG_FS  0b0100
#define REG_GS  0b0101

// Special register
#define REG_RIP 0b10000

#define REG_NUL ((uint8_t)-1)

struct modrm
{
    uint8_t mod, reg, rm;
};

#define SIB_USED   (1 << 0) // Used
#define SIB_NODISP (1 << 1) // No displacement
#define SIB_16BIT  (1 << 2) // 32-bit address
#define SIB_32BIT  (1 << 3) // 32-bit address
#define SIB_DISP8  (1 << 4) // 8-bit displacement

struct sib
{
    int flags;
    uint8_t scale, idx, base;
    uint8_t seg;
};

#define IF_ROPCODE (1 << 0) // opcode+r
#define IF_DEF64   (1 << 1) // instruction defaults to 64-bit
#define IF_REL     (1 << 2) // immediate operand is relative to %rip

struct inst
{
    const char *mnem; // mnemonic
    uint64_t op1, op2;
    uint8_t pre;
    uint8_t opcode;
    uint8_t reg; // reg field in modr/m byte, 0 if unused
    int flags;
    uint8_t size; // optional size attribute
};
