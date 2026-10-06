/*
 * g3d/g3d_anmscn.cpp - one accessor of the nw4r g3d `ResAnmAmbLight` cluster: resolve the object through the
 *   checked helper fn_80066C8C and read the word at +0xC.
 * RANGE. .text 0x800680A8-0x800680CC (1 function); extab, extabindex.  The TU is `g3d_anmscn.cpp`: the neighbours
 *   fn_80067EFC and fn_800680CC cite that string (lbl_8058C288), whose data fragment 0x8058C288-0x8058CC40 holds the
 *   header cluster the file includes; this unit holds one function of the TU.
 * FLAGS. `cflags_g3d`'s `-O3` is measured on this function (docs/g3d.md, playbook 27).
 * NAMES. The map rows are the C++ manglings of the stems (`fn_800680A8__FPv`, `fn_80066C8C__FPv`; playbook 48); the
 *   source keeps the bare stems, because writing the suffix mangles a second time (playbook 50).
 * RESIDUALS. none.
 */

#include "types.h"
#include "g3d/fn_80063888.h" /* fn_80066C8C - the owner is g3d/fn_80063888.cpp (rule 2); its map name
                              is the mangling `fn_80066C8C__FPv`, so the declaration keeps C++ linkage. */

unsigned int fn_800680A8(void *obj)
{
    return *(unsigned int *)((char *)fn_80066C8C(obj) + 0xC);
}
