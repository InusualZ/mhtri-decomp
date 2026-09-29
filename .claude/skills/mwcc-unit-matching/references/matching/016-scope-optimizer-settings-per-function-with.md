---
id: 16
title: Scope optimizer settings per function with pragmas
status: works
problem: The optimizer levers are command-line flags, i.e. global: `-opt level=4` fixes one function's frame but reorders instructions elsewhere, so a per-function codegen difference looks unreachable from the source side.
tags: [flags, pragma]
applies: []
demo:
---

# 16. Scope optimizer settings per function with pragmas

**Problem.** The optimizer levers are command-line flags, i.e. global: `-opt level=4` fixes one function's
frame but reorders instructions elsewhere, so a per-function codegen difference looks unreachable from the
source side.

**Why try it.** MWCC pragmas apply from the point where they appear onward, so a pragma *pair* scopes a
setting to a single function - a restore pragma before the next function puts the defaults back. That makes
every global `-opt` lever testable per function, and it is the only way to ask "would this function's
allocation match under a different optimizer setting, without disturbing the rest of the unit?".

**Result.** `#pragma optimization_level 4` immediately before the function, with
`optimization_level 3` / `peephole off` / `scheduling off` before the next one, moved
`camellia_setup256`'s frame from `-0x1e0` to the target's `-0x1d0` while the other nine functions stayed
byte-identical. It also proved the residual is *not* scheduling (level 4 + `scheduling off` is identical to
level 4 alone) and left exactly one 14-instruction window different: 12 rows, a register choice plus where
`CAMELLIA_RL1` is computed. Pragmas that demonstrably change codegen here: `peephole`, `scheduling`,
`optimization_level`, `opt_common_subs`, `opt_propagation`, `opt_lifetimes`. Pragmas that did nothing:
`opt_dead_code`, `opt_dead_store`, `opt_strength_reduction`, `opt_loop_invariants`, `opt_cse`, `opt_global`,
`opt_space`.

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
python tools/flags/tryvar.py -u <unit> v26_pragma_opt4   # frame -0x1d0, 99.32 % (only the window differs)
```

**A pragma is per *function*, not per region** (measured on the same unit, 2026-09-23). The scope looks
like it starts where the pragma is written, but `optimization_level` and the `opt_*` passes are read once
per function: inserting `#pragma optimization_level 3` (or `4`) mid-function changes nothing at all, and the
same is true of `opt_common_subs off` / `opt_propagation off` / `opt_lifetimes off` - 18 marker positions x
both directions and 36 regional `opt_*` variants all produced results byte-identical to the same pragma at
the function head. So "level 4 for the part that needs the frame, level 3 for the part that needs the
order" is not available: a pragma pair scopes a setting to **one whole function**, and two levers that
pull in opposite directions inside one function cannot be separated this way. The window above is also
worth re-locating before rewriting it - the recorded `v40`-`v44` probes rewrote the *first* `tl`/`tr` group
of the function (`sub256(..., count=1)` matches only the first occurrence), while the reorder was in the
*fifth*; the object's own MWCC `.line` section (`u32 size`, then 10-byte `{u32 addr, u32 line, u16 flags}`
records - note `addr2line` cannot read it) is what maps a diff row to a source line. Rewriting the right
group and reusing the rotated value's own variable (`dw = CAMELLIA_RL1(dw);`, which stops the global
optimizer folding the next XOR operand into the tree) took that window from 14 rows to 9.
