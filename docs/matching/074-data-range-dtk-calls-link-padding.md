---
id: 74
title: A data range dtk calls link PADDING needs a `type:function` symbol over the claim
status: works
problem: `Runtime.PPCEABI.H/TRK_interrupt_vectors` - the 8,000 B TRK interrupt-vector image in `.init` - matched byte-for-byte and **could not land**: `dtk dol split` classifies the range as link padding (the analyzer finds no function prologue in the stubs, which begin with `mtsprg`), so its target object carries dtk's own symbol `pad_00_80004380_init`. With the map's `gTRKInterruptVectorTable` left as a sizeless `label`, the claim links and the object is identical, but the report's row keeps dtk's name and nothing can re-measure it by name; giving that label an extent instead made the **split fail**, with `Symbol gTRKInterruptVectorTable (0x80004380..0x80004514) overlaps with symbol pad_00_80004380_init`.
tags: [symbols, linker, sections, data]
applies: []
demo:
---

# 74. A data range dtk calls link PADDING needs a `type:function` symbol over the claim

**Problem.** `Runtime.PPCEABI.H/TRK_interrupt_vectors` - the 8,000 B TRK interrupt-vector image in `.init` -
matched byte-for-byte and **could not land**: `dtk dol split` classifies the range as link padding (the
analyzer finds no function prologue in the stubs, which begin with `mtsprg`), so its target object carries
dtk's own symbol `pad_00_80004380_init`. With the map's `gTRKInterruptVectorTable` left as a sizeless `label`,
the claim links and the object is identical, but the report's row keeps dtk's name and nothing can re-measure
it by name; giving that label an extent instead made the **split fail**, with `Symbol
gTRKInterruptVectorTable (0x80004380..0x80004514) overlaps with symbol pad_00_80004380_init`.

**Why try it.** `dtk/src/util/split.rs:873-945` (`add_padding_symbols`) emits `pad_{NN}_{ADDR}_{section}` for
any split or gap start that has no symbol whose **kind** matches (`kind_at_section_address`,
`src/obj/symbols.rs:380`), and `type:label` maps to `ObjSymbolKind::Unknown` (`config.rs:377`) - so a label
can never satisfy a Function pad, and an `object`-kind symbol does not either (hence the overlap error: dtk
had already created the pad). A **`type:function`** symbol of exactly the claimed extent satisfies it, the pad
symbol is never created, and the row becomes ours. Precedent found by surveying ten dtk-based projects:
`mitsevox/tw2004` does exactly this -
`TRK_exception_vectors = .init:0x80003534; // type:function size:0x1F34 scope:global` - and its own comment
says the function-typed name "lets objdiff compare it". (`doldecomp/melee`, `zeldaret/tww` and others instead
check in the vector table as assembly, or accept dtk's pad name; `doldecomp/ogws` leaves it unclaimed
entirely. Full survey: `.pi/notes/other-wii-init.md`.)

**Result.** The two rows of the TRK claim landed as `Object(Matching)` units, 100.00000 % each, with
`grep -c pad_00_80004380_init build/RMHE08/report.json` = **0**, the DOL sha1 unchanged, and the ledger
+2 units / +2 closed rows (`4673def1b`). Two facts the same attempt established, both worth knowing before
the next data claim:

* **MWCC pads every object in `.init` to 8 bytes** - probe: two 5-byte `u8` arrays land at +0x0 and +0x8, and
  `__declspec(align(4))` is rejected with a usage warning. An interior label therefore cannot live inside one
  array object: split the claim **at the referenced address** and let `tools/elf/objalign.py` lower the second
  claim's alignment (row 55). Ours is two units, 404 B + 7,596 B, because a `.data` word relocates to the
  boundary between them.
* **A claim over the LINKER's own tables is inert by design.** `_rom_copy_info` (0x84) and `_bss_init_info`
  (0x20) are emitted by `mwldeppc`; dtk strips `_eti_init_info|_rom_copy_info|_bss_init_info|_ctors$99|
  _dtors$99` from splits on purpose (`is_linker_generated_object()`), no split target object exists for the
  range, our object never appears in `ldscript.lcf`, and a deliberately corrupted word in our copy leaves the
  DOL byte-identical. No project in the ecosystem defines them - only `extern`s and non-mwld link stubs. Leave
  `__start.c`'s `extern`s alone.
