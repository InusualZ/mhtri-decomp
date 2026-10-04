# `stylelint_rules` - The section 6.5 lint as a package: one module per rule, the lint, the add-only comparison, the reports

`tools/units/stylelint_rules/` (WP3d). The entry point stays `tools/units/stylelint.py` (the gate, the profiles and
`tools/selftest.py` call it); it star-imports `api` and resolves any other name through it, so every `sl.<name>` an
importer spells (`land`, `backlog`, `dataclaim`, `methodize`, the tests) keeps working. The rules themselves and the CLI
are `stylelint.md`'s; this spec is the package layout.

## Purpose

Holds the lint the way design.md section 5 decomposes it: a module per rule with one `findings(...)` function, the lint
that runs them over a file and the header tree, the `--diff`/`--ref` judgement, the git side of a comparison, the reports
and the command line - so a rule is read, changed and tested in one place.

## Users

`tools/units/stylelint.py` (the only importer outside the package: `stylelint.py -> api.py` is its one layering edge).

## CLI

None: modules. The CLI is `stylelint.py`'s (`cli.main`), spec `stylelint.md`.

## Modules

| module | holds |
| --- | --- |
| `common` | paths (`SRC`, `HEADERS`, `UNSPLIT`...), `RULE_NAMES`, `EXEMPT`/`RULE7_NOTES`/`UNCHECKED`, `Source` (a `lib.cscan.Text` with paths), the `lib.cscan` views, `type_defs`, `field_name` + `Field`/`field_walk` (the one field walk rules 4, 5 and 7 read), `_finding`/`_rule2_finding`, `_untyped_marker` (rule 11's and 13's marker window), the tree walks |
| `context` | the rule-2 `Ownership` (lib.project's index plus the run's counters), `load_ownership`, the open STOPGAP request ids |
| `r01_shared_type` ... `r13_method` | one rule each: its regexes and its finding function (`rule1_findings`, `rule2_*`/`stopgap_findings`, `r03..r09.findings`, `codegen_pragma_findings`, `rule11_findings`, `rule12_findings`, `rule13_findings`); r13 also holds `Rule13Context` and `set_rule13_context` |
| `lint` | `lint_source` (every rule over one `Source`), `lint_tree`, `lint_all`, the header-tree walks |
| `diff` | identities, rename credits, move/split credits, the added-finding detail, and `judge` - the one judgement `--diff` and `--ref` share |
| `refs` | `git`/`git_bytes`, the ref's map, the changed pairs, the base/ref copies linted, `texts_at_ref`/`headers_at_ref` |
| `report` | the budget, the distinct-name counts, the rule-2 shape, the `--findings` listing |
| `cli` | `main`, `ref_comparison`, `report_comparison`, `_resolve_diff_ref` |
| `selftest` | the lint's selftest (445 checks), on fixtures and temporary git trees |
| `api` | the facade: every name `stylelint.py` had, plus the split's new ones, with `__all__` |

## Invariants and rules

* **The split moved code, not behaviour.** `lint_source` runs the rules in the order the monolith did (3, 4, 5, 6, 7, 8,
  9, then 2, 12, 11, 13 and the STOPGAP check) and sorts by `(rule, line)` with a stable sort, so the order inside one
  `(rule, line)` is still each rule's own report order.
* **The seams are module attributes.** A name a test stubs or a setter rebinds is read as `_refs.git` / `_refs.git_bytes`
  outside `refs`, never copied into another module's namespace; the selftest stubs `refs.git_bytes`. The open-request
  cache (`context._OPEN_IDS`) and the rule-13 registry (`r13_method._RULE13_CTX`) are rebound only by their setters.
* **A tree at a ref is read once.** `texts_at_ref` reads every blob under `include/` (or `src/`) at a ref in one
  `git cat-file --batch` (`lib.git.Git.show_many`) and keeps it for the process, keyed by the ref's **tree** id (a moving
  branch name cannot serve a stale tree); `headers_at_ref` builds the `Source`s once per (tree, rename) for the four
  header walks. A blob that cannot be read is skipped, exactly as a failed `git show` was; the changed pairs are still
  read one `git_bytes` call per file (the selftest blinds one of them).
* **One judgement.** `diff.judge` is the credit pipeline (`rename_map`, `added_identities`, `apply_move_credits`,
  `apply_rename_credits`, the detail and the credit lines) for both comparisons; `cli.report_comparison` is their one
  renderer. `--diff` and `--ref` differ only in where each side is read from and in their summary sentence.
* **A move that also renamed is a move.** `apply_move_credits` matches an addition against the removals keyed
  `(rule, token, detail)`; `lib.findings.removed` spells each removal's key the way the after side does
  (`renamed_finding` through `rename_map` and the file map), so a finding whose map row the batch renamed *and* which
  moved file in the same batch (L3, 2026-10-04: `lbl_80794868` -> `frame_counter`, the rule-12 extern from
  `src/main.cpp` to `include/unsplit/unknown.h`) is one credited move, not +1. The same rule, the same renamed token,
  one credit per removal; another rule, or a copy that leaves the original in place, is still added. Every rule whose
  token or detail names the symbol (2, 7, 11, 12, 13 ...) is covered, because the translation is on the finding, not
  per rule.
* **Rule 13's mangling estimate is `lib.names`.** It was `import mangle`, which resolved only while `tools/units/` was on
  `sys.path` (a script in that directory); a caller from anywhere else (a `tools/tests/` module, `python -m`) silently got
  no estimate and a different detail text. Every script's output is unchanged.
* **The selftest flag is the entry point's.** `stylelint.py` registers the selftest with `lib.cli.Tool(tests=...)`, the
  shape `tools/selftest.py` discovers; `cli.py` spells `--selftest` through a constant so the runner does not mistake a
  package module for a second entry point (`tools/units/merge/*.py` show that failure mode: each is listed as a tool).

## Lib dependencies

cscan, findings, git, names, project (`Ownership`, `SymbolMap`, `Splits`), requests, repo (lazily, for the outbox dirs).
One tool edge: `diff -> tools/units/dataclosure.py` (the fold map `derive_file_absorbers` reads; it was
`stylelint -> datagap`, which only re-exported it).

## Test contract

`python tools/units/stylelint.py --selftest` (the runner's entry `tools/units/stylelint`): 445 checks, unchanged by the
split; 451 with the move+rename rows (two of them fail on the old `removed` keying). Re-homing it to `tools/tests/units/test_stylelint.py` (fixture tier; the one live read is
`load_ownership(".")`, which the runner's temp cwd turns into "no map") is WP6's.

## Measured (WP3d)

On the tree at `1c0e5252b` (+ main through `af8b41844`), old monolith vs the package, same machine:

* `--budget --json`: 50 084 findings, identical bytes; `--budget`, `--budget --headers`, `--findings`, `--findings --json`
  identical bytes.
* `--diff` on four refs identical (stdout and stderr): `main` (0 changed files), `0375f98c4~1` (81 changed, 8 moves),
  `ee7d53b06~1` (170 changed, text and JSON), `3f7532ded~1` (471 changed: 58 added rows, 1 122 detail rows, 4 152 moves,
  5 rename credits, exit 1). `--ref` identical on an unreferenced commit replaying `3f7532ded` over its parent (39 changed
  files, 671 moves) and on the refusing self-comparisons.
* Wall time of `--diff`: 66 / 70 / 78 / 87 s -> 10 / 17 / 26 / 45 s (the four header walks and rule 1 at the ref read
  ~2 650 blobs one `git show` each; now one `cat-file --batch` per tree). `--budget --json` 25 s before and after.

## Measured (move + rename, 2026-10-04)

On main at `dadbdf8e9`, old vs new `lib.findings.removed`: `--budget --json`, `--budget` and `--diff` on `main`,
`0375f98c4~1` and `ee7d53b06~1` (text with `--list-added`, and JSON) identical bytes. `--diff 3f7532ded~1` (a batch with
renames) credits 13 more moves (4 147 -> 4 160; additions 1 123 -> 1 110), each a re-homed owner path or a renamed token
moving out of a folded file. `--ref worker/net2-l3-2c54` (105fe4cc6): +1 rule 12 -> clean, one move credited.

## Known gaps

* The selftest is still one function in the package (`selftest.py`), not a `tools/tests/` fixture module.
* Three regexes say "a type definition starts here": `lib.cscan.STRUCT_RE` (rules 3-5: `struct`/`class`, no base clause),
  `common.RULE1_TYPE_RE` (rules 1 and 13: `union` too, and a base clause) and `lib.cscan.CLASS_OPEN_RE` (vtableaudit:
  `typedef` and a base clause). Unifying them changes the lint's finding set (a `struct B : A {` gains rule 3-5 findings),
  so it needs a ruling rather than a refactor.
* `common._unit_stem` (the file->unit key of the split credit) keeps stripping any extension, so a private header
  `src/x/foo.h` maps to unit `x/foo`; `lib.units.stem` would leave `.h` on and change the credit, so it is not collapsed.
* The changed pairs of a comparison are read with one `git show` each (and `--ref` reads each branch copy three times);
  batching them needs the selftest's blinding stub to move to the batch reader.
