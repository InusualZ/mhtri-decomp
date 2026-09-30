---
id: 6
title: Guard against stale objects when scripting the compiler
status: works
problem: A scripted matrix run can report all compilers producing *identical* output - which cannot be true. Every run was diffing the same stale object, one the compiler never wrote.
tags: [measurement, tooling]
applies: []
demo:
reviewed: 2026-09-29
related: [1, 4, 14]
---

# 6. Guard against stale objects when scripting the compiler

**Problem.** A scripted matrix run can report all compilers producing *identical* output - which cannot be
true. Every run was diffing the same stale object, one the compiler never wrote.

**How it looks.** Nine compiler versions give byte-identical results, or a source edit changes nothing, or a
"win" appears that vanishes in the real build. `ninja build/RMHE08/report.json` answers "no work to do" after a
source edit and still holds the previous build's scores.

**Why it happens.** Results that are too clean are a bug report about the harness. Two mechanisms: (a) the
object being read is older than the source (objdiff also caches on `(mtime, size)`, and mtimes have whole-second
granularity); (b) the compile wrote somewhere else. MWCC's `-o` takes an output **directory** (ninja passes
`-o build/RMHE08/src/<Unit>`), and a scratch or debug build that keeps the unit's own `-o` directory
**overwrites the unit object** (a `-gdwarf-2` build has the same `.text` but a different hash).

**How to work it.** Delete the object before each compile, wait long enough that the mtimes differ, and fail
loudly when the object was not regenerated. Always redirect `-o` when compiling anything that is not the unit
build, and restore with `rm -f <obj> && ninja <obj>` afterwards. Use the shared guard rather than a private one:
`tools/objdiff/freshguard.py` (report or object older than any source in the unit's include closure) is what
`unitscore.py` and `symdiff.py -u` apply. Any number produced before the guard is worthless.

**When NOT to apply.** A flat matrix is not always a stale object: a unit compiled without the flag that differs
between versions really can be flat (idea 4). Check by deliberately changing something and seeing the object
change.

**Result.** Measurements that can be trusted, and the campaign rule that a tool reporting a score must refuse a
stale artefact.

**Example**

```sh
# -o names a directory base for ninja's build
sjiswrap.exe mwcceppc.exe <flags> -c src/<Unit>/<file>.c -o build/RMHE08/src/<Unit>
```

**Demonstration (finding, 2026-09-29).** The original text claimed that a file-like `-o` path "silently writes
an extension-less file and leaves the real object untouched". On Wii/1.3 here, `-o <dir>/x.o` for a directory
that did not contain `x.o` produced a plain `x.o` (712 B) and `-o <dir>` produced `<dir>/<source stem>.o`. The
extension-less-file behaviour was **not reproduced**; treat it as unconfirmed (it may depend on an existing file
or an older version). The overwrite hazard for a shared `-o` directory is the confirmed one.
