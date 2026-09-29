---
id: 27
title: Same instructions, different order names the `-O` level - probe both, per unit
status: works
problem: the instruction multiset and the relocations agree, but independent instructions are swapped or split across registers - typically the epilogue's `lwz r0,0x14(r1)` (LR reload) and the function's real last load. It reads like a scheduling residual with no lever, and source rewrites do nothing.
tags: [source-shape, allocator, symbols, relocations]
applies: []
demo:
---

# 27. Same instructions, different order names the `-O` level - probe both, per unit

Problem: the instruction multiset and the relocations agree, but independent instructions are swapped or
split across registers - typically the epilogue's `lwz r0,0x14(r1)` (LR reload) and the function's real last
load. It reads like a scheduling residual with no lever, and source rewrites do nothing.

Why try it: the schedule is a property of the `-O` level, and the project default (`-O4,p` in `cflags_base`)
is not what every unit wants. It is also **not one setting for the build**: of the six units reconstructed so
far, `-O3` is right for Camellia, RSO, `g3d` and `lobby`, while `-O4,p` is right for `OS/OSAlarm.c` and
`NetworkWiiMediator.c` - under the wrong one the instructions come out reordered (g3d, lobby) or with the
`li r0,-1` no longer hoisted above the stores (OSAlarm).

Why the probe comes first: the target function can be diffed **before** the unit is registered, which saves a
re-split (minutes) per guess. It sits inside the `auto_*` object covering its region at `offset = function
address - blob base`, and the base is in the object's own name - region blobs are `auto_<n>_<BASE>_text.o`,
per-function objects are `auto_fn_<ADDR>_text.o`:

```sh
# auto_03_80458A60_text.o covers 0x80458A60..., so 0x804CBC50 is at offset 0x731F0
build/binutils/powerpc-eabi-objdump.exe -d --section=.text build/RMHE08/obj/auto_03_80458A60_text.o
#     its window for 0x804CBC50-0x804CBC60 is offset 0x731F0-0x73200
```

Compile the candidate source under both levels into a scratch directory and compare that window: the level
that reproduces it byte-for-byte (mnemonic + operands, reloc names normalised) is the one for the per-library
`cflags_*` override.

Example: `src/g3d/g3d_anmscn.cpp` (`fn_800680A8__FPv`, 0x24 B / 9 instructions; `g3d_resanmamblight.c`
until the wQ-recut re-homed it, and the map name gained its argument list in the wR-mangling pass - playbook 48).
Under `cflags_base`
(`-O4,p`) the unit measured 97.56 % with `lwz r0,0x14(r1)` before `lwz r3,0xc(r3)`; the same source under
`-O3` is byte-identical (100 %), now `cflags_g3d`. The same two-variant probe then landed
`src/OS/OSAlarm.c` (`-O4,p` identical, `-O3` differs) and `src/lobby/lobby_scene.c` (`-O3` identical) at
100 % on their first registration. `tools/flags/optsweep.py` cannot see any of this: it reports frame sizes,
so a frame-equal sweep row is not evidence about instruction order.

The level also decides **function packing**, which is a second and independent reason to probe it: `-O4,p`
implies `-func_align 16`, so under it every function after the first moves to the next 16-byte boundary. Which
one a unit wants is visible in its target object - `Runtime.PPCEABI.H`'s five objects are all `.init
align 2**2` and `memcpy.o` packs two functions contiguously, so `-func_align 4` was required there (it is a
per-lib flag now, `cflags_ppceabi`, with `#pragma function_align 4` in `src/Runtime.PPCEABI.H/memcpy.c` as its
first witness; `__start.o` in the same lib does want 16-byte function starts, which come from each function's
own `#pragma section code_type ".init"` section, not from the function alignment). A `-O` change that fixes an
instruction order but moves every symbol after the first is not a fix: check the section alignment and the
function offsets too.

The same flag bites *inside* a function: MWCC aligns loop heads to 8 bytes relative to the object's `.text`
start, so 16-byte packing made two `Gecko_ExceptionPPC.cp` functions carry a `nop` before their loop
(`ExPPC_FindExceptionRecord` 99.07 -> 100.00 with `-func_align 4`), and left 4-8 bytes of padding between the
functions of two other objects in that lib (`global_destructor_chain` `.text` 0x20 vs 0x18,
`__init_cpp_exceptions` 0x74 vs 0x70). Three independent witnesses to one flag is what makes the re-split worth
it.
