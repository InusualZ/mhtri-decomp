# `lib/findings` - The one shape for a check's result, its add-only comparison, its rendering and its exit code

## Purpose

The one shape for a check's result, its add-only comparison, its rendering and its exit code.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Finding(rule, file, line, token, detail, remedy)` + `identity()`; `Row(name, status in {PASS, FAIL, UNKNOWN, SKIP}, detail, evidence, remedy, kind)`; `Verdict(rows)`: `ok`, `failed_kinds`, `summary`
* `added(before, after, credits)`: the stylelint credit model (rename, move, deleted file); `render_table`, `render_json` (`{tool, rows, ok, summary}`), `exit_code(verdict)`: 0 ok, 1 findings, 2 could not run

## Absorbs (today's implementations)

`land.check` rows, `stylelint._finding/finding_identity/added_identities/apply_move_credits`, `vtableaudit.violation_rows/keys`, `undefrefs.check_object`, `datagap.strict_verdict`, `splitcheck.Results`, `flipcheck.check`, `verifyunit.*_problems`, `symbolpreflight.severity_for`, `dataclaim.classify`, `handoff.validate`

## Test contract

Tier: fixture (a lib test never reads the live tree). the credit model rows of `stylelint`'s selftest; JSON round trip; exit codes

## Known gaps

None until implemented; `migration.md` names the package.
