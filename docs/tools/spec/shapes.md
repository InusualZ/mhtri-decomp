# `shapes` - Source-shape generators for `shapesearch.py`

## Purpose

The source-side analogue of `tools/flags/variants/<lib>.py`: each generator rewrites one function's body without
changing its meaning (or at worst into a compile error) so that the IR the allocator and the peephole see changes.
`shapesearch.py` searches over them.

## Users

`tools/flags/shapesearch.py`, `tools/flags/shapes_selftest.py`.

## CLI

None (library).

## Test contract

Tier: legacy (`tools/flags/shapes_selftest.py`).

## Moved from the module docstring (WP6)

From `tools/flags/shapes.py`:

A *shape* is a rewrite of one function's body that leaves the function's meaning alone (or is at worst a
compile error) but changes the IR the allocator and the peephole see. The levers here are the ones the
matching playbook names: declaration order and types (rows 18, 20, 38), named temporaries, casts and
signedness, statement order, compound assignment vs assignment, field form vs pointer arithmetic, dead
copies (row 35), the switch tail and `default`-first shapes (rows 34, 37), condition/branch form,
ternaries, and the loop shape (row 19).

Nothing in here compiles or scores anything: every generator is a pure `body -> [(name, new_body)]`
function over the text between a function's braces, so it can be unit-tested without a compiler. The
driver (`shapesearch.py`) owns the compile/score/dedupe loop.

The transforms are deliberately textual, not a C parser: they must never silently mangle the source into
something the compiler accepts but that means something else, so each one only fires on a narrow pattern
and a rewrite that does not apply yields no variant at all. A generator that produces an uncompilable
variant is not a bug (the driver reports the compile failure and moves on); a generator that produces a
*semantically different* variant is, so the aggressive rewrites (`break` -> `return C`) are opt-in by
generator name.
