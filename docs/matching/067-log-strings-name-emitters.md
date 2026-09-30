---
id: 67
title: A band's `.data` LOG STRINGS name their own emitters
status: works
problem: A band arrives as a hundred `fn_XXXXXXXX` and the naming evidence is the runtime dump (playbook 25), a `__FILE__` string (54) or the neighbours' scheme - all of which can be absent at registration. The band's own `.data` log strings were being read past, even though they are loaded by the code under reconstruction.
tags: [symbols, data]
applies: []
demo:
---

# 67. A band's `.data` LOG STRINGS name their own emitters

**Problem.** A band arrives as a hundred `fn_XXXXXXXX` and the naming evidence is the runtime dump
(playbook 25), a `__FILE__` string (54) or the neighbours' scheme - all of which can be absent at
registration. The band's own `.data` log strings were being read past, even though they are loaded by the
code under reconstruction.

**Why try it.** A log string is materialised (`lis`/`addi` of its address, or `@sda21`) by the function that
logs it, and each string is loaded by **exactly one** function in the range - so the emitter's name is
*evidenced* by the string's own text, not guessed. It is a naming-evidence class for rule 7, and it needs no
runtime dump: `tools/units/callers.py <address>` answers "who loads this label" from the whole-DOL
address-keyed index in one query.

**Result.** Five strings named five functions of `Network/fn_803D3CE8`:
`NetworkSessionStable::downPerformance` (0x805FA550/0x805FA590), `::upPerformance` (0x805FA5D4/0x805FA610),
`NetworkSessionStable::move` (0x805FA64C), `NetworkSessionManager::move` (0x805FA788, the old `slot_18`) and
`NetworkSessionManagerPat::final` (0x805FAB08). The band was renamed off those names and 22 bodies written
against real signatures.

**Example.** The query is the whole search:

```sh
python tools/units/callers.py 0x805FAB08     # -> the one function that loads "NetworkSessionManagerPat::final"
```
