---
id: 16
title: Scope optimizer settings per function with pragmas
status: works
problem: The optimizer levers are command-line flags, i.e. global: `-opt level=4` fixes one function's frame but reorders instructions elsewhere, so a per-function codegen difference looks unreachable from the source side.
tags: [pragma, flags]
applies: [Wii/1.3, GC/3.0a3]
demo: 016-per-function-pragmas.cpp
reviewed: 2026-09-29
related: [21, 32, 33, 39, 41, 61]
---

# 16. Scope optimizer settings per function with pragmas

**Problem.** The optimizer levers are command-line flags, i.e. global: `-opt level=4` fixes one function's
frame but reorders instructions elsewhere, so a per-function codegen difference looks unreachable from the
source side.

**How it looks.** One function of a unit is off by a frame size, an unrolled loop or a fused instruction,
and every other function of the unit already matches. Changing the unit's flag fixes the one function and
breaks its neighbours.

**Why it works.** MWCC has a `#pragma` twin for most `-opt` switches. A pragma takes effect from the point
where it is written, so a pragma *pair* (set before the function, restore after it) scopes a setting to a
single function. That makes every global `-opt` lever testable per function: "would this function match
under a different optimizer setting, without disturbing the rest of the unit?".

**How to work it.**

1. Put the candidate pragma directly before the function and the restoring pragma directly after it.
2. Re-measure the whole unit, not only the function (idea 32: a pragma can leak into what follows).
3. If the whole unit wants the setting, promote it to the library's flags instead (idea 33).

**Pragmas that change codegen here** (measured on `Camellia`): `peephole`, `scheduling`,
`optimization_level`, `opt_common_subs`, `opt_propagation`, `opt_lifetimes`. **Pragmas that parsed and did
nothing:** `opt_dead_code`, `opt_dead_store`, `opt_strength_reduction`, `opt_loop_invariants`, `opt_cse`,
`opt_global`, `opt_space` (and `opt_peephole`/`peep`, see idea 28). Never assume a pragma works because it
parses.

**A pragma scopes one whole *function*, not a region.** `optimization_level` and the `opt_*` passes are read
once per function: writing `#pragma optimization_level 1` in the middle of a body gives the same object as
writing it above the function (the demo compiles both: 0x2C bytes each, against 0x88 at level 4). So
"level 4 for the part that needs the frame, level 3 for the part that needs the order" is not available; two
levers pulling in opposite directions inside one function cannot be separated this way. (`peephole` is the
other kind: it is a per-function toggle that is visibly restored by the next pragma, and the demo shows the
record-form instructions coming and going with it.)

**Example**

```c
#pragma optimization_level 4
void camellia_setup256(const unsigned char *key, u32 *subkey) { ... }

#pragma optimization_level 3
#pragma peephole off
#pragma scheduling off
void camellia_setup192(const unsigned char *key, u32 *subkey) { ... }
```

```sh
python tools/flags/tryvar.py -u Camellia/camellia v26_pragma_opt4   # variant file: tools/flags/variants/camellia.py
```

**Demonstration.** `016-per-function-pragmas.cpp` (`ideas.py demo-check 16`): the same loop compiled three times
in one file - `#pragma peephole off` removes the record-form `srawi.`/`srwi.` from that one function and
`#pragma peephole on` brings them back for the next; `optimization_level 1` turns the eight-fold unrolled loop
(0x88 B) into 0x2C B, and gives the same 0x2C B whether written above the function or mid-body.

**When NOT to apply.** A function-scoped pragma is a fix for one function. If several functions of a unit want
the same setting the unit was probably built with it: use the library flags (idea 33), which is also the
faithful reconstruction (a translation unit was compiled once). `#pragma optimization_level 1` does not turn
the peephole off (idea 41).

**Evidence** (`Camellia`, dated 2026-09-22/23). `#pragma optimization_level 4` immediately before
`camellia_setup256`, with `optimization_level 3` / `peephole off` / `scheduling off` before the next function,
moved the frame from `-0x1e0` to the target's `-0x1d0` while the other nine functions stayed byte-identical
(variant `v26_pragma_opt4`, 99.32 %). It proved the residual is *not* scheduling (level 4 + `scheduling off` is
identical to level 4 alone) and left a 14-instruction window (12 rows: a register choice plus where
`CAMELLIA_RL1` is computed). The mid-function test used 18 marker positions x both directions and 36 regional
`opt_*` variants; all were byte-identical to the pragma at the function head. The window is best located with
the object's own MWCC `.line` section (`u32 size`, then 10-byte `{u32 addr, u32 line, u16 flags}` records -
`addr2line` cannot read it) that maps a diff row to a source line; the recorded `v40`-`v44` probes rewrote the
*first* `tl`/`tr` group while the reorder was in the *fifth*, and rewriting the right group with the rotated
value in its own variable (`dw = CAMELLIA_RL1(dw);`) took the window from 14 rows to 9.
