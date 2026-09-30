---
id: 60
title: The declaration set is part of the codegen - fold a declaration only where one is deleted
status: works
problem: A unit sits at 100 %, you add one `#include` of a header that declares a callee the file already sees, and a function drops by a hair (`fn_800CC5B0` 96.79185 -> 96.78541) with a byte-identical `.text` length - not a size problem, not a flag problem.
tags: [source-shape, measurement]
applies: []
demo: 060-declaration-set-codegen.cpp
reviewed: 2026-09-29
related: [51, 64, 72]
---

# 60. The declaration set is part of the codegen - fold a declaration only where one is deleted

**Problem.** A unit sits at 100 %, you add one `#include` of a header that declares a callee the file already
sees through another header, and a function drops by a hair - `fn_800CC5B0` went 96.79185 -> 96.78541 with a
byte-identical `.text` length. It is not a size problem and not a flag problem, so it looks unreachable from the
source side.

**How it looks.** A change that is "obviously" codegen-neutral (an added include, a consolidated prototype, a
rename sweep) moves a per-function percentage in a unit you did not aim at, by hundredths of a point, with equal
sizes. Often the moved row differs only in an anonymous pool symbol's *name* or a register.

**Why it happens.** MWCC numbers its anonymous compiler-made symbols (the `@N` pool and jump-table names) and
orders its internal work in declaration order, so the *set* of declarations a translation unit sees is an input
to codegen even when no declaration is used differently. Measured 2026-09-29 with the demo: the pool entry for
`3.5f` is named `@7` when the TU has one prototype above the function and `@10` with three more unused prototypes
above it - each declaration advances the counter. That the counter can change a *score* (objdiff pairs an anonymous
symbol by name, so a shifted name reads as a relocation-name difference) matches the original observation but was
not re-measured in this review; that register colouring follows declaration order (the original explanation) is
likewise unconfirmed here - what would need measuring is an added unused prototype's effect on a function's
register allocation in a real unit, before and after.

**How to work it.** A declaration consolidation is safe when it is **net-zero**: delete N local declarations and
provide the same N through the owner's header, so the TU's declaration surface is unchanged. Never add an owner's
header "for tidiness" to a TU that already reaches the declaration another way. When a band's header is too heavy
to pull in at all - including `fn_8004CAD8.h` into 41 TUs failed with `(10505)`/`(10197)` on `fn_80052BC0`/
`fn_80050850`/`fn_800504D4` - that is a signal to settle the owners' signatures first, not to force the include.

**How to check.** Diff `report.json` over the **whole tree**, not the unit you touched: a fold moves a row in a TU
that has nothing to do with the symbol being renamed, and only a whole-tree diff shows it. One command:

```sh
ninja changes      # every unit whose score moved vs the baseline (after `ninja baseline`)
```

A **non-empty** line means a unit you did not touch moved - investigate it, never wave it through. A batch that
folded 114 declarations across 65 files moved **0 of 2,797 units** once it followed the net-zero rule (measured then).

**When NOT to apply.** Not a reason to avoid headers in general - only a reason to make include/declaration changes
net-zero in a unit that is at or near a match, and to measure them. It interacts with idea 51 (one owner per
declaration) - consolidating to the owner's header is right, but do it as a fold, not an addition - and with idea 64
(a declaration's *size* selects the addressing form, a stronger version of the same effect).

**Demonstration.** `060-declaration-set-codegen.cpp` (`ideas.py demo-check 60`): with three unused prototypes above
`scale_a`, its pool entry is `@10`; the demo asserts `@10` present and `@7` absent (deleting the three prototypes
and re-running `--dump` gives `@7`). It demonstrates that declarations are counted into compiler-generated names -
not, by itself, a score change.
