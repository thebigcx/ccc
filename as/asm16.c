#include "asm.h"
#include "inst.h"
#include "lib.h"

static struct inst s_insttbl[] = {

};

struct inst *searchi16(struct code *code)
{
    return searchi(code, s_insttbl, ARRLEN(s_insttbl));
}

size_t instsize16(struct inst *inst, struct code *code)
{
    (void)inst; (void)code;
    return 0;
}

void assemble16(struct code *code, struct inst *inst, size_t lc)
{
    (void)code; (void)inst; (void)lc;
}
