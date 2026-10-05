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
| `common` | paths (`SRC`, `HEADERS`, `UNSPLIT`...), `RULE_NAMES` (1-15), `AUDIT_RULES` (rule 10 is vtableaudit's: no lint finding carries it), `EXEMPT`/`RULE7_NOTES`/`UNCHECKED`, `Source` (a `lib.cscan.Text` with paths), the `lib.cscan` views, `type_defs`, `field_name` + `Field`/`field_walk` (the one field walk rules 4, 5 and 7 read), `_finding`/`_rule2_finding`, `_untyped_marker` (rule 11's and 13's marker window), the tree walks |
| `context` | the rule-2 `Ownership` (lib.project's index plus the run's counters), `load_ownership`, the open STOPGAP request ids |
| `r01_shared_type` ... `r15_comments` | one rule each: its regexes and its finding function (`rule1_findings`, `rule2_*`/`stopgap_findings`, `r03..r09.findings`, `rule11_findings`, `rule12_findings`, `rule13_findings`, `codegen_pragma_findings`, `r15_comments.findings`); r13 also holds `Rule13Context` and `set_rule13_context`, r15 `REFUSING`/`ADVISORY`, `is_advisory`, `ADDRESS_PREFIX_RE` and `set_rule15_context`. There is no `r10`: rule 10 is vtableaudit's, and the pragma check was `r10_pragma` until 2026-10-05 |
| `lint` | `lint_source` (every rule over one `Source`, the one file classifier), `lint_tree`, `lint_all` (every file the lint judges), `HEADER_BODY_RULES[_ON]` (the recommended header rules' one switch), `is_band_rule2`, the whole-tree header walks (API only, plus rule 12's untouched-file view) |
| `diff` | identities, rename credits, move/split credits, the added-finding detail, and `judge` - the one judgement `--diff` and `--ref` share |
| `refs` | `git`/`git_bytes`, the ref's map, the changed pairs, the base/ref copies linted, `texts_at_ref`/`headers_at_ref` |
| `report` | the budget (with `rule10_counts`: the audit's violations per file, by subprocess), the distinct-name counts, the rule-2 shape, the `--findings` listing |
| `cli` | `main`, `ref_comparison`, `report_comparison`, `_resolve_diff_ref` |
| `selftest` | the lint's selftest (533 checks), on fixtures and temporary git trees |
| `api` | the facade: every name `stylelint.py` had, plus the split's new ones, with `__all__` |

## Invariants and rules

* **One classifier, path-free (2026-10-05).** A header is a `HEADER_SUFFIXES` file **anywhere** (`common.is_header`:
  `src/**`, and `include/**` in a ref older than the owner's 2026-10-05 header move); the unsplit band is `lib.project.ownership.BAND_ROOT`
  (`src/unsplit`; `LEGACY_BAND_ROOT` `include/unsplit` still classifies a pre-move ref), the one spelling of its path. `lint_source` runs the body
  rule set on **every** `.c`/`.cpp`/`.h` - 3, 4, 5, 6, 7, 8, 9, then 2 and 12, 11, 13, 14, 15 and the STOPGAP check - and
  only rule 2 (source / header / band reading) and the STOPGAP check (not in the band) read the kind. It sorts by
  `(rule, line)` with a stable sort, so the order inside one `(rule, line)` is still each rule's own report order.
  Rules 3, 4, 5, 6, 8 and 9 on a header are the orchestrator's recommendation (the owner was not asked):
  `lint.HEADER_BODY_RULES_ON = False` switches exactly those off. `_owns` and `leaf_header_owner` recognise an owner's
  header and a leaf header by their stem wherever they live, never by their directory.
* **A finding names no header path.** Identity is `(rule, file, token, detail)` (`lib.findings`), so a detail that
  spelled `src/unsplit/<m>.h` would turn the band move into a removal plus an addition per finding; rule 2's
  unowned detail names the band by its module (`the `ef` unsplit band header`) instead.
* **A comparison lints the changed files completely and walks nothing else but rule 12.** Every rule of a changed or
  deleted file comes from `lint_source` on both sides; the one whole-tree walk left is rule 12 over the **untouched**
  headers (`cli.untouched`), because each side is judged by its own map and a `splits.txt` edit can uncover an untouched
  header's `extern`. The rule 11/13/14 whole-tree walks read identical text on both sides for an untouched file, so they
  contributed no identity and were dropped. `lint_all` (`--budget`) is `lint_source` over `lint_files` (all of `src/`,
  then the headers outside it) plus rule 1; it leaves the band's rule-2 reading to `--headers`, as before.
* **The seams are module attributes.** A name a test stubs or a setter rebinds is read as `_refs.git` / `_refs.git_bytes`
  outside `refs`, never copied into another module's namespace; the selftest stubs `refs.git_bytes`. The open-request
  cache (`context._OPEN_IDS`) and the rule-13 registry (`r13_method._RULE13_CTX`) are rebound only by their setters.
* **A tree at a ref is read once.** `texts_at_ref` reads every blob under `src/` (and a pre-move ref's `include/`) at a ref in one
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
  `src/main.cpp` to `src/unsplit/unknown.h`) is one credited move, not +1. The same rule, the same renamed token,
  one credit per removal; another rule, or a copy that leaves the original in place, is still added. Every rule whose
  token or detail names the symbol (2, 7, 11, 12, 13 ...) is covered, because the translation is on the finding, not
  per rule.
* **Rule 13's mangling estimate is `lib.names`.** It was `import mangle`, which resolved only while `tools/units/` was on
  `sys.path` (a script in that directory); a caller from anywhere else (a `tools/tests/` module, `python -m`) silently got
  no estimate and a different detail text. Every script's output is unchanged.
* **A forward declaration is never a declaration, and a leaf header may carry one (2026-10-04).** Rule 2's
  file-scope walk (`r02_extern._file_scope_declarations`) skips a type-only statement (`struct Foo;`, `class Foo;`,
  `typedef struct Foo Foo;`) - it names a type, not a map symbol - and that was always the intent. It blanks
  preprocessor lines first (`lib.cscan.mask_preproc`): before, a header's guard or `#include` opened the first
  statement's segment, so a forward declaration right after it did not match the type-only pattern and read as a
  declaration of `Foo`, an unowned name, which made `leaf_header_owner` reject the header and reported every symbol
  in it as foreign. The leaf rule itself is unchanged: a symbol of a second unit, or an unowned one, still makes the
  header foreign.
* **Rule 15 is two refusing checks and an advisory count (owner, 2026-10-05).** `stale-path` (a `lib.comments`
  marker hit in comment text - `lib.comments.comment_only`, never code or a literal - whose path the tree does not have)
  and `address` (a function comment directly above a definition - only whitespace and no blank line between - whose
  `0xADDR [(SIZE)]` prefix, `ADDRESS_PREFIX_RE`, names no function row, the definition's own name at another address,
  or a size that is not the row's, read from `Ownership.functions`) are findings like any rule's: identity `(15, file,
  token, detail)`, refused when new to a file. Every other class carries `advisory: True` and `diff.judge` drops it from
  both sides before anything is compared, so it never refuses, moves or credits; `--budget` counts all of them in the
  `r15` column and splits it by class (`report.rule15_classes`, the `budget.rule15` JSON key). Path existence is asked
  of one tree for both sides (`set_rule15_context`, the invocation root), so only a comment change can add a finding;
  `.pi/` is never live. `src/Camellia/` is not read (the vendor's MPL-1.1 comments).
* **The selftest flag is the entry point's.** `stylelint.py` registers the selftest with `lib.cli.Tool(tests=...)`, the
  shape `tools/selftest.py` discovers; `cli.py` spells `--selftest` through a constant so the runner does not mistake a
  package module for a second entry point (`tools/units/merge/*.py` show that failure mode: each is listed as a tool).

## Lib dependencies

comments (rule 15's markers and stale judgement, shared with `sweepcomments`), cscan, findings, git, names, project (`Ownership`, `SymbolMap`, `Splits`), requests, repo (lazily, for the outbox dirs).
One tool edge: `diff -> tools/units/dataclosure.py` (the fold map `derive_file_absorbers` reads; it was
`stylelint -> datagap`, which only re-exported it).

## Test contract

`python tools/units/stylelint.py --selftest` (the runner's entry `tools/units/stylelint`): 445 checks, unchanged by the
split; 451 with the move+rename rows (two of them fail on the old `removed` keying); 454 with the leaf forward-declaration rows (two fail without the preprocessor mask); 510 with the 2026-10-05 classifier, rule 7 exact and file-name rows (35 of the new checks fail on the code before them: 15 classifier, 9 rule 7 exact, 11 file names); 533 with rule 15 (23 checks; each mutation fails at least one: advisory findings kept in `--diff` 1, `.pi/` live where it exists 1, no size check 2, no other-address check 1, code read as comment text 1, a blank line allowed above a function 1, live paths not whitelisted 1, a percentage clause crossing punctuation 2). Re-homing it to `tools/tests/units/test_stylelint.py` (fixture tier; the one live read is
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

## Measured (rule 15, 2026-10-05)

Worktree at `aea522b5e` plus this batch, `--budget --json` old code vs new on the same tree: every column 1-14 identical
per file (1: 8, 2: 3 860, 3: 193, 4: 97, 5: 213, 6: 381, 7: 44 705, 9: 377, 11: 4 813, 12: 129, 13: 32; `r10` 22 here,
the worktree has no built objects), `unique` identical; the new `r15` column is 955, 72 of its files carrying no other
finding, so `findings` 54 808 -> 55 763. By class - refusing 172: `stale-path` 161 over 95 files (`.pi/` 69, retired
tool 31, `proposal/` 23, `auto/<hex>_` 22, `include/` 16; `sweepcomments --list-stale` says 162 for the same markers
without `configure.py`, the one more being a hit outside a comment, which rule 15 does not read) and `address` 11 (3
sizes in `NWC24/nwc24_msg.c`, 8 addresses no function starts at in `Pl/pl_act.cpp`, of 2 022 prefixed function comments
read, 340 of them with a size); advisory 783: `phase 4` 177, `next pass` 19, date 109, `round N` 29, `pilot` 13, `wave`
12, `lane` 49, `header inherited` 40, `percent` 115, `self-name` 220. All are grandfathered by identity.

## Measured (rule 7 file names, 2026-10-05)

Rule 7 44 433 -> 44 710 (+277), total 54 932 -> 55 209: 270 generated stems (70 sources, 200 headers) and 7 directory
findings - three generated directories (`include/fn_80047398`, `include/fn_8004CAD8`, `include/fn_80056F24`) holding 7
files. The design estimate was 273 with one finding per directory; one per file in it is what keeps the identity through
a move. No stem is address-named today.

## Measured (rule 7 exact, 2026-10-05)

On main `2e6610017` plus the classifier, rule 7 42 945 -> 44 433 (+1 488), every other rule identical, total 53 444 ->
54 932; no existing rule-7 finding disappeared (the `#include` mask removed none: quoted paths were already literals). The
additions by class: address-named +836 (508 in headers, 328 in sources; 303 distinct tokens, 418 of the occurrences are
generated headers' include guards), suffix/prefix stems +591 (590 sources, 1 header), `dtor_` +61 (56 sources, 5 headers).
Against the pre-classifier baseline 40 119 the rule-7 jump is +4 314: headers +2 826 (the old spellings), suffix forms
+591, `dtor_` +61, address-named +836. Distinct names: 7 867 `fn_`-class, 4 153 data labels, 303 address-named, 205 bare
`unk`.

## Measured (classifier, 2026-10-05)

On main `2e6610017`, `--budget --json` before vs after the classifier: rules 1, 2, 6, 8, 10, 11, 12, 13 and 14 identical
(1: 8, 2: 3 860, 6: 381, 10: 396, 11: 4 813, 12: 129, 13: 32); the additions are all under `include/`: rule 7 +2 826,
rule 3 +182, rule 5 +146, rule 4 +35, rule 9 +20 - total 50 235 -> 53 444. The recommended header rules (3, 4, 5, 6, 8,
9) are 383 of those; rule 7's 2 826 is the owner's. 99 rule-2 findings changed only their detail (the band path became
its module). Replays (`--diff <c>~1 --json` at `<c>`, old vs new code): `150b7e2dd`, `5a4602a88`, `35d86a517` and
`ee7d53b06` identical; `3f7532ded` +7 (rule 7 in `include/enemy/em_prog_tail.h`, `include/hud/cockpit.h`,
`include/lobby/lbl_806BE340.h`, `include/menu/menu_effect_slot.h`, `include/stage/stg_w.h` x2; rule 9 in
`include/hud/get_lsp_data__FUsP10_mh_ivec2_.h`) and `0375f98c4` +1 (rule 3, `include/Network/session_mediator_views.h`)
- only headers those batches touched.

## Measured (rule 14 renumber, 2026-10-05)

Worktree at `378ec7aaf` (main `35d86a517` plus this batch), `--budget --json` before vs after: every lint column
identical (1: 8, 2: 3 861, 3: 11, 4: 62, 5: 67, 6: 381, 7: 40 185, 8: 0, 9: 357, 11: 4 814, 12: 129, 13: 32),
`findings` 49 907 both; the old `10` (pragma) 0 is the new `14` 0; the new `10` is vtableaudit's 396 violations over
101 files (37 of them files with no lint finding), so `total` 49 907 -> 50 303. Wall time: the audit adds 15 s.

## Measured (move + rename, 2026-10-04)

On main at `dadbdf8e9`, old vs new `lib.findings.removed`: `--budget --json`, `--budget` and `--diff` on `main`,
`0375f98c4~1` and `ee7d53b06~1` (text with `--list-added`, and JSON) identical bytes. `--diff 3f7532ded~1` (a batch with
renames) credits 13 more moves (4 147 -> 4 160; additions 1 123 -> 1 110), each a re-homed owner path or a renamed token
moving out of a folded file. `--ref worker/net2-l3-2c54` (105fe4cc6): +1 rule 12 -> clean, one move credited.

## Measured (forward declarations in a leaf header, 2026-10-04)

On main at `5347af8fa`, `--budget --json` before vs after the preprocessor mask: rule 2 3 921 -> 3 863 (-58), total
49 981 -> 49 923, every other rule identical. The 58 are the whole rule-2 count of four leaf headers that each
forward-declare a type after their guard - `include/ef/fn_800FE978.h` (1, `struct _EFT;`),
`include/enemy/fn_8012BA00.h` (1), `include/enemy/fn_8015D860.h` (8) and `include/enemy/fn_801B4458.h` (48,
`struct Vec;`); each declares only symbols its folded owner defines, so each now reads as that owner's leaf header.
`rule2_gaps['not in symbols.txt']` 599 -> 588 and `unique.extern_symbols` 2 859 -> 2 802 are the same forward-declared
type names no longer counted as declarations. The selftest pins the shape (two checks fail without the mask).

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
