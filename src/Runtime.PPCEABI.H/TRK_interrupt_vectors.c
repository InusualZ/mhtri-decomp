/*
 * Runtime.PPCEABI.H/TRK_interrupt_vectors.c - the banner and the reset slot of the Metrowerks TRK interrupt-vector image.
 *
 * .init 0x80004380..0x80004514 (404 B), part of the 8,000 B TRK exception-vector table that sits
 * between memset.c and __start.c: a banner string at +0x000, the reset slot's word at +0x100, then 24
 * handler stubs at a 0x100 stride (0x200..0x1F00) - each saving r2-r4 in SPRG1-3, planting the handler in
 * SRR0 and returning with rfi - with zero padding everywhere else.  The stride is position coding (a raw
 * copy to 0x80000000 puts each handler at its own vector), so no source shape or flag can produce the
 * layout: `-func_align` accepts only 4/8/16/32/64/128, and `mwcceppc -func_align 256` answers
 * `Unknown option '256'` (measured).
 *
 * Why data: the stubs carry their addresses as absolute immediates and the split target object has no
 * relocation section at all, so any `lis`/`addi` written as an expression would add a relocation the
 * target does not have.  The bytes come from tools/splits/gen_trk_vectors.py, run against the split
 * target object - the acceptance test is `ninja build/RMHE08/ok` plus a byte-for-byte .init comparison.
 *
 * Why this file ends/starts here: 0x80004514 is referenced by name from `.data+0x3ea8` of
 * `auto_07_8057C820_data.o`, and MWCC pads every object in .init to 8 bytes, so the only place that
 * label can live is the start of an object.  The image is therefore two units; see the generator's
 * docstring for the probe that ruled out the alternatives.  The name
 * `gTRKSystemResetVectorSlot` is the weakest here and is marked a GUESS: all that is known is that the
 * address lies inside the 0x100 (system reset) vector's 256-byte slot and that its bytes are zero.
 *
 * Nothing in the image is executed: no `lis`/`addi` base lands in the range and no 4-byte literal
 * 0x80004380 exists in the DOL, so it is dead runtime-library data kept because its object was linked
 * whole (docs/init-section.md).
 */

#include "types.h"

__declspec(section ".init") u32 gTRKInterruptVectorTable[101] = {
    0x4D657472, 0x6F776572, 0x6B732054, 0x61726765, 0x74205265, 0x73696465, 0x6E74204B, 0x65726E65,  /* 0x80004380 */
    0x6C20666F, 0x7220506F, 0x77657250, 0x43000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x800043A0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x800043C0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x800043E0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x80004400 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x80004420 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x80004440 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x80004460 */
    0x48464BE0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x80004480 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x800044A0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x800044C0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x800044E0 */
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  /* 0x80004500 */
};
