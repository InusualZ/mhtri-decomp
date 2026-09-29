---
id: 6
title: Guard against stale objects when scripting the compiler
status: works
problem: A scripted matrix run can report all compilers producing *identical* output - which cannot be true. Every run was diffing the same stale object, one the compiler never wrote.
tags: [measurement, tooling]
applies: []
demo:
---

# 6. Guard against stale objects when scripting the compiler

**Problem.** A scripted matrix run can report all compilers producing *identical* output - which cannot be
true. Every run was diffing the same stale object, one the compiler never wrote.

**Why try it.** Results that are too clean are a bug report about the harness, not a finding about the
compiler. MWCC's `-o` takes an output **directory** (ninja passes `-o build/RMHE08/src/<Unit>`); given a
file-like path it silently writes an extension-less file and leaves the real object untouched.

**Result.** Delete the object before each compile, wait long enough that the filesystem's whole-second
mtimes differ (objdiff caches on `(mtime, size)`), and fail loudly when the object was not regenerated. Any
number produced before that fix is worthless. The same trap has a second form: a scratch or debug build that
keeps the unit's own `-o` directory **overwrites the unit object** (a `-gdwarf-2` build has the same `.text`
but a different hash), so always redirect `-o` when compiling anything that is not the unit build, and
restore with `rm -f <obj> && ninja <obj>` afterwards.

**Example**

```sh
# -o is a directory base, not a file
sjiswrap.exe mwcceppc.exe <flags> -c src/<Unit>/<file>.c -o build/RMHE08/src/<Unit>
```
