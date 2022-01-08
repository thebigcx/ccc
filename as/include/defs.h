#pragma once

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

enum INST
{
    // Simple instructions, must be in this order to encode
    INST_ADD = 0,
    INST_OR,
    INST_ADC,
    INST_SBB,
    INST_AND,
    INST_SUB,
    INST_XOR,
    INST_CMP,
    
    // Rest, any order (doesn't matter)
    INST_MOV,
    INST_PUSH,
    INST_POP,
    INST_RET,
    INST_HLT,
    INST_CLI,
    INST_STI,

    // Jumps
    INST_JMP
};
