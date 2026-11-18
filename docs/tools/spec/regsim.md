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
                                       [--seed N] [--require-exact] [--tryvar [--keep]] [--apply-local [--local-name N] [--dry-run]] [--apply-order [--dry-run]] [--json]
python tools/mwcc-debugger/regsim.py run UNIT FUNCTION [--no-search] [--gdb G] [--size N] [--tryvar [--keep]] [--apply-local [--local-name N] [--dry-run]]
                                       [--apply-order [--dry-run]]
```

`run` (usage line for lanes: `python .claude/skills/mwcc-unit-matching/scripts/mt.py regsim run <unit> <function>`):
compiles the unit with its own compiler and, when that is not Wii/1.3, with Wii/1.3 as a stand-in (unknown options
dropped, named), dumps `FUNCTION` (plain, mangled, `Class::method` or address) with the unit's real command line
(`-pragma "cats off"` quoted), into `.pi/tmp/regsim-run/<unit>/<function>/`, finds or fetches gdb into `.pi/tmp/mwcc-dbg`,
prints `stand-in check: IDENTICAL` (trust the dump) or `DIFFERS` (a hint only), then `verify` and `search`
(`--search` is accepted, searching is the default; `--no-search` stops after `verify` and prints no plans, so it ignores `--tryvar`, `--apply-local` and
`--apply-order`). `--size N` is the `symbols.txt` size that picks one of several functions of a name (decimal or `0x`). `--ours` for the search is the
stand-in's object so the instruction counts agree with the dump.

`--apply-local` (search/run) writes the best plan's local - for a declared temp by what defines it: a walking pointer
for `addi T,L,K`, a named constant for `li T,K`, a call's result for `mr T,r3`, a sum for `add T,A,B`; or a hoisted
call result for a temp held across a call that retail computes after it (`spec/lib-localedit.md`) - runs it as a
unittry trial and keeps it only when the unit's matched bytes rise with no row down. A shape it cannot identify
unambiguously is refused (exit 3, nothing written) with the edit to make by hand; `--dry-run` prints the edit only.

`--apply-order` (search/run, idea 324) takes the best plan that lists block-scope locals (`other line N, block K`) and
needs no edit, declares every local it names at function scope in the plan's order (`localedit.hoist_order`: initialisers
become assignments in place, a name declared in two blocks stays), runs it as one unittry trial and keeps it only when
the unit's matched bytes rise with no row down. No such plan, or a shape it cannot write, is refused (exit 3);
`--dry-run` prints the edit only. A kept trial rebuilds the unit's object and `build/RMHE08/report.json`
(`spec/unittry.md`), so `mt.py diff` reads the kept source.

`DIR` is a `mwcc_debugger.py` output directory (`regalloc-gpr-pass-N-all.txt`, `-assigned.txt`, the `backend-*`
dumps). Get one with `python tools/mwcc-debugger/mwcc_debugger.py --gdb GDB -a '<unit command line>' FUNCTION DIR`
(the tool's README has the command line).

* `verify`: the select order (first coloured first) with each node's scan, degree, spill cost, `stalled`/`FORCED` mark,
  the model's and the dump's register (callee-saved and stalled nodes; `--all` adds the rest). Exit 0 `EXACT`, 1 on any
  divergence (`DIVERGES colour|order ...`).
* `want`: every vreg's register in our object and in retail's (`<- differs`), from `-u UNIT`'s
  `build/RMHE08/src/<unit>.o` and `obj/<unit>.o` (or `--ours`/`--retail`). A note when our object does not have the
  dump's instruction count (rebuild it from the dumped source) or a vreg retail spells two ways.
* `search`: the current order's hits and stalled nodes, then the best `--top` plans: hits (callee-saved / volatile),
  rewrite cost, the edit if any (with the node's first PCode line), the declarations in order with their source line
  and scope, what still differs, the stalled nodes. The parameter count and the locals come from the unit's source
  (`--source`/`--function`/`--params` override); `--want` aims at registers given by hand; `--saved-only` drops the
  volatile targets. A target on a vreg that is not a coloured node of the pass's graph (retail's registers are read
  from the last pass's code, so a later pass's spill temp) is dropped with a note. Exit 0 when the plans are printed
  (`EXACT` on a plan, and a closing `no plan reproduces every callee-saved target` line, say which);
  `--require-exact` exits 1 unless the best plan reproduces every callee-saved target.
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

Reads the dump directory, the two objects and the unit's source; prints to stdout. Writes only the `--tryvar`/`--apply-local`/`--apply-order` spec
(`.pi/tmp/regsim/<function>.spec`); unittry then edits and restores the unit's source as its own spec says.

## Invariants and rules

* The model, the numbering and the per-vreg attribution are `lib/regsim`'s (`spec/lib-regsim.md`); this tool adds no
  rule of its own.
* A plan is a prediction: the renumbering it names is what the colouring needs, and the source spelling (which
  declaration, which block) is the lane's to write and measure. A plan that moves a declaration out of its block, or
  makes an `@` web or temp a local, says so.
* The dump must come from the source our object was built from; `want` says when the instruction counts disagree.

## Lib dependencies

regsim, regsimrun, localedit, objcompare (function bytes), units, repo.

## Test contract

Tier: fixture - `tools/tests/mwcc-debugger/test_regsim.py`: `verify` exits 0 on the real fixture dump and 1 after a
register is corrupted; `search --want` on the fixture names the `@4192` edit and exits 0, `--no-edits` exits 0
saying no plan is exact and 1 with `--require-exact`, a target that is no node of the graph is a note, not a crash; the
`--tryvar` spelling splits an initialiser off a reordered run, writes nothing for the source's own order and refuses
a plan with an edit; `test_apply_order` (`--apply-order`'s plan choice and refusals, `--dry-run`, and `run --help` saying
`--no-search` prints no plans); the run writes nothing into the repository. Measured by hand: a `--tryvar` run on
`ef/ef_drawsmoothstripestrategy` scored the rewrite (one row 100 -> 99.89), reverted it and restored the file.

Real rows (2026-11-17): `run fn_8004CAD8 my_set_equip_id_swap --apply-local` writes `u16* part`, the order set/part/i/j
and `for (j = 0, part = set->parts_0x02; j < 2; part++, j++)` (the fix batch 5 wrote by hand), unit 98.7907 -> 98.8102;
`userdata_hunter_rank_info_get` hoists `hunter_rank_points_get(999)` into a local before its `if`, 98.8102 -> 98.8449;
`run homebutton/tiHKBManager hkbManagerSetMap` (GC/3.0a5.2): stand-in IDENTICAL, the plan folds r43 into a dead local
and `--apply-local` refuses saying so. Selftests: `tools/tests/lib/test_regsimrun.py`, the `rewrite_runs` extra-local
case in `test_regsim.py`.

## Known gaps

`--tryvar` spells permutations only (no edits, no hoists across blocks); FPR colouring is not modelled; two-edit plans
(the `ef_smooth_ribbon_loop` rewrite needed a new variable and several moves) are out of reach of one search.
