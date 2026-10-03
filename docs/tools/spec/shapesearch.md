# `shapesearch` - Search source shapes (or hand-written candidate bodies) of one function for the target's codegen; scores with the official metric in scratch

<!-- generated from the module docstring of `tools/flags/shapesearch.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Search *source shapes* of one function for the codegen the original object has.

## Users

skills (8); CLAUDE.md (1); docs (12)

## CLI

```
python tools/flags/shapesearch.py -u Pl/pl_act                     # worst function, all generators
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --gens switch,cond --depth 2
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --top 10 --jobs 8
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --emit <label>   # dump the winner
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --expr "return 0;" --expr "return 1;"
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027BC48 --expr-file cands.txt
python tools/flags/shapesearch.py --list-gens
```
Flags: `--beam`, `--depth`, `--emit`, `--expr`, `--expr-file`, `--gens`, `--jobs`, `--list-gens`, `--max`, `--per-gen`, `--scan`, `--selftest`, `--top`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit, function -> ranked table, build/tmp/shapes.

## Invariants and rules

* The loop is the one tryvar already uses for flags, lifted to the source:
```
generate variants -> compile each with the unit's *real* ninja command line -> score each with
objdiff's official report metric -> deduplicate identical objects -> rank
```
* Nothing here writes to the repository's source: every candidate is compiled from a scratch copy under `build/tmp/shapes/<run>/`, and only a single object is compiled (no link, no `ninja` build edge). The score is `unitutil.report_functions` - `report generate`'s `fuzzy_match_percent`, the number `build/RMHE08/report.json`, `ledger.py` and `land.py` read - not objdiff `diff`'s positional value.
* Ranking is by the target function's official score; the table also shows the unit mean and the number of functions the variant regressed, so a shape that fixes the function by breaking its neighbours is not mistaken for a win.

## Lib dependencies

units, report, cscan (shapes).

## Test contract

Tier: fixture (offline).
Today's selftest (`tools/flags/shapesearch_selftest.py`): Offline: no compiler, no target object, no repository state. It pins the pieces that turned "twelve spellings hand-written into a scratch `.c`" into a mode: * `expr_candidates` - the `--expr` order, the `---` file separator, the one-line-per-candidate file, the `#` comment skip, the `shapes.norm_code` dedupe, and a missing file refusing instead of scoring zero; * `build_parser` actually wires `--expr`/`--expr-file` (the flag was the whole gap); * `divergence_line` names the first differing row and its kind, or says MATCH - the "per candidate, its score and first divergence" half that the score alone does not answer.
Target: `tools/tests/flags/test_shapesearch.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* This is the source-side twin of `tools/flags/tryvar.py`: where tryvar varies the compiler flags, this varies the source - declaration order and types, named temporaries, casts and signedness, statement order, compound assignment vs assignment, field form vs pointer arithmetic, dead copies, the switch tail and `default`-first shapes, condition/branch form, ternaries and loop shape. It exists because the residual on a near-matching unit is almost always *codegen* (allocator web order, a branch direction, a register colouring) and finding it by hand cost the earlier sessions hundreds of hand-written variants per function.
* `--expr` / `--expr-file` are the **candidate-expression** mode: instead of the generated source shapes, each candidate is a body you write, compiled with the unit's real cflags and scored with the same machinery as the search, and each is printed with its score **and its first divergence**. A lane that hand-wrote twelve spellings into a scratch `.c` to attribute one residual did the same thing by hand; a file separates its candidates with a line of `---` (without one, each non-empty non-`#` line is one), and `--emit <label>` dumps a candidate's full source plus its body diff.
