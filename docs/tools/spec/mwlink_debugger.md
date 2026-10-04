# `mwlink_debugger` - The CLI entry point of the `tools/mwlink/` package

## Purpose

`tools/mwlink_debugger.py` is the path the profiles, skills and docs spell (`python tools/mwlink_debugger.py
trace|verify|order|align --unit <X>`, the CLI compatibility list); it forwards to `tools/mwlink/cli.py`. Everything
else - the subcommands, the rules, the tests - is the package's spec, `docs/tools/spec/mwlink.md`.

## Users

The `decompiler` profile's linker section, the `mwcc-unit-matching` skill, `CLAUDE.md`, `docs/`.

## CLI

`mwlink_debugger.py info|messages|order|anchors|records|align|timeline|verify|trace|diagnose|phases` (see `mwlink.md`).

## Test contract

`tools/tests/mwlink/test_cli.py` and the smoke check `tools/tests/smoke/test_mwlink_live.py` (through the package).
