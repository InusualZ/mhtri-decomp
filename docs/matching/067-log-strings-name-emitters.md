---
id: 67
title: A band's `.data` LOG STRINGS name their own emitters
status: works
problem: A band of `fn_XXXXXXXX` functions has no names from the runtime dump or a `__FILE__` string, yet its own `.data` log strings - each loaded by exactly one function - were being read past as noise.
tags: [symbols, data]
applies: []
demo:
reviewed: 2026-09-29
related: [25, 54, 76]
---

# 67. A band's `.data` LOG STRINGS name their own emitters

**Problem.** A band arrives as a hundred `fn_XXXXXXXX` and the naming evidence is the runtime dump (idea 25),
a `__FILE__` string (idea 54) or the neighbours' scheme - all of which can be absent at registration. The
band's own `.data` log strings were being read past, even though they are loaded by the code under
reconstruction.

**How it looks.** A `.data` string like `"NetworkSessionManagerPat::final"` sits in the band's data range and
nothing names the function that prints it.

**Why it happens.** A log string is materialised (`lis`/`addi` of its address, or `@sda21`) by the function
that logs it, and each string is loaded by **exactly one** function in the range - so the emitter's name is
*evidenced* by the string's own text, not guessed. It needs no runtime dump.

**How to work it.** Query who loads the label (address-keyed, because the asm dump is stale), and name that
function after the string:

```sh
python tools/units/callers.py 0x805FAB08     # -> the one function that loads "NetworkSessionManagerPat::final"
```

(`callers.py --help` for `--data`, `--each`, `--json`.) Scheme the name after the text: `Class::method` in the
string becomes the class and method of the function, and the neighbours follow.

**When NOT to apply.** A shared string (loaded by several functions) names none of them; a format string
(`"%s failed"`) names nothing but its topic; and a string reused by a *helper* the callers inline names the
helper. Mark guesses in the unit header.

**Result.** Measured at the time (2026-09): five strings named five functions of `Network/fn_803D3CE8`:
`NetworkSessionStable::downPerformance` (0x805FA550/0x805FA590), `::upPerformance` (0x805FA5D4/0x805FA610),
`NetworkSessionStable::move` (0x805FA64C), `NetworkSessionManager::move` (0x805FA788, the old `slot_18`) and
`NetworkSessionManagerPat::final` (0x805FAB08). The band was renamed off those names and 22 bodies written
against real signatures.

**Not a codegen idea, so no demo.** It is a naming-evidence procedure (rule 7 evidence class); the check is
the `callers.py` query itself.
