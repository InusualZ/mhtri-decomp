---
id: 48
title: A C++ unit's unmangled map name is not a reason for `extern "C"`
status: works
problem: The unit is C++ (a `__FILE__` names a `.cpp`, or a callee is mangled) but the map spells its symbols unmangled (`fn_800CCCF8`), so a C++ definition emits a mangled name, objdiff pairs nothing and the unit reads 0 % with byte-perfect code.
tags: [symbols]
applies: []
demo: 048-unmangled-name-not-extern-c.cpp
reviewed: 2026-09-29
related: [31, 42, 49, 50, 51]
---

# 48. A C++ unit's unmangled map name is not a reason for `extern "C"`

**Problem.** The unit is C++ - a `__FILE__` string names a `.cpp`, or a callee is mangled - but the symbol map
spells its symbols unmangled (`fn_800CCCF8`), because dtk could not demangle them. objdiff pairs by symbol name,
so a C++ definition emits a *mangled* name, pairs nothing, and the unit reports 0 % with byte-perfect code. The
tempting fix is `extern "C"`, which is the wrong front-end for the unit (idea 42's other side) and quietly makes
the source a lie about the language it was written in. It is also unavailable where it is needed most: a **member
function** cannot be `extern "C"` at all.

**How it looks.** Functions score 0 % (idea 31) although `objdump -d` of ours and the target shows identical
instructions; the object's symbol table has `fn_800CCCF8__FP9ResHandle` where the map (and the target object) says
`fn_800CCCF8`.

**Why it happens.** The symbol map is a **build input**, not a description of the original's symbol table - and
the original object's name *is* the mangled one. So the map is not evidence against the C++ reading; it is
under-specified. (A *mangling* is the compiler's spelling of a name plus its argument types: `Us` = `u16`,
`FP4_PLW` = "function taking a pointer to `_PLW`", `Q3...` = qualified by classes/namespaces.)

**How to work it.** Rename the map symbol to the mangled spelling the source emits. A rename is always two edits
in one change (map + source; `tools/symbols/symedit.py rename`, see the `symbol-map-editing` skill). Derive the
spelling with `tools/units/mangle.py`: it compiles a probe (the declaration with a body appended, plus
`#include "types.h"`) using a **real unit's command line**, so the compiler version and `-lang=c++` are the ones the
project uses, and prints the object's defined symbol names. Only the front-end affects the mangling, so the
`--unit` borrowed for flags is not a claim about the unit being renamed.

**Result.** Validated by exact reproduction of a name the map already had. Re-run 2026-09-29:

```sh
python tools/units/mangle.py 'struct _PLW; void Pl_Skill_ck(_PLW* self, u16 x)' --unit Pl/pl_act.cpp
# Pl_Skill_ck__FP4_PLWUs       <- the spelling in symbols.txt

python tools/units/mangle.py 'struct _PLW; void Pl_Skill_ck(_PLW* self, u16 x)' --old <the fn_XXXXXXXX name being renamed>
# prints the name and the exact `symedit.py rename` command for the map + source edit

python tools/units/mangle.py 'struct M { void f(int); }; void M::f(int)'
# f__1MFi        <- a member function: the case extern "C" cannot express
```

If a derived name does not match what the map has, that is information about the **signature** (the map's `Us`
says `u16`, not `u8`): read it as evidence and re-measure with `recompile.py`, because a rename that makes objdiff
pair nothing is a regression, not progress.

**When NOT to apply.** A `fn_XXXXXXXX` stem is a placeholder, so this rename is right for it. A map name that is
*already* a mangling is the real name: write the real declaration instead (idea 50), do not rename. For a unit
that is genuinely C (a `.c` file, `__FILE__` names a `.c`) there is nothing to rename - the symbols are unmangled.
`extern "C"` remains right for a symbol whose real name you cannot express (idea 42 is that case); the two ideas
are the two halves of one decision - see 50 for the summary.

**Example.**

```cpp
void Pl_Skill_ck(u16 x) { sink(x); }            // emits Pl_Skill_ck__FUs
extern "C" void Pl_Skill_ck_c(u16 x) { sink(x); } // emits Pl_Skill_ck_c, no mangled twin
struct Box { int v; void f(int a); };
void Box::f(int a) { v = a; }                    // emits f__3BoxFi - no extern "C" spelling exists
```

**Demonstration.** `048-unmangled-name-not-extern-c.cpp` (`ideas.py demo-check 48`): a free function is emitted as
`Pl_Skill_ck__FUs`, the `extern "C"` twin under its bare name with no mangled companion, and the member function
under the class-qualified `f__3BoxFi`. (The checker's `absent` also matches the mangled-prefix form, so the demo
asserts the absence of the mangled twin, not of the bare name.)
