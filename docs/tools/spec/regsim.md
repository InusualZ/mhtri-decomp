# `regsim` - Replay MWCC's GPR colouring from a mwcc-debugger dump and search the declaration order retail needs

## Purpose

Checks that a function's register colouring is reproduced by the Wii/1.3 allocator model (`lib/regsim`, playbook ideas
243/244), reads retail's register for every virtual register from the two objects, and searches the declaration
orders (and single source edits) whose colouring gives retail's registers, printing the order to write.

## Users

Unit lanes on a register-only residual (`decompiler`, `fixer`) through
`python .claude/skills/mwcc-unit-matching/scripts/mt.py regsim`; the `mwcc-unit-matching` skill.

## CLI

```
python tools/mwcc-debugger/regsim.py verify DIR [--pass N] [--all] [--json]
python tools/mwcc-debugger/regsim.py want   DIR -u UNIT [--symbol S] [--ours OBJ --retail OBJ] [--json]
python tools/mwcc-debugger/regsim.py search DIR -u UNIT [--symbol S] [--source F] [--function F] [--params N]
                                       [--want rV=rP,...] [--saved-only] [--no-edits] [--max-evals N] [--top N]
                                       [--seed N] [--tryvar [--keep]] [--json]
```

`DIR` is a `mwcc_debugger.py` output directory (`regalloc-gpr-pass-N-all.txt`, `-assigned.txt`, the `backend-*`
dumps). Get one with `python tools/mwcc-debugger/mwcc_debugger.py --gdb GDB -a '<unit command line>' FUNCTION DIR`
(the tool's README has the command line).

* `verify`: the select order (first coloured first) with each node's scan, degree, `stalled`/`FORCED` mark, the
  model's and the dump's register (callee-saved and stalled nodes; `--all` adds the rest). Exit 0 `EXACT`, 1 on any
  divergence (`DIVERGES colour|order ...`).
* `want`: every vreg's register in our object and in retail's (`<- differs`), from `-u UNIT`'s
  `build/RMHE08/src/<unit>.o` and `obj/<unit>.o` (or `--ours`/`--retail`). A note when our object does not have the
  dump's instruction count (rebuild it from the dumped source) or a vreg retail spells two ways.
* `search`: the current order's hits and stalled nodes, then the best `--top` plans: hits (callee-saved / volatile),
  rewrite cost, the edit if any (with the node's first PCode line), the declarations in order with their source line
  and scope, what still differs, the stalled nodes. The parameter count and the locals come from the unit's source
  (`--source`/`--function`/`--params` override); `--want` aims at registers given by hand; `--saved-only` drops the
  volatile targets. Exit 0 when the best plan reproduces every callee-saved target, 1 otherwise.
* `--tryvar` writes the best plan as a `tools/units/unittry.py` spec under `.pi/tmp/regsim/` and runs it on the unit
  with `--restore` (`--keep` keeps a winning rewrite). Each scope's declaration lines are slots: without an
  initialiser each slot takes the declaration the plan puts there; with one, every plain declaration goes to the first
  slot in the plan's order and each initialiser stays on its line as an assignment. It refuses a plan that needs an
  edit or moves a declaration between blocks, and a `for` or `const` local that would move. Exit status is unittry's.
* `--json` (each subcommand): `verify` `{function, nodes, exact, divergences, select}`; `want` `{function, want,
  notes, instructions, paired}`; `search` `{function, params, evals, current, plans: [{saved, volatile, exact,
  distance, edits, stalled, decl: [{vreg, name, line, scope}]}], notes}`.

Exit 2 (SystemExit) when the dump is missing, the symbol is not in an object, or the parameters cannot be told.

## Inputs and outputs

Reads the dump directory, the two objects and the unit's source; prints to stdout. Writes only the `--tryvar` spec
(`.pi/tmp/regsim/<function>.spec`); unittry then edits and restores the unit's source as its own spec says.

## Invariants and rules

* The model, the numbering and the per-vreg attribution are `lib/regsim`'s (`spec/lib-regsim.md`); this tool adds no
  rule of its own.
* A plan is a prediction: the renumbering it names is what the colouring needs, and the source spelling (which
  declaration, which block) is the lane's to write and measure. A plan that moves a declaration out of its block, or
  makes an `@` web or temp a local, says so.
* The dump must come from the source our object was built from; `want` says when the instruction counts disagree.

## Lib dependencies

regsim, objcompare (function bytes), units, repo.

## Test contract

Tier: fixture - `tools/tests/mwcc-debugger/test_regsim.py`: `verify` exits 0 on the real fixture dump and 1 after a
register is corrupted; `search --want` on the fixture names the `@4192` edit and exits 0, `--no-edits` exits 1;
the `--tryvar` spelling splits an initialiser off a reordered run, writes nothing for the source's own order and refuses
a plan with an edit; the run writes nothing into the repository. Measured by hand: a `--tryvar` run on
`ef/ef_drawsmoothstripestrategy` scored the rewrite (one row 100 -> 99.89), reverted it and restored the file.

## Known gaps

`--tryvar` spells permutations only (no edits, no hoists across blocks); FPR colouring is not modelled; two-edit plans
(the `ef_smooth_ribbon_loop` rewrite needed a new variable and several moves) are out of reach of one search.
