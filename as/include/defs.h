#pragma once

// Lexer register names
enum REG
{ 
    LREG_AH,
    LREG_BH,
    LREG_CH,
    LREG_DH,

    LREG_AL,
    LREG_BL,
    LREG_CL,
    LREG_DL,
    LREG_SPL,
    LREG_BPL,
    LREG_SIL,
    LREG_DIL,

    LREG_AX,
    LREG_BX,
    LREG_CX,
    LREG_DX,
    LREG_SP,
    LREG_BP,
    LREG_SI,
    LREG_DI,

    LREG_EAX,
    LREG_EBX,
    LREG_ECX,
    LREG_EDX,
    LREG_ESP,
    LREG_EBP,
    LREG_ESI,
    LREG_EDI,

    LREG_RAX,
    LREG_RBX,
    LREG_RCX,
    LREG_RDX,
    LREG_RSP,
    LREG_RBP,
    LREG_RSI,
    LREG_RDI,
};

// Simple instructions (encode easily)
#define INST_ADD 0
#define INST_OR  1
#define INST_ADC 2
#define INST_SBB 3
#define INST_AND 4
#define INST_SUB 5
#define INST_XOR 6
#define INST_CMP 7

#define INST_MOV 8
