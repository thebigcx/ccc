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
    INST_POP
};
