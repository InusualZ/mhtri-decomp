/*
 * One function: `fn_802B2978`, a byte-wise blend of two packed 32-bit colours.
 * `.text` 0x802B2978-0x802B2AA0 (296 B, the only function in the range).
 *
 * Per channel (bits 24, 16, 8, 0) it computes `from_byte + (int)(t * (float)(s16)(to_byte - from_byte))`,
 * masks the result to a byte and repacks the four channels in their original order.  It calls nothing and
 * reads no memory, so besides the function itself the only relocation is the compiler's implicit
 * int->float magic constant.  `(s16)` is load-bearing: without the sign-extended difference the object is
 * 8 instructions off (90.81 %).
 *
 * Flags: the unit builds with `cflags_main` (`auto`, `Wii/1.3`, `-O3 -inline noauto`), whose peephole is
 * on, but retail's codegen is the peephole-off one - 0x43300000 is materialised twice instead of once,
 * `srwi`+`clrlwi` stays unfused instead of becoming `rlwinm`, and the return value is built from
 * `slwi`/`or` instead of `rlwimi`.  Measured on the real command line: `-opt nopeephole` -> 99.93 %,
 * peephole on -> 75.27 %.  The neighbouring `fn_802B2AA0` (another unit) shows the same duplicated `lis`,
 * so the whole original TU was built peephole-off and a per-region `cflags` group is the real fix; the
 * `#pragma peephole off` / `reset` pair below is the source-level stand-in until that exists.
 *
 * Residual: 73 of 74 rows match.  The one that does not is `lfd f2, <magic>@sda21` - retail loads
 * `lbl_8079A460`, ours loads the pool entry MWCC names `@NN`.  That constant is the implicit int->float
 * magic, which cannot be named from source (playbook 29), so it is the residual and the unit's `.sdata2`
 * range stays unclaimed.
 *
 * Name: still generated.  The runtime dump has only `zz_02b2978_` for it and the neighbours carry no
 * naming scheme, so no evidenced name exists; the two-edit rename rides a rename batch when one does.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit auto/802B2978_fn_802B2978.c`.
 */

#include "types.h"

#pragma peephole off

/* Blends the two packed colours channel by channel and returns the packed result. */
u32 fn_802B2978(u32 from, u32 to, f32 t)
{
    u8 a, b;
    u32 r0, r1, r2, r3;

    a = from >> 24; b = to >> 24; r0 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from >> 16; b = to >> 16; r1 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from >> 8;  b = to >> 8;  r2 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from;       b = to;       r3 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;

    return (r0 << 24) | (r1 << 16) | (r2 << 8) | r3;
}

#pragma peephole reset
