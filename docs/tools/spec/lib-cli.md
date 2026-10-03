# `lib/cli` - The tool entry point: parser with the common flags, exit policy, JSON output, and the test registry

## Purpose

The tool entry point: parser with the common flags, exit policy, JSON output, and the test registry.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Tool(name, spec, tests)`: `parser()` with `--json`, `--root`, `--main`, `--limit`, `--dry-run`, `--quiet`; `run(main)` maps a `Verdict` or an exception to the exit convention and prints JSON on `--json`
* the prologue line check; `--selftest` forwarding shim during migration

## Absorbs (today's implementations)

97 `ArgumentParser` sites, `selftest.SELFTEST_FLAG` discovery

## Test contract

Tier: fixture (a lib test never reads the live tree). a tool built on it parses every flag in the compatibility list

## Known gaps

None until implemented; `migration.md` names the package.
