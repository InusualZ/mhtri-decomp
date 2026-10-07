# `lib/declperm` - A function's leading plain declarations and the orders to try them in

## Purpose

Finds the run of plain local declarations at the top of a function, enumerates (or samples) the orders of that run and
rewrites a source text to one of them. MWCC assigns stack slots and registers in declaration order, so the order is
match evidence for a function whose locals carry no initialiser.

## Users

`tools/flags/tryvar.py --permdecl` (and `mt.py permdecl`, which forwards to it).

## Public API

* `find_run(text, function, max_lines=6) -> Run | None` - the first `max_lines` lines after the function's opening brace
  that read `    Type [*]name[N];` with no initialiser, after skipping leading initialised declarations. `None` when the
  function is absent or fewer than two lines qualify; `ValueError` when the name is defined more than once.
* `Run` - `function`, `start`, `end` (offsets into the LF text), `lines`, `block`.
* `orders(n, cap, seed=0) -> (orders, sampled)` - every order but the identity when there are at most `cap`, else `cap`
  distinct ones from a fixed-seed shuffle (never the identity), so a rerun tries the same set.
* `reorder(text, run, order)` - the text with the run's lines in `order`, replaced **where the run was found**; `None`
  when the text no longer holds the run there.
* `variants(run, orders)` - `[(name, callable)]` in `tryvar`'s variant shape; `name_of(order)` is `order-2,0,1`.

## Invariants and rules

* The run is replaced by offset, never by `str.replace` of its text: two functions often open with the same lines.
* An initialised declaration is not a member of the run (reordering it reorders its side effects).
* Text is LF; the caller normalises line endings and restores them (`tryvar` keeps a file's CRLF).

## Test contract

Tier: fixture, in `tools/tests/flags/test_tryvar.py` (`test_declperm_run_and_orders`). Replacing the first copy of the
block instead of the found offset fails 4 checks.

## Known gaps

The function head must open on one line (`...name(args) {`); a head wrapped over lines is not found. Only the leading
run is permuted - declarations after a statement are not.
