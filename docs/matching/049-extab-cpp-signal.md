---
id: 49
title: `extab` in a no-exceptions lib is a cheap C++ language signal
status: works
problem: A unit's language (`.c` vs `.cpp`) must come from evidence, but the conclusive signals (a mangled definition, a `.cpp` `__FILE__`) may be absent at registration; an `extab` section is a cheap hint, valid only as a hint (measured: it follows the exception mode, not the language).
tags: [sections, flags]
applies: []
demo: 049-extab-cpp-signal.cpp
reviewed: 2026-09-29
related: [30, 42, 48, 62, 83]
---

# 49. `extab` in a no-exceptions lib is a cheap C++ language signal

**Problem.** A unit's language has to be decided from evidence, not from convenience (`docs/plan.md`, "The
language comes from the symbol"), because the extension decides the front-end (`-lang`) and the name objdiff pairs
by. The two conclusive signals - a mangled definition, a `__FILE__` string that names a `.cpp` - may both be
**absent** at the moment a unit is registered, so the `.c`/`.cpp` choice is a guess that a later pass has to undo
(ideas 42 and 48 are the two ends of getting it wrong).

**How it looks.** The unit's split target object carries `extab` and `extabindex` sections (`elfsect.py
build/RMHE08/obj/<unit>.o`) while nothing else names a language.

**Why it works, and how far.** `extab`/`extabindex` are the exception-unwind tables MWCC emits when exception
handling is on. The argument "C has no exceptions" is only a heuristic: **measured 2026-09-29, the sections follow
the exception MODE, not the language.** With `-Cpp_exceptions off`:

| source | switch | `extab` |
| --- | --- | --- |
| C++ function with a destructor-bearing local | none | **absent** (0) |
| C++ function with a destructor-bearing local | `#pragma exceptions on` (or `-Cpp_exceptions on`) | 0x18 + `extabindex` 0xC |
| plain **C** function (`-lang c`) | `#pragma exceptions on` | 0x8 + `extabindex` 0xC |
| plain **C** function (`-lang c`) | `-Cpp_exceptions on` | 0x8 + `extabindex` 0xC |

So in a lib whose flags leave exceptions off, an `extab` object means exceptions were switched on **locally** (a
file-level `#pragma exceptions on`, as ideas 30/62 need for `new`/destructors) - usually because the code is C++,
but a C unit can do it too, which is why the signal is only ever *suggested*, never conclusive. In a lib whose
`cflags` set `-Cpp_exceptions on` (`cflags_pl`, `cflags_main`, `cflags_g3d`, `cflags_camellia`) a **C** unit emits
`extab` as well, so the section says nothing there. And it is **one-directional**: a C++ file with no `try`/
`catch`/`throw`/destructor-bearing object under exceptions-off emits no `extab` (the first table row), so the
section's *absence* is not evidence of C.

**How to work it.** `python tools/units/langcheck.py --disagree` sweeps every registered unit; `--unit <path>`
shows one. `langcheck` reads the target object's sections, threads the lib's exception setting in
(`cflags_exceptions`), and reports an `extab` object in a no-exceptions lib as **suggested C++** (`medium`,
`conclusive: False`) beside the mangled-callee signal: listed with its reason, keeps the extension, never renames on
its own; an exceptions-ON lib silences it. Use it to find a wrong extension the moment a unit is registered in one
of the no-exceptions libs (`Network`, `OS`, `RSO`, `Runtime.PPCEABI.H`, `Runtime.PPCEABI.H/init`, `lobby`).

**Result (measured when written, 2026-09).** 54 units: **43** objects carried `extab`, **0** decisive candidates
(every one of the 25 `.c` units with `extab` sat in `auto`/`main`/`g3d`/`Camellia`, which enable exceptions), 18
`.cpp` units consistent with it, and **2** contradictions (`__ppc_eabi_init.cpp`, `__init_cpp_exceptions.cpp`:
registered `.cpp`, no `extab`, no mangled symbol - the C++-without-exceptions the one-directional rule predicts).
Re-run 2026-09-29 on the whole registered tree: **263** objects carry `extab`; **0** decisive candidates, **19**
confounded (lib enables exceptions), **6** contradictions. The yield is still nil, and that is the honest number;
the value is prospective.

**When NOT to apply.** Never rename a unit's extension on this signal alone; never infer C from its absence; and
never read it in an exceptions-on lib. Idea 83 records why the flag itself is not the lever for matching (the flag
adds the sections; a `new` expression also moves `.text`, idea 62).

**Example.**

```sh
python tools/units/langcheck.py --disagree       # the sweep, including the extab buckets
python tools/units/langcheck.py --unit RSO/runtime
```

The selftest pins the three properties that make the signal safe: an `extab` object in a no-exceptions lib is
C++/medium and **never conclusive**; a lib that enables `-Cpp_exceptions` silences it; and an object with no
`extab` is not evidence of C.

**Demonstration.** `049-extab-cpp-signal.cpp` (`ideas.py demo-check 49`) compiles a **C** function
(`-lang c`, `-Cpp_exceptions off`) under `#pragma exceptions on` and asserts `extab` 8 B / `extabindex` 12 B: the
sections are produced by the exception mode alone, so the "C has no exceptions" premise is scoped to "C does not
usually turn them on". (The C++ rows of the table above were measured by hand with the same demo machinery and are
not encoded: a demo carries one flag set.)
