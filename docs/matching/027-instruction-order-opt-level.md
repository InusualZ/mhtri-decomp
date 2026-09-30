---
id: 27
title: Same instructions, different order names the `-O` level - probe both, per unit
status: works
problem: The instruction multiset and relocations agree, but independent instructions are swapped or split across registers (typically the epilogue's `lwz r0,0x14(r1)` and the last real load); source rewrites do nothing.
tags: [flags, allocator]
applies: [Wii/1.3]
demo: 027-instruction-order-opt-level.cpp
reviewed: 2026-09-29
related: [5, 16, 28, 33, 39, 40]
---

# 27. Same instructions, different order names the `-O` level - probe both, per unit

**Problem.** The instruction multiset and the relocations agree, but independent instructions are swapped or
split across registers - typically the epilogue's `lwz r0,0x14(r1)` (the link-register reload) and the
function's real last load. It reads like a scheduling residual with no lever, and source rewrites do nothing.

**How it looks.** A small function is one or two rows off at 90-98 %, and the two objects contain the same
instructions in a different order - e.g. retail loads the return value *before* `lwz r0,0x14(r1)`, ours after.

**Why it works.** The schedule is a property of the `-O` level, and the project default (`-O4,p` in
`cflags_base`) is not what every unit wants. It is also **not one setting for the build**: of the units
reconstructed when this was written, `-O3` was right for Camellia, RSO, `g3d` and `lobby`, while `-O4,p` was
right for `OS/OSAlarm.c` and `NetworkWiiMediator.c` - under the wrong one the instructions come out reordered
(g3d, lobby) or with the `li r0,-1` no longer hoisted above the stores (OSAlarm).

**How to work it.** Compile the candidate source under both levels into a scratch directory and compare the
window: the level that reproduces it byte-for-byte (mnemonic + operands, reloc names normalised) is the one for
the per-library `cflags_*` override. **The probe comes first**: the target function can be diffed *before* the
unit is registered, which saves a re-split (minutes) per guess. It sits inside the region object covering it at
`offset = function address - blob base`, the base being in the object's own name (`auto_<n>_<BASE>_text.o`
under `build/RMHE08/obj/`, per-function objects `auto_fn_<ADDR>_text.o`):

```sh
# auto_03_80458A60_text.o covers 0x80458A60..., so 0x804CBC50 is at offset 0x731F0
build/binutils/powerpc-eabi-objdump.exe -d --section=.text build/RMHE08/obj/auto_03_80458A60_text.o
#     its window for 0x804CBC50-0x804CBC60 is offset 0x731F0-0x73200
```

**The level also decides function packing** - a second, independent reason to probe it: `-O4,p` implies
`-func_align 16`, so under it every function after the first moves to the next 16-byte boundary. Which one a unit
wants is visible in its target object (the section's alignment and the function offsets): `Runtime.PPCEABI.H`'s
objects are all `.init align 2**2` and `memcpy.o` packs two functions contiguously, so `-func_align 4` was
required there (a per-lib flag now, `cflags_ppceabi`; `#pragma function_align 4` in `memcpy.c` was its first
witness; `__start.o` in the same lib wants 16-byte function starts, which come from each function's own `#pragma
section code_type ".init"` section). A `-O` change that fixes an instruction order but moves every symbol after
the first is not a fix: check section alignment and function offsets too. The same flag bites *inside* a
function: MWCC aligns loop heads to 8 bytes relative to the object's `.text` start, so 16-byte packing made two
`Gecko_ExceptionPPC.cp` functions carry a `nop` before their loop (`ExPPC_FindExceptionRecord` 99.07 -> 100.00 with
`-func_align 4`) and left 4-8 bytes of padding between the functions of two other objects in that lib.

**Demonstration.** `027-instruction-order-opt-level.cpp` (`ideas.py demo-check 27`) reproduces the *packing*:
three 8-byte functions at `-O4,p` land at 0x0/0x10/0x20 (`.text` 0x60 with a 0x30-byte function after them),
while at `-O3` (or `-O4,p -func_align 4`) the same source is 0x0/0x8/0x10 and `.text` is 0x48. It does **not**
reproduce the *order* effect: the small call-then-load function (`bl poke; lwz r3,8(r31); lwz r31,12(r1); lwz
r0,20(r1)`) is the same 12 instructions in the same order at both levels. So the reorder in the evidence below
needs the particular shape of the retail function (or the wrapper's aggregate), not any function with a reload
after a call; expect to probe on the real body.

**When NOT to apply.** `tools/flags/optsweep.py` cannot see any of this: it reports frame sizes, so a frame-equal
sweep row is not evidence about instruction order. If a global level change fixes one function and breaks others,
it is not the unit's level: use a function-scoped pragma (idea 16) - but prefer the unit's flags if most functions
want it (idea 33), and see idea 39/40 for the peephole and fused-multiply-add switches that also reorder code.

**Evidence** (dated 2026-09-2x). `src/g3d/g3d_anmscn.cpp` (`fn_800680A8__FPv`, 0x24 B / 9 instructions): under
`-O4,p` the unit measured 97.56 % with `lwz r0,0x14(r1)` before `lwz r3,0xc(r3)`; the same source under `-O3` is
byte-identical (100 %), now `cflags_g3d` (which today also carries `-inline noauto`, see idea 28). The same
two-variant probe landed `src/OS/OSAlarm.c` (`-O4,p` identical, `-O3` differs) and `src/lobby/lobby_scene.c`
(`-O3` identical) at 100 % on their first registration.
