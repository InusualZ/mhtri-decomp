# `lib/localedit` - write the named local a regsim plan asks for, and hoist its block-scope locals

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
* `declare(...) -> Change`: the entry `--apply-local` calls for a `declare` edit; it picks by what defines the temp
  (`g.defs`): `addi` the walker, `li` `constant_local`, `mr T,r3` `call_result_local`, `add` `sum_local`; any other
  definition (`lwz`, ...) is `Refused` naming the four it writes.
* `constant_local(...)`: `li T,K` where T is not written again (a counter or loop offset - `addi T,T,0x60` - is refused):
  `s32 zero;` in the plan's place (`zero`/`one`, else `const_K`), `zero = K;` on the line before the first
  `<lvalue> = K;` statement and `zero` as the value of every such statement of that block. Needs the first use to
  start a statement (after a `;`, `{` or `}` line, not an `else`/`while`/`for`/`do` header), every other use inside the
  first one's block, and no `case`/`default`/`goto` between them.
* `call_result_local(...)`: `mr T,r3` after a direct `bl` (r3 not rewritten between): the call is the n-th source call
  of its callee (the dump's `bl` count must equal the object's REL24 count, as for `hoist`); a local of the callee's
  return type declared in the plan's place, `v = callee(args);` on the line before the statement and `v` in the call's
  place (`hoist_call(..., assign_only=True)`, the declaration half of `hoist`).
* `sum_local(...)`: `add T,A,B` with `A`, `B` named integer locals or parameters (a pointer operand is refused: the sum
  is then address arithmetic): `s32`/`u32 sum;` in the plan's place, `sum = A + B;` before the first statement that
  writes `A + B` or `B + A` as a whole operand (not under `*`, `/`, `%`, unary `-`/`~`/`!`, a cast or a member access)
  and `sum` at every such place of the block; refused when `A` or `B` is written between the first and the last use.
  Both this and `constant_local` refuse a first use that is a function-scope declaration's initialiser (those lines are
  the declaration run the plan reorders, a second edit of them would not apply); so does `call_result_local` for a
  call that initialises a declaration.
* `call_symbols(obj, symbol)` leaves out the prologue/epilogue helpers (`_savegpr_N`, `_restgpr_N`, `_savefpr_N`,
  `_restfpr_N`): they are relocated `bl`s of the object that no source call explains (the dump's `bl` count then agreed
  with the object's only for functions that save few registers).
* `assign_before_first(...)`, `enclosing_block(...)`: the shared edit (`value` assigned before the first of the sites,
  the sites replaced by the name, one `(old, new)` block over the lines first..last).
* `hoist_order(decl, mapping, text, function) -> Change` (idea 324, `--apply-order`): the plan's declaration order with
  its block-scope locals hoisted: every local the plan names becomes one declaration in one run at the first
  function-scope declaration's line (after the `{` when there is none), in the plan's order; an initialiser stays on
  its line as an assignment (`T v = e;` -> `v = e;`), a block declaration without one is removed. A block-scope local
  declared twice in the function (two blocks each name an `i`), a `for` variable and an initialised `const` stay in
  place and are named in the summary. `Refused` when the plan lists no block-scope local, names a local the source has
  no declaration for, has nothing left to move, or the touched lines are not unique in the file.
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
`--local-name`, and the ambiguity, call-count and `&&` refusals; `test_constant_local` (the rewrite, the counter
refusal, no `= K;`, a `case` label between the uses, a use outside the first one's block), `test_sum_local` (the
rewrite, a pointer operand, an operand written between the uses, no `A + B`), `test_call_result_local`,
`test_declare_dispatch` and `test_hoist_order` (the order and the assignments, names declared twice stay, plans
with no block local or an unmapped one refused).

## Known gaps

A temp defined by a load (`lwz T,...`, a field or a global), a fold, a loop with a braceless body and a constant whose
uses are call arguments, returns or compares (only `<lvalue> = K;` statements are rewritten) are refused with the
manual edit. `constant_local`'s type is always `s32`: a pointer-field store of the local does not compile and the
trial is reverted. The `mr T,r3` copy needs the dump's `bl` count to equal the object's (an inlined or duplicated
call breaks it). `hoist_order` does not hoist a `for` variable, so a plan that needs one is spelled by hand.
The plain `hoist` (live across a call) still declares its local in place; its position in the order is not searched.
