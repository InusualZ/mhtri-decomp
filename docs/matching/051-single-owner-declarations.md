---
id: 51
title: A shared type, an extern and a mangled name each have one owner
status: works
problem: An identifier that belongs to someone else is spelled out locally instead of reached through its owner - a shared type copied into each unit, a foreign function/variable re-declared "to save an include", or a mangling written as the callable name.
tags: [symbols, process]
applies: []
demo:
reviewed: 2026-09-29
related: [50, 56, 60, 64, 70]
---

# 51. A shared type, an extern and a mangled name each have one owner

**Problem.** Three defects are the same mistake in three shapes: an identifier that belongs to someone else is
spelled out locally instead of reached through its owner. A type two units share is **copied** into each unit's
source; a function or variable another unit **defines** is `extern`-declared (or plain-prototyped) in the
consumer's file "to save an include"; and a compiler **mangling** - `get_now_areano__Fv`, `move__6MHcharFUs`,
`Panic__Q24nw4r2dbFPCciPCce` - is written as the callable identifier. The last is idea 50's bug waiting to happen:
a mangling is a *compiler* spelling, so a C++ front-end handed `Panic__Q24nw4r2dbFPCciPCce` as a name mangles it a
second time (`...__FPCciPCce`) and the link cannot resolve it, invisibly while the unit is `NonMatching`.

**How it looks.** A struct defined in three `.cpp` files that drift apart; a `void foo(void);` in a consumer for a
function some other unit defines; a call to `move__6MHcharFUs(obj, 0)` instead of `obj->move(0)`. None of these
shows in a score: they are hygiene defects that turn into a wrong layout, an `illegal overloading` error, a wrong
`__F` suffix or an undefined symbol later.

**Why it happens / why it is decidable.** Each defect is decidable from data we already have, so none needs a
reviewer's memory. A `struct`/`class`/`union` definition is textual, so a duplicate is one cross-file scan. A
mangling has a signature - an argument list (`__F`) or a qualifier (`__Q`), or the class-member form
`__<len>ClassF` - and the map's own `fn_XXXXXXXX` placeholder has no `__` at all, so it never matches. And
ownership is derivable: `config/RMHE08/symbols.txt` gives a symbol's section and address, `config/RMHE08/splits.txt`
gives each registered unit's ranges, so the unit that owns a symbol is a lookup.

**How to work it.** `tools/units/stylelint.py` does all three (rules 1, 2 and 9 of `docs/plan.md` section 6.5);
`--diff <base>` refuses only *new* violations (`--ref <branch>` judges a branch without checking it out, and
`--list-added` names each added finding), so the backlog burns down unit by unit and a batch that touches a unit
with 300 old findings is still allowed. The fix is the same refactor in each case:

```cpp
// rule 1: one definition, in the owner's header, included where needed
// src/ef/effect.h
struct Effect { /* size: 0x10 */ /* +0x00 */ u32 flags; };
// src/ef/eft004.cpp
#include "ef/effect.h"

// rule 2: the declaration lives with the unit that defines the symbol; the consumer includes it
// src/ef/eft002.cpp
#include "ef/fn_800FD520.h"

// rule 9: call the owner, never the mangling
obj->move(0);                        // not move__6MHcharFUs(obj, 0)
nw4r::db::Panic(file, line, fmt);    // not Panic__Q24nw4r2dbFPCciPCce(file, line, fmt)
```

A rename has a **third place** to sweep beyond the map and the source: `symedit.py refs` roots at `src/`
only; `docs/` and `tools/` are searched only when named in `--roots`. Regenerable caches (the retired
`tools/units/attribution-queue.json` was one; `docs/tools/retired.md`) keep their own name strings (keyed by `symbols_sha1`), so the map plus
`--roots` is the authority, not the rows in the cache (the symbol-map-editing skill, rule 2, states this).

**Result (measured when written).** Rules 1, 2 and 9 were made checks (they were prose in 6.5). At that time: rule
1 **68** extra definitions of **20** names; rule 9 **598** sites (446 calls, 152 declarations) over **83** mangled
names - not firing on an `fn_XXXXXXXX` stem, on `obj->method()`, or on `ns::func()`; rule 2 **126** declarations
into 27 other registered units plus **209** unsplit declarations the address band places in six modules. Rule 2's
gap is named, not guessed: the registered bands interleave across modules, so an unsplit address whose bracketing
units disagree has no sound header to move to and is counted (1,324 sites, 20 names absent from the map) rather
than flagged.

**When NOT to apply / limits.** *One owner does not always mean one DECLARATION* (see the DWCi finding below):
when two consumers relocate the same data symbol differently, the sizedness of the declaration is a codegen input
(idea 64) and each object needs its own spelling; idea 60 is the general form (the declaration set is codegen).
Rule 2's ownership test resolves the address the name maps to, so a half-done rename leaves both spellings
visible to `--diff`.

**Evidence.**

*Rule 2 widened (2026-09-28).* The scan was extended so rule 2 judges every **declaration** - the `extern`
keyword *or* a plain prototype (`void foo(void);`) - in **two file classes**, a `src/` file and an ordinary
`src/<module>/*.h` header. `_owns` accepts an owner's own public header (`src/<module>/<stem>.h`) first,
without which every owner's header would report itself; and under `src/unsplit/*.h` the reading **inverts** -
a declaration there of a symbol a registered unit owns is the finding, because the band is a fallback, not the
owner. Measured whole-tree on that branch: **7,482** rule-2 findings over **5,949** distinct symbols (4,287
unsplit-address sites across 7 band names, plus 647 names the map does not contain, which stay gaps). That is
~60x the `extern`-only count above and the same semantics: the plain prototype is how the foreign declarations
were actually written (e.g. `src/Network/fn_8041A87C.h`'s `u16 DWCi_htons(u16 port);`, owned by
`src/DWCi/fn_805113B0.c`).

*Shared data with two relocation forms (2026-09-28, DWCi band; local notes `.pi/notes/dwci-band-9050.md`,
`.pi/notes/loop-dwci-phase2.md`).* `natNegMessageMagic` (`.sdata` 0x80794380, six signature bytes) is referenced
from two objects that disagree on the *form*: the negotiator's relocates it `SDA21`, `Network/fn_8041A87C.cpp`'s
relocates it `ADDR16_HA`/`ADDR16_LO`. A declared size selects the SDA form and an unknown size the absolute one
(idea 64), so one header declaration cannot serve both. First written as one spelling per band header; as of
2026-09-29 the tree carries it once: `src/DWCi/DWCi_NatNeg.h` declares the **unsized** form
(`extern const u8 natNegMessageMagic[];`, the consumer's `lis`/`addi`), `src/unsplit/Network.h` reaches it by
including that header, and the owner's own source re-declares it **sized** before its first use for its ten SDA21
sites (the first use of a symbol fixes its addressing for the whole TU, idea 12). Rule 1 still wants one *definition* where there is
one; this is the declaration half.

*A rename's third place (same session).* The DWCi band's renames left 44 stale names in
`tools/units/attribution-queue.json` (retired 2026-10-03) plus one stale example in `tools/splits/tudiscover.py`; re-checked
2026-09-29, `git grep fn_805087A0 -- tools` finds nothing (the cache has been regenerated). `rename` is not one of
`backlog.py`'s kinds, so a rename filed in an outbox is picked up by nothing.
