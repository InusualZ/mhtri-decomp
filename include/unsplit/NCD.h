/*
 * NCD declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * `NCDGetCurrentIpConfig` (0x8051C64C, 344 B) is the NCD library's "read the console's IP
 * configuration into this block" entry point: `NHTTP/NHTTP_bgnend.c`'s `NHTTPi_Startup` calls it
 * with the NHTTP info block and panics on a negative answer.  The address is inside no registered
 * `splits.txt` range (the enclosing band is the NCD/REX region below `NWC24/nwc24_msg.c`, and
 * `ncdsystem.c` is the file name the neighbouring pool carries), and no registered unit defines it,
 * so rule 2 puts it here.
 *
 * Added with the `NHTTP/NHTTP_bgnend.c` body pass.
 */
#ifndef MHTRI_UNSPLIT_NCD_H
#define MHTRI_UNSPLIT_NCD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8051C64C - fill the caller's NCD IP configuration block; negative on failure.  The block is
 * the caller's payload (NHTTP passes its own system-info block), so it stays untyped here. */
/* untyped: caller-owned payload */
s32 NCDGetCurrentIpConfig(void* config);

/* 0x8051C554 - fill the caller's interface-configuration block; non-zero on failure (the DWCi
 * runtime initialiser prints its own " NCDGetCurrentIfConfig failed.[%d]\n" with the answer and
 * hands it the `+0x4000` region of its runtime block).  The name is read off that call site's own
 * message: this band is unregistered, so this header is the symbol's home until `ncdsystem.c`
 * registers, and that unit's own header will be the real one then. */
s32 NCDGetCurrentIfConfig(u8* config);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_NCD_H */
