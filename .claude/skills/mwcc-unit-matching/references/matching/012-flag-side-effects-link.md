---
id: 12
title: Check that the flag's side effects still link
status: works
problem: Some flags change the *relocations*, not just the instructions: a save idiom that calls runtime helpers, or a table base that changes how a symbol is addressed. Matching the code shape while referencing a symbol the original build never had is a false positive.
tags: [flags, relocations]
applies: []
demo:
---

# 12. Check that the flag's side effects still link

**Problem.** Some flags change the *relocations*, not just the instructions: a save idiom that calls
runtime helpers, or a table base that changes how a symbol is addressed. Matching the code shape while
referencing a symbol the original build never had is a false positive.

**Why try it.** Flags that change relocations have a second acceptance criterion - the referenced symbol
must exist in the target's symbol table - and it is a one-line check.

**Result.** Confirm the symbol exists (`symbols.txt`) or that the relocation shape matches (each table gets
its own `@ha`/`@l` pair). This is also the check that tells you a "flag win" is real rather than cosmetic.

**Example**

```sh
grep -n "_savegpr_14" config/RMHE08/symbols.txt
# 38292:_savegpr_14 = .text:0x80456DD4; // type:label scope:global
```
