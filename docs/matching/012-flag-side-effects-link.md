---
id: 12
title: Check that the flag's side effects still link
status: works
problem: Some flags change the *relocations*, not just the instructions: a save idiom that calls runtime helpers, or a table base that changes how a symbol is addressed. Matching the code shape while referencing a symbol the original build never had is a false positive.
tags: [flags, relocations]
applies: [Wii/1.3]
demo: 012-flag-side-effects-link.cpp
reviewed: 2026-09-29
related: [3, 23, 43]
---

# 12. Check that the flag's side effects still link

**Problem.** Some flags change the *relocations*, not just the instructions: a save idiom that calls runtime
helpers, or a table base that changes how a symbol is addressed. Matching the code shape while referencing a
symbol the original build never had is a false positive.

**How it looks.** The instructions match after a flag change, but the object now has a relocation naming a
helper (`_savegpr_14`) or a different symbol/addend than the target's, and the link either fails
(`undefined: '_savegpr_14'`) or silently binds to the wrong thing.

**Why it happens.** A relocation is the compiler's note "the linker must fill in this address"; a flag that
switches the save idiom from inline `stmw` to `bl _savegpr_N` adds a reference to a runtime symbol, and a flag
that changes table pooling changes which symbol+offset each `@ha`/`@l` pair (the high and low 16-bit halves of an
absolute address) names. Flags that change relocations have a second acceptance criterion.

**How to work it.** Confirm the referenced symbol exists in the target's symbol table
(`grep -n "^_savegpr_14 " config/RMHE08/symbols.txt`) or that the relocation shape matches (each table gets its
own `@ha`/`@l` pair). Compare the relocations of the two objects, not only the bytes: `tools/objdiff/relocdiff.py`
does this per unit. This is also the check that tells you a "flag win" is real rather than cosmetic.

**When NOT to apply.** A relocation *name* difference is sometimes only the split object's synthesized name
against the compiler's local `@NNN` (idea 59); check whether the two resolve to the same address before calling
it a mismatch.

**Result.** A flag is accepted only when its side effects also exist in the target.

**Example**

```sh
grep -n "^_savegpr_14 " config/RMHE08/symbols.txt
# <line>:_savegpr_14 = .text:0x80456DD4; // type:label scope:global   (line number varies)
```

**Demonstration.** `012-flag-side-effects-link.cpp` shows the relocations `-use_lmw_stmw off` adds: an
18-live-value function references `_savegpr_14` and `_restgpr_14` (the same source with `-use_lmw_stmw on` has no
such relocation and saves with `stmw`; see idea 3).
