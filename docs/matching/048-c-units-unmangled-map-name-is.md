---
id: 48
title: A C++ unit's unmangled map name is not a reason for `extern "C"`
status: works
problem: The unit is C++ - a `__FILE__` string names a `.cpp`, or a callee is mangled - but the symbol map spells its symbols unmangled (`fn_800CCCF8`), because dtk could not demangle them. objdiff pairs by symbol name, so a C++ definition emits a *mangled* name, pairs nothing, and the unit reports 0 % with byte-perfect code. The tempting fix is `extern "C"`, which is the wrong front-end for the unit (row 42's other side) and quietly makes the source a lie about the language it was written in. It is also unavailable where it is needed most: a **member function** cannot be `extern "C"` at all.
tags: [symbols, measurement]
applies: []
demo:
---

# 48. A C++ unit's unmangled map name is not a reason for `extern "C"`

**Problem.** The unit is C++ - a `__FILE__` string names a `.cpp`, or a callee is mangled - but the symbol
map spells its symbols unmangled (`fn_800CCCF8`), because dtk could not demangle them. objdiff pairs by
symbol name, so a C++ definition emits a *mangled* name, pairs nothing, and the unit reports 0 % with
byte-perfect code. The tempting fix is `extern "C"`, which is the wrong front-end for the unit (row 42's
other side) and quietly makes the source a lie about the language it was written in. It is also unavailable
where it is needed most: a **member function** cannot be `extern "C"` at all.

**Why try it.** The symbol map is a **build input**, not a description of the original's symbol table - and
the original object's name *is* the mangled one. So the map is not evidence against the C++ reading; it is
simply under-specified, and the correct edit is to **rename the map symbol to the mangled spelling the
source emits**. A rename is always two edits in one change (map + source), which is the ordinary
`symbol-editing` path - nothing about the language changes.

**Result.** `tools/units/mangle.py` derives that spelling: it compiles a probe (the declaration, with a body
appended, plus `#include "types.h"`) using a **real unit's command line**, so the compiler version and
`-lang=c++` are the ones the project actually uses, and prints the object's defined symbol names. Only the
front-end affects the mangling, so the `--unit` borrowed for flags is not a claim about the unit being
renamed. Validated by exact reproduction of a name the map already had.

**Example.**

```sh
python tools/units/mangle.py 'struct _PLW; void Pl_Skill_ck(_PLW* self, u16 x)' --unit Pl/pl_act.cpp
# Pl_Skill_ck__FP4_PLWUs          <- byte-identical to symbols.txt's existing name

python tools/units/mangle.py 'struct _PLW; void Pl_Skill_ck(_PLW* self, u16 x)' --old fn_80270F50
# prints the name and the exact `symedit.py rename` command for the map + source edit
```

A member function works the same way and is the case `extern "C"` cannot express:

```sh
python tools/units/mangle.py 'struct M { void f(int); }; void M::f(int)'   # -> f__Q23MangleProbe... style
```

If a derived name does not match what the map has, that is information about the **signature** (the map's
`Us` says `u16`, not `u8`) - read it as evidence and re-measure with `recompile.py`, because a rename that
makes objdiff pair nothing is a regression, not progress.
