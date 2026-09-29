---
id: 55
title: An odd-start section claim cannot be linked with MWCC's alignment
status: works
problem: A unit whose object is byte-identical to its target still refuses to flip: `flipcheck.py` says READY, the bytes match, and `ninja build/RMHE08/ok` still fails with a DOL in which the tables are four bytes late. It reads as link order or a misplaced `.data` boundary, and neither a source rewrite nor a flag spelling moves it.
tags: [sections, flags, source-shape, data]
applies: []
demo:
---

# 55. An odd-start section claim cannot be linked with MWCC's alignment

**Problem.** A unit whose object is byte-identical to its target still refuses to flip: `flipcheck.py` says
READY, the bytes match, and `ninja build/RMHE08/ok` still fails with a DOL in which the tables are four bytes
late. It reads as link order or a misplaced `.data` boundary, and neither a source rewrite nor a flag spelling
moves it.

**Why try it.** MWCC writes one alignment per section regardless of where the linker script puts it
(`sh_addralign = 8` for `.data`), but `splits.txt` may claim that section at a 4-mod-8 address - retail really
has such units. mwld cannot honour the claim then: it rounds the section up to the next 8-byte boundary and
everything after it shifts, while the *object* stays byte-identical, so every "is the unit complete?" check
says yes.

**Result.** Compare the section's alignment in our object against dtk's **target** object
(`build/RMHE08/obj/<unit>.o`). `dol split` already normalises the target to what the address can honour, so if
the target says 4 and ours says 8, the object is complete and the fix is a post-compile step, not source:
lower the emitted section's `sh_addralign` to `lowbit(claimed start)` - never raise it. This is implemented in
`tools/elf/objalign.py`, chained into every MWCC rule by `tools/project.py` after `dtk extab clean` (the two
rules without a chain of their own need `CHAIN`, i.e. `cmd /c`, or MWCC is handed `&&` as a file argument). It
is a strict lowering, so it is a no-op for every unit whose starts are aligned already, and a full rebuild with
no flips is its regression proof.

**Example.** `Pl/fn_8023C2D0`: ours `.data` size 0x84C align 8, target size 0x84C align **4** (the claim starts
at 0x805C34D4). Setting that one field to 4 in a scratch copy relinked `main.dol` byte for byte. With the step
in place the flip links green, and `Pl/fn_80230FBC` with it. `Pl/fn_802373AC`, `Pl/fn_802430E8` and
`enemy/fn_80165FC8` have the same odd start but real `.text` residuals, so their alignment is already right and
their code is not - one measurement tells the two apart, and `flipcheck.py` still refuses them for the code.

**Refined (2026-09-28) - the alignment question is NOT a refusal, and `objalign.py` is about matching the
layout.** `tools/mwlink_debugger.py` reads the linker's own round-up site (RVA 0x57451 loads the input
section's `sh_addralign`, 0x57466/0x5747d do `(addr + align - 1) & ~(align - 1)`, and 0x57492 pushes the
`*fill*` literal) and the differential is conclusive: take `build/RMHE08/src/Pl/fn_8023C2D0.o` (whose `.data`
claims `0x805C34D4`, 4 mod 8), copy it, set the allocatable sections' `sh_addralign` to 8 in one copy and
relink both:

| copy | `.data` row | residue |
| --- | --- | --- |
| `align 4` (what `objalign.py` leaves) | off `+0x46cb4`, addr `0x805c34d4` | none |
| `align 8` (what MWCC emits) | off `+0x46cb8`, addr `0x805c34d8` | a 4-byte `*fill*` at `0x805c34b8`...`0x805c34d8` |
| `extab` row | `0x80011cb4` -> `0x80011cb8` | moved 4 with it |

**No diagnostic, no error, exit 0 both times** - which is exactly why it reads as a link-order mystery. So
mwld never refuses: it silently aligns a fragment's address up to **the input section's own**
`sh_addralign` and prints a `*fill*` row for the residue. The check over the whole link (the 2296 input
objects, every allocatable section whose name is unique in its object and whose start `splits.txt` claims)
compares **798 sections and finds 0 that cannot be honoured** - the one ambiguity is
`build/RMHE08/obj/fn_80429B94.o`, which carries 7 sections called `.data`, and `align` says so rather than
comparing them all against one address. So the row is not "the claim cannot be linked": every claim is
honourable once the object's `sh_addralign` agrees with the address `lowbit(claimed start)` allows, and
`align --unit <unit>` reports the condition from the linker's side.
