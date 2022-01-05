#pragma once

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
    INST_ADD,
    INST_MOV
};
