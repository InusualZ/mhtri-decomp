# `lib/localedit` - write the named local a regsim plan asks for

## Purpose

Turns the edit of a `regsim search` plan (or its unmet volatile target) into source text the lane would otherwise
write by hand, and refuses - printing the exact edit - whenever the expression cannot be identified unambiguously.

## Users

`tools/mwcc-debugger/regsim.py` (`search/run --apply-local`).

## Public API

* `parse_edit(edit strings) -> Edit(kind declare|fold, vreg, label, position, target)`.
* `walker(...) -> Change`: `declare` of a temp first seen as `addi T,L,K` where `L` is a pointer local/parameter whose
  class has a `/* +0xK */` field (read from the unit's include closure): the one braced loop
  `for (IV = 0; IV < N; IV++)` reading `&L->field[IV]` becomes `for (IV = 0, p = L->field; ...; p++, IV++)` with `p`
  at the sites; `p` is `T* p;` (`NewLocal`), named singular of the field (`parts_0x02` -> `part`). The declaration
  goes into the plan's order through `regsim.rewrite_runs(extra=)`.
* `hoist(...) -> Change`: a temp (not a named local) live across exactly one direct `bl` in ours and kept volatile by
  retail: `T v = callee(args);` on the line before the statement, `v` in its place. Requires the dump's `bl` count to
  equal the object's REL24 relocations of the function, the callee's source calls to equal its object calls, the
  statement to be plain (no `else`/`while`/`for`/`do`, no `&&`, `||`, `?:`, `,`, `++`, `--` before the call, first line
  after a `;`, `{` or `}`) and the callee's return type to be declared in the unit's includes. The call is matched to
  the n-th source occurrence by code order.
* `fold(...)`: always `Refused` (a local that interferes with nothing is already propagated; otherwise the reuse needs
  the temp's uses).
* `Refused(reason, manual)`.

## Invariants and rules

A `Change` is a list of exact `(old block, new block)` replacements; nothing is written here. The caller measures the
trial with `unittry.py` and keeps it only when it wins. Identification by position is a heuristic that the
measurement checks; ambiguity refuses.

## Test contract

Tier: fixture - `tools/tests/lib/test_regsimrun.py`: edit parsing, fold refusal, field offsets from the comments, types
of locals/parameters, the walker's rewrite and its refusals (no loop site, not an `addi`), the hoist's rewrite, the
`--local-name`, and the ambiguity, call-count and `&&` refusals.

## Known gaps

Only the two shapes above are written; a frontend `@` temp that is a global load, a fold, and a loop with a braceless
body are refused with the manual edit. The hoisted local is declared in place with its initialiser; its position in
the declaration order is not searched.
