# `lib/declperm` - A function's leading local declarations (plain or initialised) and the orders to try them in

## Purpose

Finds the run of local declarations (with or without an initialiser) at the top of a function, enumerates (or samples) the orders of that run and
rewrites a source text to one of them. MWCC assigns stack slots and registers in declaration order, so the order is
match evidence for a function's leading locals.

## Users

`tools/flags/tryvar.py --permdecl` (and `mt.py permdecl`, which forwards to it).

## Public API

* `find_run(text, function, max_lines=6, initialised=True) -> Run | None` - the first `max_lines` single-line
  declarations after the function's opening brace: `    Type [*]name[N];` and, with `initialised`, `    Type [*]name[N] = expr;`
  (a line opening with `return`/`else`/`goto`/... is a statement). With `initialised=False` only plain lines count and
  leading initialised ones are skipped (the pre-2026-10-07 reading, `--plain-only`). `None` when the function is absent
  or fewer than two lines qualify; `ValueError` when the name is defined more than once.
* `Run` - `function`, `start`, `end` (offsets into the LF text), `lines`, `block`, `deps` (per line, the earlier lines
  whose declared name its initialiser mentions: `dependencies(lines)`).
* `orders(n, cap, seed=0, deps=()) -> (orders, sampled)` - every order but the identity when there are at most `cap`, else
  `cap` distinct ones from a fixed-seed shuffle (never the identity), so a rerun tries the same set. With `deps` only
  orders that keep each line after its dependencies are produced (a depth-first enumeration, else random topological
  draws), so `int c = a * 2;` never moves before `int a = ...;`.
* `reorder(text, run, order)` - the text with the run's lines in `order`, replaced **where the run was found**; `None`
  when the text no longer holds the run there.
* `variants(run, orders)` - `[(name, callable)]` in `tryvar`'s variant shape; `name_of(order)` is `order-2,0,1`.

## Invariants and rules

* The run is replaced by offset, never by `str.replace` of its text: two functions often open with the same lines.
* An initialised declaration is a unit of the run, moved whole and never before a declaration its initialiser names. Two
  initialisers with side effects can swap: the compile and the score decide, which is the point of the probe.
* Text is LF; the caller normalises line endings and restores them (`tryvar` keeps a file's CRLF).

## Test contract

Tier: fixture, in `tools/tests/flags/test_tryvar.py` (`test_declperm_run_and_orders`, `test_declperm_initialised`).
Replacing the first copy of the block instead of the found offset fails 4 checks; ignoring `deps` fails 4 (orders that
put `c = a * 2` before `a`).

## Known gaps

The function head must open on one line (`...name(args) {`); a head wrapped over lines is not found. A declaration over several
lines, a `;` inside an initialiser string and a declarator list (`int a = 1, b = 2;`, only the first name is tracked) end or
blur the run. Only the leading run is permuted - declarations after a statement are not.
