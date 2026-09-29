---
id: 21
title: Count record-form instructions to fingerprint peephole/scheduling per unit
status: works
problem: The flags of one unit were inferred for the whole binary from "the DOL contains no `extrwi`" - wrong twice over: it is not a whole-binary property, and the detector could not see what it was looking for.
tags: [flags, process]
applies: []
demo:
---

# 21. Count record-form instructions to fingerprint peephole/scheduling per unit

**Problem.** The flags of one unit were inferred for the whole binary from "the DOL contains no `extrwi`" -
wrong twice over: it is not a whole-binary property, and the detector could not see what it was looking
for.

**Why try it.** MWCC emits record-form instructions (`srwi.`, `add.`, `extsb.`, `clrrwi.`) only with the
peephole pass on, and their count is a *per-unit* property of the retail bytes. Count them on the target
object before deciding a unit's `-opt` flags; they are the fingerprint the `-opt` axis actually leaves.

**Result.** The `RSO/runtime` target has 9 record forms (`srwi.` x4, `add.` x3, `extsb.`, `clrrwi.`) and
needs `-opt peephole,schedule,level=4`; the `Camellia` target has 0 and needs `-opt nopeephole`. The same
count exposes the alias trap: **GNU objdump never prints `extrwi`** - it prints the underlying
`rlwinm rX,rY,SH,MB,ME` - so "0 `extrwi` in 1 357 339 instructions" said nothing at all (a raw-word scan
finds 5 386 extrwi-form words). Any fingerprint built on a disassembler *alias* is a fingerprint of the
disassembler, not of the build.

**Example**

```sh
powerpc-eabi-objdump -d build/RMHE08/obj/<Unit>.o \
  | grep -oE "(srwi\.|slwi\.|add\.|subf\.|extsb\.|clrrwi\.|and\.|or\.)" | sort | uniq -c
```
