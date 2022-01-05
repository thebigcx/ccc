#pragma once

// Register list with proper encoding.
// Bit 3 is encoded in the REX prefix for register R8-R15
// For extended registers

/*#define REG_AX (0b0000) // Accumulator
#define REG_CX (0b0001) // Counter
#define REG_DX (0b0010) // Data
#define REG_BX (0b0011) // Base
#define REG_SP (0b0100) // Stack pointer
#define REG_BP (0b0101) // Base pointer
#define REG_SI (0b0110) // Source index
#define REG_DI (0b0111) // Destination index
*/

enum REG
{ 
    REG_AH,
    REG_BH,
    REG_CH,
    REG_DH,

    REG_AL,
    REG_BL,
    REG_CL,
    REG_DL,
    REG_SPL,
    REG_BPL,
    REG_SIL,
    REG_DIL,

    REG_AX,
    REG_BX,
    REG_CX,
    REG_DX,
    REG_SP,
    REG_BP,
    REG_SI,
    REG_DI,

    REG_EAX,
    REG_EBX,
    REG_ECX,
    REG_EDX,
    REG_ESP,
    REG_EBP,
    REG_ESI,
    REG_EDI,

    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSP,
    REG_RBP,
    REG_RSI,
    REG_RDI,
};

enum INST
{
    INST_ADD
};
