#!/usr/bin/env python3
"""Generate the two source files that reproduce the TRK interrupt-vector image.

    python tools/splits/gen_trk_vectors.py <target.o> <out_dir>

Why a generator: the image is position-coded data whose absolute addresses are already immediates and
whose split target object has **zero relocations**, so the only faithful reconstruction is the exact
8,000 bytes. Hand-writing 2,000 words invites a typo only the DOL hash would catch.

Why TWO units and not one: one address inside the range is referenced **by name** from another object.
`.data+0x3ea8` of `auto_07_8057C820_data.o` - the first word of the `.data` item at 0x805803EC that
`src/mh3_pad.cpp` declares - carries a relocation to 0x80004514. With the range unowned the auto target
object defined that label; an owning object must define it instead or the link dies with
`undefined: 'lbl_80004514'` (measured). C cannot label the middle of an array, and MWCC pads every
object in `.init` to **8 bytes** even when it is a `u8` array (measured with a probe: two 5-byte arrays
land at +0x0 and +0x8, and `__declspec(align(4))` is rejected with a usage warning). So the range is
split at that address into two units, each a single array with no interior boundary:

    Runtime.PPCEABI.H/TRK_interrupt_vectors.c        .init 0x80004380..0x80004514  (404 B)
    Runtime.PPCEABI.H/TRK_interrupt_vector_stubs.c   .init 0x80004514..0x800062C0  (7,596 B)

The second unit's claim starts 4-mod-8, which is playbook row 55 - `tools/elf/objalign.py` (chained into
every MWCC rule) lowers the emitted alignment to lowbit(claimed start), so mwld can place it. The map's
`gTRKInterruptVectorTableEnd` (0x800062B4) falls inside the second array and is referenced by nothing,
so no object needs to define it.

Acceptance is not a score: our two objects' `.init` must equal the target's byte for byte, and
`ninja build/RMHE08/ok` must reproduce the DOL hash `bf4850739478caaedfe675949eb7c28595a7fde9`.
"""
import struct
import sys

START = 0x80004380
SIZE = 8000
SPLIT = 0x194  # 0x80004514 - the one address another object references by name

FILES = (
    ("TRK_interrupt_vectors.c", 0x000, SPLIT),
    ("TRK_interrupt_vector_stubs.c", SPLIT, SIZE - SPLIT),
)

HEAD = """/*
 * Runtime.PPCEABI.H/{fname} - {what} of the Metrowerks TRK interrupt-vector image.
 *
 * .init 0x{start:08X}..0x{end:08X} ({size} B), part of the 8,000 B TRK exception-vector table that sits
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
"""


def init_section(path):
    d = open(path, "rb").read()
    shoff, = struct.unpack_from(">I", d, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(">HHH", d, 0x2E)

    def sh(i):
        o = shoff + i * shentsize
        return dict(zip(("name", "typ", "flags", "addr", "off", "size", "link", "info", "align", "entsize"),
                        struct.unpack_from(">10I", d, o)))

    secs = [sh(i) for i in range(shnum)]
    base = secs[shstrndx]["off"]

    def nm(n):
        e = d.index(b"\0", base + n)
        return d[base + n:e].decode()

    sec = next(s for s in secs if nm(s["name"]) == ".init")
    return d[sec["off"]:sec["off"] + sec["size"]]


def main():
    src, out_dir = sys.argv[1], sys.argv[2]
    body = init_section(src)
    assert len(body) == SIZE, "target .init is %d B, expected %d" % (len(body), SIZE)
    for fname, off, size in FILES:
        assert size % 4 == 0, "a u32 array needs a 4-byte multiple"
        name = "gTRKSystemResetVectorSlot" if off == SPLIT else "gTRKInterruptVectorTable"
        what = "the remainder and the handler stubs" if off == SPLIT else "the banner and the reset slot"
        out = [HEAD.format(fname=fname, what=what, start=START + off, end=START + off + size, size=size)]
        words = struct.unpack_from(">%dI" % (size // 4), body, off)
        out.append('__declspec(section ".init") u32 %s[%d] = {' % (name, size // 4))
        for b in range(0, len(words), 8):
            row = words[b:b + 8]
            out.append("    " + " ".join("0x%08X," % x for x in row)
                       + "  /* 0x%08X */" % (START + off + b * 4))
        out.append("};")
        out.append("")
        with open("%s/%s" % (out_dir, fname), "w", encoding="utf-8", newline="\n") as f:
            f.write("\n".join(out))
        print("wrote %s: %s[%d] at 0x%08X (%d B)"
              % (fname, name, size // 4, START + off, size))


if __name__ == "__main__":
    main()
