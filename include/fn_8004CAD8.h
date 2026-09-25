/*
 * Declarations for the symbols `src/fn_8004CAD8.cpp` owns (docs/plan.md 6.5, rule 2).  The parameters
 * are the owner's `long`, which is the mangling's (`wii_sysmsg_gen__FlPcl`) - `int` would ask for a
 * different symbol.
 */
#ifndef MHTRI_FN_8004CAD8_H
#define MHTRI_FN_8004CAD8_H

#include "types.h"

#ifdef __cplusplus
void wii_sysmsg_gen(long id, char* buf, long a);
#endif

/* C linkage: the target symbol is the unmangled `fn_80051570` (.text 0x80051570, a 4-byte `blr`).
 * Moved out of `src/g3d/g3d_anmchr.cpp` on landing (docs/plan.md 6.5, rule 2): that range was
 * written before this owner registered, so its local `extern "C"` declaration was a boundary
 * artefact. */
#ifdef __cplusplus
extern "C" {
#endif
u32 fn_80051570(u32);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_8004CAD8_H */
