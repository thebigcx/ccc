#include <asm.h>

size_t asm_sizeof(struct type type)
{
    if (type.ptr) return 8;

    if (type.name == TYPE_STRUCT || type.name == TYPE_UNION)
        return type.struc.size;

    int prim;
    switch (type.name)
    {
        case TYPE_INT8:
        case TYPE_UINT8:
            prim = 1; break;
        case TYPE_INT16:
        case TYPE_UINT16:
            prim = 2; break;
        case TYPE_INT32:
        case TYPE_UINT32:
        case TYPE_FLOAT32:
            prim = 4; break;
        case TYPE_INT64:
        case TYPE_UINT64:
        case TYPE_FLOAT64:
            prim = 8; break;
        case TYPE_FUNC:
            return 8;
    }

    return prim * (type.arrlen ? type.arrlen : 1);
}