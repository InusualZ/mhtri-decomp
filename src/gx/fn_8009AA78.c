/* gx/fn_8009AA78.c - the 8 functions at .text 0x8009AA78..0x8009ACE4.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * A write-gather-pipe command writer and the six entry points built on it. `fn_8009AB1C`/`28`/`38` are
 * the u32/u16/u8 stores to the pipe window at 0xCC008000; the other functions pack a command word and
 * hand it to one of two shape-identical writers, `fn_800868A0` (opcode byte 0x61, then a u32) and
 * `GDWriteXFCmd` (0x10, a u16 register, then a u32). `fn_8009AA78`/`fn_8009AB48`/`fn_8009AB8C` build
 * that word from bit fields; `fn_8009AC44`/`fn_8009AC98` emit the fixed 0x10 and 0x08 opcodes. The
 * `.sdata2` byte table `lbl_80795F58` ({0,2,1,3}, owned by another unit) is read as `lbl_80795F58[e]`.
 *
 * Codegen lever: this unit needs the **peephole pass off**. Under the lib's `-O3` the peephole folds
 * `value & 0xFFFF` + `sth` into a bare `sth` and `(x & 0xFF) << 16` into one `rlwinm`; retail emits the
 * unfused `clrlwi`/`slwi` pair everywhere, which costs `fn_8009AB28`/`AB38` four bytes each and the
 * register colouring of `fn_8009AA78`/`AB8C`. `#pragma peephole off` at the top of this file takes all
 * 8 symbols to 100 % (fn_8009AA78 98.90, fn_8009AB28/38 71.25, fn_8009AB8C 77.93 -> 100.00) and
 * reproduces the target's 0x26C-byte `.text`, 0x28-byte `extab` and 0x3C-byte `extabindex` exactly.
 * `-O1` (which is `-opt level=1`, i.e. no peephole) is indistinguishable from `-O3` + the pragma here,
 * so the pragma scopes the setting to this unit rather than moving the shared `auto` lib flag.
 *
 * The object is byte-identical to the target's `.text`, `extab`, `extabindex` and relocation records;
 * only the local extab object names (our `@13`, the map's `@etb_800099F0`) and the `.comment` version
 * byte (ours 0x0f, retail 0x0e) differ, neither of which reaches the linked DOL.
 *
 * Names: the dump's symbol list (docs/memory-dump.md) carries `SetTRKConnected` at 0x80077474,
 * 0x800868D8 *and* 0x8009AB1C, whose bodies are the same single-store pipe write - a dumper alias, not
 * evidence - and no other address in the range resolves, so the provisional names stand.
 *
 * NAMES. GDSetTexCoordScale2 is a GUESS (0x8009AB8C: it writes one texture coordinate's s/t scale, bias and wrap
 *   pair behind the 0xFE03FFFF mask, the SDK's GD call of that name); GDSetGenMode2 is a GUESS (0x8009AA78: the gen-mode
 *   word and the two XF counts, the SDK's GD call of that name).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit gx/fn_8009AA78.c`.
 */

#include "gx.h"
#include "g3d/g3d_state.h" /* fn_800868A0 (rule 2) */
#include "g3d/fn_80075DCC.h" /* GDWriteXFCmd, owned by g3d/fn_80075DCC.cpp (rule 2) */

/* The write-gather-pipe window at 0xCC008000 and its `GXWGFifo` macro live in `gx.h` now that
 * a second `auto` unit (the GX-writer family at 0x800C6F90) needs them too (CLAUDE.md -> Conventions,
 * rule 1: a type more than one unit uses lives in one header). */

/* Pipe writers and the byte table, all defined by other translation units of the same library; the
 * addresses are the map's and the local extab names above are the only object-level difference.
 * `fn_800868A0` comes from `g3d/g3d_state.h`, `GDWriteXFCmd` from `g3d/fn_80075DCC.h` (rule 2). */
extern const u8 lbl_80795F58[4];

#pragma peephole off

/* Packs the four operands and the table's byte into one command word, then writes it and the two
 * register fields it selects. */
void GDSetGenMode2(u8 a, u8 b, u8 c, u8 d, int e) {
    fn_800868A0(0xFE07FC3F);
    fn_800868A0((((d & 0xFF) << 16) | ((u32)lbl_80795F58[e] << 14))
                | ((((c & 0xFF) - 1) << 10) | ((a & 0xFF) | ((b & 0xFF) << 4))));
    GDWriteXFCmd(0x1009, b);
    GDWriteXFCmd(0x103F, a);
}

/* Writes a 32-bit command word to the pipe. */
void fn_8009AB1C(u32 value) {
    GXWGFifo.u32 = value;
}

/* Writes the low half of a command word to the pipe. */
void fn_8009AB28(u16 value) {
    GXWGFifo.u16 = value;
}

/* Writes the low byte of a command word to the pipe. */
void fn_8009AB38(u8 value) {
    GXWGFifo.u8 = value;
}

/* Writes one table byte as a command word. */
void fn_8009AB48(int e) {
    fn_800868A0(0xFE00C000);
    fn_800868A0((u32)lbl_80795F58[e] << 14);
}

/* Writes the same field set twice, once for each of two adjacent registers. */
void GDSetTexCoordScale2(u32 a, u16 b, u8 c, u8 d, u16 e, u8 f, u8 g) {
    fn_800868A0(0xFE03FFFF);
    fn_800868A0(((d << 17) | ((b - 1) | (c << 16))) | ((a * 2 + 0x30) << 24));
    fn_800868A0(((g << 17) | ((e - 1) | (f << 16))) | ((a * 2 + 0x31) << 24));
}

/* Emits the 0x10 opcode with a decremented register and a 16-bit operand. */
void fn_8009AC44(u16 a, u8 b) {
    fn_8009AB38(16);
    fn_8009AB28(b - 1);
    fn_8009AB28(a);
}

/* Emits the 0x08 opcode with a byte register and a 32-bit operand. */
void fn_8009AC98(u8 a, u32 b) {
    fn_8009AB38(8);
    fn_8009AB38(a);
    fn_8009AB1C(b);
}
