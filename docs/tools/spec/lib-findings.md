# `lib/findings` - The one shape for a check's result, its add-only comparison, its rendering and its exit code

## Purpose

A check produces data, not text: a lint-style `Finding`, a gate-style `Row`, folded into a `Verdict`; one add-only
comparison (with the rename/move credit model) for every "does this batch add one" question; one table renderer,
one JSON schema and one exit convention.

## Users

`units/stylelint.py` (`_finding`, `finding_identity`, `added_identities`, `removed_identities`), `units/land.py`
(the check rows: KIND, remedy, the table, the compact listing, `failing_checks`), `lib.outbox` (validation
results), `lib.cli` (the JSON on `--json`, the exit code).

## Public API

* `Finding(rule, file, line, token=None, detail="", remedy=None, text="")`: `identity()`, `to_dict()`,
  `from_dict(d)`; `identity(f)` for a `Finding` or its dict.
* `added(before, after, credit=None) -> {(rule, file): [finding]}`; `removed(before, after, credit=None) ->
  {(rule, token, detail): [file]}`; `credit(f)` yields the other spellings a base finding is known under.
* `Row(name, status, detail="", evidence="", remedy="", kind="gate")`: `Row.check(name, good, ...)`,
  `Row.from_tuple(row)`, `.failed`, `.note()`, `.line()`, `.to_dict()`; `rows_of(items)`.
* `render_table(rows, name_width=58, note_width=80, header=True)`, `render_lines(rows)`.
* `Verdict(rows)` / `Verdict.of(items)`: `.ok`, `.failures`, `.failed_kinds`, `.summary`;
  `render_json(tool, verdict, **extra)`; `exit_code(result)`.
* Constants: `PASS`, `FAIL`, `UNKNOWN`, `SKIP`, `STATUSES`; `KIND_GATE`, `KIND_BOOKKEEPING`, `KIND_TAG`;
  `EXIT_OK`, `EXIT_FINDINGS`, `EXIT_ERROR`.

## Invariants and rules

* **The identity is the token and the detail, never the line** (`(rule, file, token, detail)`): the line moves with
  every edit above it. Keeping `detail` stops two complaints about one token (rule 7's generated name and its
  bare identifier) from collapsing into one.
* **Add-only is a set difference per `(rule, file)`**, not a count delta: spelling an already-flagged token 30
  more times adds nothing; a token new to the file is one addition however often it is spelled, reported at its
  first occurrence by line.
* **Credits**: each base identity is admitted under every spelling `credit` gives it (the old name and the new,
  for a map rename or a re-homed file), so a rename is never one removal plus one addition. `removed` is the
  mirror: one removal per identity per file, unless a credited spelling is still there.
* **A removal is keyed by the after side's spelling**: the first spelling `credit` gives, else the base one. So a
  finding that moved file *and* was renamed in the same batch meets its addition on `(rule, token, detail)` and a move
  credit can match it (stylelint's `apply_move_credits`); without a credit, or when `credit` returns the finding
  unchanged, the key is the base `(rule, token, detail)` exactly as before.
* **A gate row has a KIND** (GATE: the batch is bad; BOOKKEEPING: the landing's own state is stale). A row whose
  kind is missing or unknown reads as GATE - a refusal of unknown kind is never soft-pedalled.
* **Rendering is the gate's table, byte for byte** (lanes parse it): a `check / result` header, `%-58s` names cut
  at 58, the status word, and two spaces plus the note cut at 80 - a failure's detail, else the evidence; no note,
  no trailing spaces. The compact form is `STATUS name - note`.
* **Exit codes**: 0 ok, 1 findings or a refusal, 2 could not run. An int passes through; a `SystemExit` keeps its
  code.
* The JSON schema is `{"tool", "rows", "ok", "summary"}`; a `Finding` serialises as `rule, file, line, token, text,
  detail` (+ `remedy` when set), the key order stylelint's `--json` always had.

## Absorbs (today's implementations)

`stylelint._finding/finding_identity/added_identities/removed_identities`, `land.check_kind/check_remedy/
failed_kinds/failing_checks`'s row reading and the verify table and listing loops, `land.KIND_*`.

## Lib dependencies

None (stdlib).

## Test contract

Tier: fixture (`tools/tests/lib/test_findings.py`). The credit model rows of stylelint's selftest (repeat spelling,
new token, per-file identity, two details, a credited rename and its old-spelling referrer), `removed` (and its
after-side key: a moved+renamed removal meets its addition, an in-place rename is neither, another rule never), the
table's exact bytes (header, PASS evidence, FAIL detail, the cuts, a `None` detail), KIND normalisation, the
Verdict fold, the JSON round trip and every exit code.

## Measured (WP2c)

`stylelint` on the tree at `d82dbf1a2`: the 53 677 finding dicts of `lint_all` + the header rules are byte-identical
before and after; `--budget`, `--budget --json`, `--diff <ref>` (three refs, one with 1 193 additions) and `--ref` on
a probe branch (additions, a moved file's credits, a rename) print identical output. The gate's row renderers
(table, listing, `failing_checks`, `failure_summary`, `failed_kinds`, kind/remedy) are byte-identical old vs new
over a corpus of row shapes.

## Known gaps

* stylelint still passes findings as dicts (`to_dict()` at creation): its rules carry extra keys (`symbol`, rule 13's
  `owner`/`method`/`mangled`/`static`...) that `--diff`, `methodize` and `backlog` read, so the 3d split kept the dict and
  left the move to `Finding` (with an `extra` mapping) to a batch that changes those readers too.
* `land.py`'s checks are still built as tuples and rendered through `Row` (`rows_of`); WP4 builds `Row`s directly.
* Not yet producing these shapes: `stylelint_rules/diff.py`'s `apply_move_credits`/`judge` (their `Judgement` is the
  comparison's own value; `added`/`removed` already come from here), `vtableaudit.violation_rows/keys` (the gate reads
  their keys), `undefrefs.check_object`, `datagap.strict_verdict`, `flipcheck.check`,
  `verifyunit.*_problems`, `dataclaim.classify` (3a), `splitcheck.Results` (3c), `symbolpreflight.severity_for`,
  `unionresolve.check_union` (3f).
