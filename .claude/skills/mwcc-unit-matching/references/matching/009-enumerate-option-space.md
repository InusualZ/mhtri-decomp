---
id: 9
title: Enumerate the option space from the compiler, not from memory
status: works
problem: Guessed spellings waste runs, and worse, they lie: some invented keywords are **silently accepted and ignored**, so "no effect" looks like evidence when it is noise. Others do not parse at all (an `-O` level cannot carry `-opt` keywords).
tags: [flags, tooling]
applies: [Wii/1.3]
demo:
reviewed: 2026-09-29
related: [5, 8, 41]
---

# 9. Enumerate the option space from the compiler, not from memory

**Problem.** Guessed spellings waste runs, and worse, they lie: some invented keywords are **silently accepted and
ignored**, so "no effect" looks like evidence when it is noise. Others do not parse at all (an `-O` level cannot
carry `-opt` keywords).

**How it looks.** A sweep reports "no change" for a keyword that does not exist, and the result is recorded as
"this flag does not help".

**Why it happens.** The compiler's error handling differs per option: some unknown spellings are a hard error,
some only a warning on stderr that a script may discard. Measured on Wii/1.3 (2026-09-29): `-O3,level=2` is an
error (`Unknown option 'level'; expected one of '0, 1, 2, 3, 4, p, or s'`, exit 1), while `-opt bogus_kw` is only
a *Usage Warning* that lists every valid keyword and the compile still succeeds (exit 0). A harness that ignores
stderr therefore sees a clean run with the keyword doing nothing.

**How to work it.** The compiler ships an authoritative option list (`-help`, and the warning above prints the
valid `-opt` set); anything not on it cannot be the answer. Dump `-help` into the scratch area, sweep only
keywords that exist, treat any stderr from a sweep run as a failure, and record invalid spellings as no-ops so
nobody re-runs them.

**When NOT to apply.** A *valid* keyword that has no effect on this unit is also "no change" but is real
evidence (idea 5); only invalid spellings are noise. Confirm which case it is with `-opt display` (idea 8).

**Result.** Sweeps over spellings the compiler actually accepts, with invalid ones closed out.

**Example**

```sh
build/compilers/Wii/1.3/mwcceppc.exe -help > build/tmp/ref/mwcc_help.txt   # then grep for '-opt'
python tools/flags/optsweep.py -u <unit>                                    # filter the whole axis
```

**Evidence / open.** Which other invented spellings are accepted with no warning at all (as the original text
claimed) was not re-measured here; the `-opt` keyword case warns. If a silent case is found, record the exact
flag and version here.
