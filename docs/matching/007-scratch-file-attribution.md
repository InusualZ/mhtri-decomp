---
id: 7
title: Use scratch files to attribute an instruction choice
status: works
problem: Two candidate source idioms produce visibly different instructions, and the full-unit diff cannot tell you whether the difference comes from the source idiom or from an optimizer pass.
tags: [source-shape, tooling]
applies: [Wii/1.3]
demo: 007-scratch-file-attribution.cpp
reviewed: 2026-09-29
related: [3, 5, 8, 21, 39]
---

# 7. Use scratch files to attribute an instruction choice

**Problem.** Two candidate source idioms produce visibly different instructions, and the full-unit diff
cannot tell you whether the difference comes from the source idiom or from an optimizer pass.

**How it looks.** In the unit the target has `rlwinm` (a fused shift-and-mask) where ours has `srwi` + `clrlwi`,
or the reverse, and both the source spelling and the flags are plausible causes.

**Why it happens.** The front end folds some idioms itself while the peephole pass (a late cleanup that fuses
instruction pairs) fuses others, so the same value can come out fused or unfused depending on both the spelling
and the flags. A full unit has too many moving parts to attribute one instruction.

**How to work it.** Compile a ten-line file both ways, under the unit's real flags, and read the two objects. When
one form flips the output you know which pass is responsible, which is exactly the information a flag hunt needs.
Two examples that each pinned a flag: a cast-versus-explicit-mask byte extraction, and a table lookup (data
pooling decides whether the four tables share one `lis` base, idea 43). Compile scratch files with a `-o`
directory of their own (idea 6).

**When NOT to apply.** A scratch file has none of the unit's types, declarations or inlining context, so a
difference that does not show in isolation can still exist in the unit (idea 24); attribution answers "which
pass", not "will the unit match".

**Result.** A symptom attributed to a pass, then to the flag that controls it.

**Example**

```sh
mwcceppc.exe -O3 -opt nopeephole -c build/tmp/scratch/mask.c -o build/tmp/scratch
# -pool off -> 4 separate lis pairs, default -> 1 shared base
grep -c lis build/tmp/scratch/tab.s
```

**Demonstration (finding, 2026-09-29).** `007-scratch-file-attribution.cpp` compiles `(unsigned char)(x >> 16)` and
`(x >> 16) & 0xff` with `-O3 -opt nopeephole` on Wii/1.3. **The direction in the original text was reversed for
this compiler**: the cast stays `srwi r0,r3,16; clrlwi r3,r0,24` (0xC bytes) and the explicit mask is already a
single `rlwinm r3,r3,16,24,31` (0x8 bytes). With the peephole pass on (`-O3`) both spellings give the one
`rlwinm`. So with the peephole off it is the *mask* form that stays fused (the front end folds it), and the cast
form is what the peephole pass would fuse. The method is unchanged; only the example's direction was wrong.
