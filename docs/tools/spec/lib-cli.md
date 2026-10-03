# `lib/cli` - The tool entry point: parser with the common flags, exit policy, JSON output, and the test registry

## Purpose

One entry point for a tool: a parser carrying the common flags it chooses, `--selftest` forwarding while the
flag exists, the exit convention applied to whatever `main` returns, and the JSON schema on `--json`.

## Users

`units/declclash.py` (the first tool on it), `tools/selftest.py` (`SELFTEST_FLAG`).

## Public API

* `Tool(name, spec="", tests=None, description=None, common=COMMON)`: `parser(**kwargs)`, `run(main, argv=None,
  parser=None)`, `selftest(cwd=None)`.
* `COMMON = ("json", "root", "main", "limit", "dry_run", "quiet")` - the flags `--json`, `--root` (default `.`),
  `--main`, `--limit N`, `--dry-run`, `--quiet`.
* `SELFTEST_FLAG`: how the runner recognises a tool that runs its own selftest.

## Invariants and rules

* **Exit convention** (`lib.findings.exit_code`): 0 ok, 1 findings or a refusal, 2 could not run. `main` may
  return a `Verdict`, an int, a bool or None; an exception other than `SystemExit`/`KeyboardInterrupt` is printed
  on stderr with `<tool>: could not run: <why>` and exits 2. A `parser.error` keeps argparse's exit 2.
* **`--json` with a `Verdict`** prints the one schema (`{"tool", "rows", "ok", "summary"}`); a tool that prints its
  own JSON returns an int and keeps its shape.
* **`--selftest` is a forwarding shim** (until the WP6 sweep): `tests` is a callable (run in-process) or a path
  relative to the repository root (run with this interpreter in `cwd`). The flag exists only when `tests` is set.
* A tool keeps its own description and its own flags; the common flags are opt-in per tool (`common=`), so
  adopting `Tool` never adds a flag a tool does not honour.
* `SELFTEST_FLAG` sees `add_argument("--selftest"` or `Tool(..., tests=...)`, never a mention in prose.

## Absorbs (today's implementations)

`selftest.SELFTEST_FLAG`; `declclash`'s parser and selftest dispatch.

## Lib dependencies

`lib.findings`.

## Test contract

Tier: fixture (`tools/tests/lib/test_cli.py`). Every common flag parses with its default, a subset, an unknown
flag refused; the exit policy for None/int/Verdict/exception and JSON only on `--json`; `--selftest` to a callable
and to a script path; the discovery regex.

## Known gaps

* 90 tool files still build their own `ArgumentParser` (91 on `main`); they move onto `Tool` with their family
  packages (WP3), and the `--selftest` flags go in WP6.
* `--root`/`--main` do not yet resolve a `Tree` (`lib.repo`); a tool reads `args.root` as before.
* The prologue line check stays in `tools/tests/lib/test_prologue.py` (one copy); the spec's "prologue line check" is not
  duplicated here.
