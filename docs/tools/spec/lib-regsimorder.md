# `lib/regsimorder` - Spell a regsim plan's declaration order into a function's source, or refuse with the reason

## Purpose

Turns the declaration order of a `regsim` plan (`Plan.layout.decl`, first declared = highest vreg) into the new text of the
function's source: a reorder inside each scope, a hoist of block-scope locals to function scope, or that hoist with the
`for` header variables. Every spelling keeps the program's meaning; a move that would not is `Refused` with the reason.

## Users

`tools/units/applyorder.py` (`mt.py regsim run ... --apply-order`), `tools/mwcc-debugger/regsim.py` (`search --apply-order`).

## Public API

* `spellings(decl, mapping, text, function, edits=()) -> ([Spelling], [(strategy, Refused)])`: every distinct way to write
  the plan; a vreg of the plan with no declared local (the new local an edit asks for) and the plan's own `edits` are left
  out and the summary says so. `mapping` is `{vreg: regsim.SourceLocal}`.
* `scoped(...)`: each scope's declaration lines are slots, each slot takes the declaration the plan puts there. The lines
  swap when no initialiser changes its place relative to another one, nothing an initialiser uses is declared later, no
  statement lies between the slots of an initialised scope and no moved local is used before its new line. Otherwise the
  plain declarations go to the scope's first slot in the plan's order and each initialiser stays at its own line as an
  assignment (refused for a `const` or brace initialiser and for a class-typed local with an initialiser). A class-typed local
  does not move across a statement. `Nothing` (the plan is the source's order) and `Few` (no scope lists two locals) are
  `Refused` subclasses the caller reads as SAME / FEWFS.
* `hoisted(..., with_for=False)`: `localedit.hoist_order` (idea 324); with `with_for`, each `for (T v = e;` header whose `v`
  is unique in the function becomes `for (v = e;` with `T v;` declared at the top (a `const`, a multi-name header, a name
  declared twice are refused).
* `split_declarators(text, function, mapping, vregs)`: a line `T a, *b;` that declares a moved local is written one name
  per line (`T a;` / `T *b;`) first; `remap` finds every local again by (name, n-th of that name).
* The text-level reading of a declaration line is `regsim.declarators` / `regsim.source_locals` (a line with several names
  gives one `SourceLocal` each, `shared` counting them, `kind == "for"` for a header variable).

## Invariants and rules

* Pure text in, text out: no file is read or written, nothing is measured; the caller measures.
* A spelling differs from the source (a plan that changes nothing is `Nothing`/dropped), and only declarations and the
  assignments an initialiser becomes are touched.

## Lib dependencies

regsim (locals, declarators), localedit (`hoist_order`), cscan.

## Test contract

Tier: fixture - `tools/tests/units/test_applyorder.py` (every spelling and refusal above, on small functions).

## Known gaps

Types are read from the declaration text only: a typedef of a class type spelled with scalar words is judged scalar; a
brace-less `for` has its variable scoped to the next block (`regsim.source_locals`).
