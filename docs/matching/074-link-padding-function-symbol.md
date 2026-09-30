---
id: 74
title: A data range dtk calls link PADDING needs a `type:function` symbol over the claim
status: works
problem: A byte-identical claim over a range dtk classifies as link padding (`pad_00_80004380_init`) cannot land: a sizeless `label` leaves dtk's pad name in the report, and giving the label an extent makes `dtk dol split` fail with an overlap error.
tags: [linker, symbols]
applies: [Runtime.PPCEABI.H]
demo:
reviewed: 2026-09-29
related: [55, 70, 75]
---

# 74. A data range dtk calls link PADDING needs a `type:function` symbol over the claim

**Problem.** `Runtime.PPCEABI.H/TRK_interrupt_vectors` - the 8,000 B TRK interrupt-vector image in `.init` -
matched byte-for-byte and **could not land**: `dtk dol split` classifies the range as link padding (the
analyzer finds no function prologue in the stubs, which begin with `mtsprg`), so its target object carries
dtk's own symbol `pad_00_80004380_init`. With the map's `gTRKInterruptVectorTable` left as a sizeless `label`,
the claim links and the object is identical, but the report's row keeps dtk's name and nothing can re-measure
it by name; giving that label an extent instead made the **split fail**, with `Symbol
gTRKInterruptVectorTable (0x80004380..0x80004514) overlaps with symbol pad_00_80004380_init`.

**How it looks.** The unit's object is identical, `grep pad_ build/RMHE08/report.json` finds dtk's `pad_*`
row where our name should be, and the land gate's per-symbol re-measure refuses (no map lookup resolves a pad
name) - or the split dies with the overlap message above.

**Why it happens.** dtk's `add_padding_symbols` (`dtk/src/util/split.rs`, lines ~873-945 in the version the
project used) emits `pad_{NN}_{ADDR}_{section}` for any split or gap start that has no symbol whose **kind**
matches, and `type:label` maps to `ObjSymbolKind::Unknown` - so a label can never satisfy a Function pad, and
an `object`-kind symbol does not either (hence the overlap error: dtk had already created the pad). A
**`type:function`** symbol of exactly the claimed extent satisfies it, the pad is never created, and the row
becomes ours. Precedent (a survey of ten dtk-based projects): `mitsevox/tw2004` writes
`TRK_exception_vectors = .init:0x80003534; // type:function size:0x1F34 scope:global`, its comment saying the
function-typed name "lets objdiff compare it". (Other projects check the table in as assembly, accept dtk's
pad name, or leave the range unclaimed. The survey is a local note, `.pi/notes/other-wii-init.md`, which is
gitignored and may be absent in a fresh clone; line numbers above are from that dtk version.)

**How to work it.** In `symbols.txt` give the claim's first symbol `type:function` and the exact extent; split
the claim where a relocation references an interior address (see the alignment fact below); re-run
`dtk dol split` and check that `pad_*` is gone from the report.

**When NOT to apply.** A range dtk did *not* classify as padding needs nothing (`datagap`/`flipcheck` say so).
And nothing here makes a claim over the **linker's own tables** work - see the second fact.

**Result.** Measured at the time (2026-09-28): the two rows of the TRK claim landed as `Object(Matching)`
units, 100.00000 % each, with `grep -c pad_00_80004380_init build/RMHE08/report.json` = **0**, the DOL sha1
unchanged, and the ledger +2 units / +2 closed rows (`4673def1b`). Two facts the same attempt established:

* **MWCC pads every object in `.init` to 8 bytes** - probe: two 5-byte `u8` arrays land at +0x0 and +0x8, and
  `__declspec(align(4))` is rejected with a usage warning. An interior label therefore cannot live inside one
  array object: split the claim **at the referenced address** and let `tools/elf/objalign.py` lower the second
  claim's alignment (idea 55). Ours is two units, 404 B + 7,596 B, because a `.data` word relocates to the
  boundary between them.
* **A claim over the LINKER's own tables is inert by design.** `_rom_copy_info` (0x84) and `_bss_init_info`
  (0x20) are emitted by `mwldeppc`; dtk strips `_eti_init_info|_rom_copy_info|_bss_init_info|_ctors$99|
  _dtors$99` from splits on purpose (`is_linker_generated_object()`), no split target object exists for the
  range, our object never appears in `ldscript.lcf`, and a deliberately corrupted word in our copy leaves the
  DOL byte-identical. No project in the ecosystem defines them. Leave `__start.c`'s `extern`s alone.

**Not a single-object codegen idea, so no demo:** the evidence is dtk's split output and the linked DOL.
