---
id: 21
title: Count record-form instructions to fingerprint peephole/scheduling per unit
status: works
problem: A unit's `-opt` flags were inferred for the whole binary from "the DOL contains no `extrwi`" - wrong twice over: it is not a whole-binary property, and the detector could not see what it was looking for.
tags: [flags, measurement]
applies: [Wii/1.3, GC/3.0a3]
demo:
reviewed: 2026-09-29
related: [16, 39, 40, 41, 86]
---

# 21. Count record-form instructions to fingerprint peephole/scheduling per unit

**Problem.** The flags of one unit were inferred for the whole binary from "the DOL contains no `extrwi`" -
wrong twice over: it is not a whole-binary property, and the detector could not see what it was looking for.

**How it looks.** You have to decide a unit's `-opt` flags (`peephole` on or off, the level) before any source
exists to diff, or a diff shows one extra `clrlwi`/`cmpwi` and you cannot tell which switch explains it.

**Why it works.** A *record-form* instruction (a mnemonic with a trailing dot: `srwi.`, `add.`, `extsb.`,
`clrrwi.`) also updates condition register 0, so a following `cmpwi r,0` is unnecessary. MWCC folds the compare
into the instruction only with the peephole pass on, so the count of record forms is a *per-unit* property of
the retail bytes and the fingerprint the `-opt` axis actually leaves. Count them on the target object before
deciding a unit's flags.

**How to work it.**

```sh
build/binutils/powerpc-eabi-objdump.exe -d build/RMHE08/obj/<Unit>.o \
  | grep -oE "(srwi\.|srawi\.|slwi\.|add\.|subf\.|extsb\.|clrrwi\.|and\.|or\.)" | sort | uniq -c
```

Zero record forms in a unit with loops and `if (x)` tests on computed values means the peephole pass was off
(idea 39: `-opt nopeephole`); a handful means it was on. Then confirm on your own object with the same command
before believing it.

**Result** (measured at the time). The `RSO/runtime` target has 9 record forms (`srwi.` x4, `add.` x3, `extsb.`,
`clrrwi.`) and needs `-opt peephole,schedule,level=4` (re-run today on the current target object: the same 9);
the `Camellia` target has 0 and needs `-opt nopeephole`. The same count exposes the **alias trap**: GNU objdump
never prints `extrwi` - it prints the underlying `rlwinm rX,rY,SH,MB,ME` - so "0 `extrwi` in 1 357 339
instructions" said nothing at all (a raw-word scan finds 5 386 extrwi-form words). Any fingerprint built on a
disassembler *alias* is a fingerprint of the disassembler, not of the build (idea 86).

**Demonstration.** No demo of its own: the effect is shown by idea 16's `016-per-function-pragmas.cpp`, where
the same loop has 1 `srawi.` with the peephole on and 0 with `#pragma peephole off` (then a separate
`srawi` + `cmpwi`).

**When NOT to apply.** A unit with no branches on computed values has no record forms either way, so a zero
count is then not evidence; and a peephole pragma scoped to individual functions (idea 16) makes the count a
per-function property.

**Evidence.** `RSO/runtime` (9) and `Camellia` (0) as above, dated 2026-09-2x.
