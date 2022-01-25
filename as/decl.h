#pragma once

#ifndef extern_
#define extern_ extern
#endif

#include <stdio.h>

#include "sym.h"

extern_ FILE *g_inf; // Input file
extern_ FILE *g_outf; // Output file
extern_ struct symbol *g_syms; // Symbol table
extern_ struct section *g_sects; // Sections
