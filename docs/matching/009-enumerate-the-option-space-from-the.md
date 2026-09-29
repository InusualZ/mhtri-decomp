---
id: 9
title: Enumerate the option space from the compiler, not from memory
status: works
problem: Guessed spellings waste runs, and worse, they lie: some invented keywords are **silently accepted and ignored**, so "no effect" looks like evidence when it is noise. Others do not parse at all (an `-O` level cannot carry `-opt` keywords).
tags: [flags]
applies: []
demo:
---

# 9. Enumerate the option space from the compiler, not from memory

**Problem.** Guessed spellings waste runs, and worse, they lie: some invented keywords are **silently
accepted and ignored**, so "no effect" looks like evidence when it is noise. Others do not parse at all
(an `-O` level cannot carry `-opt` keywords).

**Why try it.** The compiler ships an authoritative option list, and anything not on it cannot be the
answer - that closes the search space instead of extending it.

**Result.** Dump `-help` into the repo's scratch area, sweep only keywords that exist, and record the
invalid spellings as no-ops so nobody re-runs them.

**Example**

```sh
build/compilers/Wii/1.3/mwcceppc.exe -help > build/tmp/ref/mwcc_help.txt   # then grep for '-opt'
python tools/flags/optsweep.py -u <unit>                                    # filter the whole axis
```
