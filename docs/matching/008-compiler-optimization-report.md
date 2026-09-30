---
id: 8
title: Ask the compiler which optimizations are actually on
status: works
problem: After a long sweep it is still unclear what a flag *set* resolves to: `-O3` and `-opt level=3,peephole` are the same thing, and one `-O` level can set five switches at once.
tags: [flags, tooling]
applies: [Wii/1.3]
demo:
reviewed: 2026-09-29
related: [5, 9, 16, 41]
---

# 8. Ask the compiler which optimizations are actually on

**Problem.** After a long sweep it is still unclear what a flag *set* resolves to: `-O3` and
`-opt level=3,peephole` are the same thing, and one `-O` level can set five switches at once.

**How it looks.** A debate about whether scheduling or the peephole pass is "on" for a unit, settled by
guessing what `-O4,p` expands to.

**Why it happens.** `-O<level>[,p|s]` is shorthand that sets several `-opt` sub-switches at once, later flags
replace earlier ones, and per-function pragmas can change the state mid-file (idea 16). The effective
configuration is not visible in the command line.

**How to work it.** `-opt display` prints the resolved option set (the compiler also accepts `-opt dump`). Run it
with the unit's full candidate flag list before declaring a flag set "the" answer; it turns a guess about macro
expansion into a fact. It is a process check with no codegen of its own, so it has no demo.

**When NOT to apply.** `display` shows the command-line state, not what a `#pragma` inside the file changed
(idea 16, idea 41: `#pragma optimization_level 1` does not turn the peephole off).

**Result.** `-opt display` is the check to run before recording a flag set in `configure.py`.

**Example**

```sh
mwcceppc.exe <candidate flags> -opt display -c src/<Unit>/<file>.c -o build/tmp/probe
```

Measured on Wii/1.3 (2026-09-29), `-O3 -opt display`:

```
	- global optimizer level 3
	- global optimize for speed
	- no extra global optimizations
Backend-specific optimizer options:
	- peephole optimizations on
	- no instruction scheduling
```

`-O3`, `-O3,p` and `-opt level=3,peephole` print identically (peephole **on**, scheduling off);
`-opt nopeephole,level=4` prints level 4 with peephole **off**. The `peephole optimizations off` line seen in
earlier notes therefore belongs to a candidate carrying `nopeephole`; a bare `-O3` has the peephole pass on.
