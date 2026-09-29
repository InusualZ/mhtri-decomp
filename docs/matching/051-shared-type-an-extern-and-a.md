---
id: 51
title: A shared type, an extern and a mangled name each have one owner
status: works
problem: Three defects are the same mistake in three shapes: an identifier that belongs to someone else is spelled out locally instead of reached through its owner. A type two units share is **copied** into each unit's source (20 names, 68 extra definitions under `src/`); a function or variable another unit **defines** is `extern`-declared in the consumer's file "to save an include" (126 declarations into 27 owner units); and a compiler **mangling** - `get_now_areano__Fv`, `move__6MHcharFUs`, `Panic__Q24nw4r2dbFPCciPCce` - is written as the callable identifier. The last is 50's bug waiting to happen: a mangling is a *compiler* spelling, so a C++ front-end handed `Panic__Q24nw4r2dbFPCciPCce` as a name mangles it a second time (`...__FPCciPCce`) and the link cannot resolve it, invisibly while the unit is `NonMatching`.
tags: [symbols, process]
applies: []
demo:
---

# 51. A shared type, an extern and a mangled name each have one owner

**Problem.** Three defects are the same mistake in three shapes: an identifier that belongs to someone else
is spelled out locally instead of reached through its owner. A type two units share is **copied** into each
unit's source (20 names, 68 extra definitions under `src/`); a function or variable another unit **defines**
is `extern`-declared in the consumer's file "to save an include" (126 declarations into 27 owner units); and a
compiler **mangling** - `get_now_areano__Fv`, `move__6MHcharFUs`, `Panic__Q24nw4r2dbFPCciPCce` - is written as
the callable identifier. The last is 50's bug waiting to happen: a mangling is a *compiler* spelling, so a C++
front-end handed `Panic__Q24nw4r2dbFPCciPCce` as a name mangles it a second time (`...__FPCciPCce`) and the
link cannot resolve it, invisibly while the unit is `NonMatching`.

**Why try it.** Each defect is decidable from data we already have, so none of them needs a reviewer's
memory. A `struct`/`class`/`union` definition is textual, so a duplicate is one cross-file scan. A mangling
has a signature - an argument list (`__F`) or a qualifier (`__Q`), or the class-member form `__<len>ClassF` -
and the map's own `fn_XXXXXXXX` placeholder has no `__` at all, so it never matches. And ownership is
derivable: `config/RMHE08/symbols.txt` gives a symbol's section and address, `config/RMHE08/splits.txt` gives
each registered unit's ranges, so the unit that owns a symbol is a lookup. `tools/units/stylelint.py` does all
three; `--diff <base>` refuses only *new* violations, so the backlog can burn down unit by unit and a batch
that touches a unit with 300 old findings is still allowed.

**Result.** Rules 1, 2 and 9 are checked (they were prose in 6.5). Measured on the tree: rule 1 **68** extra
definitions of **20** names; rule 9 **598** sites (**446** calls, **152** declarations) over **83** mangled
names - and it does **not** fire on an `fn_XXXXXXXX` stem, on `obj->method()`, or on `ns::func()`; rule 2
**126** declarations into 27 other registered units, plus **209** unsplit declarations the address band places
in six modules (`enemy` 125, `g3d` 32, `sound` 31, `Pl` 13, `ef` 7, `Runtime.PPCEABI.H` 1). Rule 2's gap is
named, not guessed: the registered bands interleave across modules (a `sound` unit sits inside the `ef` band),
so an unsplit address whose bracketing units disagree has no sound header to move to and is counted rather
than flagged - 1 324 such sites and 20 names absent from the map are reported as gaps. `--diff HEAD` is clean
on the current tree, so the gate is live without blocking work that touches a unit with a backlog.

**Example.** The fix is the same refactor in each case:

```cpp
// rule 1: one definition, in the owner's header, included where needed
// include/ef/effect.h
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

**Refined (2026-09-28) - rule 2 is not `extern`-only, so the numbers above read the rule too narrowly.**
`stylelint.py`'s scan was extended: rule 2 now judges every **declaration** - the `extern` keyword *or* a
plain function prototype (`void foo(void);`) - and it judges it in **two file classes**, a `src/` file and an
ordinary `include/<module>/*.h` header. `_owns` was extended to accept an owner's own public header
(`include/<module>/<stem>.h`) first, without which every owner's header would have reported itself; and under
`include/unsplit/*.h` the reading **inverts** - a declaration there of a symbol a registered unit owns is the
finding, because the band is a fallback, not the owner. Measured whole-tree on this branch: **7 482** rule-2
findings over **5 949** distinct symbols, **4 287** of them unsplit-address sites across 7 band names (`ef`
1 658, `<band unresolved>` 2 550, `Network` 59, `OS` 11, `Runtime.PPCEABI.H` 6, `menu` 2, `enemy` 1), plus
**647** names the map does not contain - those stay **gaps**, because the map cannot judge them. That is ~60x
the `extern`-only count above, and it is the same semantics the `extern` shape already had: the plain
prototype is simply how the foreign declarations were actually written (e.g. `include/Network/fn_8041A87C.h`'s
`u16 DWCi_htons(u16 port);`, owned by `src/DWCi/fn_805113B0.c`). The breadth is deliberate; what is no longer
true is the earlier implication that a `src/`-file `extern` is the only shape rule 2 sees.

**Refined (2026-09-28) - one owner does not always mean one DECLARATION, and a rename's sweep has a third
place.** Two additions from the DWCi band (`.pi/notes/dwci-band-9050.md`, `.pi/notes/loop-dwci-phase2.md`):

* **A shared data symbol whose consumers relocate it differently cannot have one declaration - the sizedness
  IS a codegen input.** `natNegMessageMagic` (`.sdata` 0x80794380, the six signature bytes `FD FC 1E 66 6A
  B2`) is referenced from two objects and they disagree on the *form*: the negotiator's relocates it `SDA21`,
  while `Network/fn_8041A87C.cpp`'s relocates it `ADDR16_HA`/`ADDR16_LO`. A declared size selects the SDA
  form and an unknown size the absolute one (row 64), so **one declaration cannot serve both objects**. Each
  band header therefore carries the spelling *its* object needs - sized in `include/DWCi/DWCi_NatNeg.h`,
  unsized in `include/unsplit/Network.h` (which also carries a class, so a plain `.c` cannot include it) - and
  **each calls out why the other spelling is wrong for it**. Rule 1 still wants one *definition* where there
  is one; this is the declaration half, where "one owner" and "one spelling" are not the same statement
  (row 60 is the general form: the declaration set is part of the codegen).
* **A rename's sweep has a third place.** A rename is "map + every referrer", and `symedit.py refs` roots at
  `src/` + `include/`. The DWCi band's renames left **44 stale names in `tools/units/attribution-queue.json`**
  plus one stale example in `tools/splits/tudiscover.py:666` (measured: `git grep -w fn_805087A0` -> the
  queue row only, 0 hits in `src/`/`include/`, and `symedit.py find` confirms the map rows are gone). The
  queue is a **regenerable cache** whose own fingerprints already prove it stale on three of four inputs, so
  a hand sweep would fix the strings and leave the fingerprints lying about freshness - the honest repair is
  to **record the scope where the next rename reads it** (`.claude/skills/symbol-map-editing/SKILL.md` rule 2
  now states the real roots and names the queue; its old "source, docs, tools" claim was false). Worth
  stating too: **`rename` is not one of `backlog.py`'s kinds**, so a rename filed in an outbox is picked up
  by nothing. Rule 2 judges each declaration by the **ownership of the address** it resolves to
  (`Ownership.resolve`: name -> `symbols.txt` address -> the `splits.txt` range covering it), so a half-done
  rename leaves both spellings visible to `--diff`, and finishing a rename the base already made moves the
  finding rather than adding one.