---
id: 60
title: The declaration set is part of the codegen - fold a declaration only where one is deleted
status: works
problem: A unit sits at 100 %, you add one `#include` of a header that declares a callee the file already sees through another header, and a function drops by a hair - `fn_800CC5B0` went 96.79185 -> 96.78541 with a byte-identical `.text` length. It is not a size problem and not a flag problem, so it looks unreachable from the source side.
tags: [flags]
applies: []
demo:
---

# 60. The declaration set is part of the codegen - fold a declaration only where one is deleted

**Problem.** A unit sits at 100 %, you add one `#include` of a header that declares a callee the file already
sees through another header, and a function drops by a hair - `fn_800CC5B0` went 96.79185 -> 96.78541 with a
byte-identical `.text` length. It is not a size problem and not a flag problem, so it looks unreachable from
the source side.

**Why it happens.** MWCC numbers the anonymous constant pool and colours the webs in declaration order, so the
*set* of declarations a translation unit sees is an input to codegen even when no declaration is used
differently. Re-declaring what the file already has shifts that ordering.

**How to work it.** A declaration consolidation is safe when it is **net-zero**: delete N local declarations
and provide the same N through the owner's header, so the TU's declaration surface is unchanged. Never add an
owner's header "for tidiness" to a TU that already reaches the declaration another way. When a band's header is
too heavy to pull in at all - including `fn_8004CAD8.h` into 41 TUs failed with `(10505)`/`(10197)` on
`fn_80052BC0`/`fn_80050850`/`fn_800504D4` - that is a signal to settle the owners' signatures first, not to
force the include.

**How to check.** Diff `report.json` over the **whole tree**, not the unit you touched: a fold moves a row in a
TU that has nothing to do with the symbol being renamed, and only a whole-tree diff shows it. It is one
command a lane can run verbatim:

```sh
ninja changes      # every unit whose score moved vs the baseline
```

A **non-empty** line means a unit you did not touch moved - investigate it, never wave it through. A batch that
folded 114 declarations across 65 files moved **0 of 2,797 units** once it followed the net-zero rule.
