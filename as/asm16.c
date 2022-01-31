/*#include "asm.h"
#include "inst.h"
#include "lib.h"
#include "parse.h"

static struct inst s_insttbl[] = {
    { .mnem = "add", .opcode = 0x01, .op1 = OP_REG | OP_SZEX8, .op2 = OP_RM | OP_SZEX8, .reg = -1 },
};

struct inst *searchi16(struct code *code)
{
    return searchi(code, s_insttbl, ARRLEN(s_insttbl));
}

void mkmodrmsib(struct modrm *modrm, struct sib *sib, struct code *code, struct inst *inst)
{
    struct codeop *mem = ISMEM(inst->op1) && ISMEM(code->op1.type) ? &code->op1
                       : ISMEM(inst->op2) && ISMEM(code->op2.type) ? &code->op2 : NULL;

    struct codeop *reg = (inst->op1 & OP_TYPEM) == OP_REG && ISREG(code->op1.type) ? &code->op1
                       : (inst->op2 & OP_TYPEM) == OP_REG && ISREG(code->op2.type) ? &code->op2 : NULL;

    struct codeop *rm = (inst->op1 & OP_TYPEM) == OP_RM && ISRM(code->op1.type) ? &code->op1
                      : (inst->op2 & OP_TYPEM) == OP_RM && ISRM(code->op2.type) ? &code->op2 : NULL;

    if (!reg && !rm) return;

    if (!modrm->reg && reg && reg != rm) modrm->reg = reg->val;
    if (!mem)
    {
        modrm->mod = 0b11;
        if (rm) modrm->rm = rm->val;
    }
    else
    {

    }
}

size_t instsize16(struct inst *inst, struct code *code)
{
    size_t s = 1;

    struct modrm modrm = { .reg = inst->reg != (uint8_t)-1 ? inst->reg : 0 };
    struct sib sib = { 0 };
    mkmodrmsib(&modrm, &sib, code, inst);

    if ((modrm.reg || ISRM(inst->op1) || ISRM(inst->op2)) && !(inst->flags & IF_ROPCODE))
        s++;

    return s;
}

#define OPOVPRE(code, inst) ((inst).size & OP_SIZE32 || ISREGSZ((code).op1.type, OP_SIZE32) || ISREGSZ((code).op2.type, OP_SIZE32))
//#define ADROVPRE(code, inst) ((ISMEM((code).op1.type) && (code).op1.sib.flags & SIB_32BIT) || (ISMEM((code).op2.type) && (code).op2.sib.flags & SIB_32BIT))

void assemble16(struct code *code, struct inst *inst, size_t lc)
{
    struct modrm modrm = { .reg = inst->reg != (uint8_t)-1 ? inst->reg : 0 };
    struct sib sib = { 0 };
    mkmodrmsib(&modrm, code, inst);

    if (OPOVPRE(*code, *inst)) emit8(0x66);
    //if (ADROVPRE(*code, *inst)) emit8(0x67);

    if (inst->pre) emit8(inst->pre);

    uint8_t opcode = inst->opcode;

    if (inst->flags & IF_ROPCODE) opcode += modrm.reg & 0b111;

    emit8(opcode);

    if ((modrm.reg || ISRM(inst->op1) || ISRM(inst->op2)) && !(inst->flags & IF_ROPCODE))
        emit8((modrm.mod << 6) | ((modrm.reg & 0b111) << 3) | (modrm.rm & 0b111));
}*/
