#pragma once

#include <stddef.h>

enum TYPENAME
{
    TYPE_INT8, TYPE_INT16, TYPE_INT32, TYPE_INT64,
    TYPE_UINT8, TYPE_UINT16, TYPE_UINT32, TYPE_UINT64,
    TYPE_FLOAT32, TYPE_FLOAT64, TYPE_VOID, TYPE_STRUCT, TYPE_UNION,

    TYPE_FUNC // Special
};

struct structmem;

struct type
{
    int ptr, name, arrlen;

    struct
    {
        struct type *ret;
        struct type *params[6];
        unsigned int paramcnt;
    } func;

    struct
    {
        struct structmem *members;
        unsigned int memcnt;
        size_t size;
    } struc;
};

// Struct member
struct structmem
{
    char *name;
    struct type type;
    size_t offset;
};