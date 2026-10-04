# `checklf` - A shim over `edit.py check --blob` (retired; deleted in WP6)

## Purpose

Keep `tools/units/checklf.py [PATH...] [--rev REV] [--repo DIR]` answering, with its exit codes, while the references are
swept: every name it had (`check_path`, `blob_of`, `endings`, `status_paths`) is `tools/agents/edit.py`'s one
implementation, and the check itself is `edit.py check --blob` (`spec/edit.md`).

## CLI

```
python tools/units/checklf.py [PATH...] [--rev REV] [--repo DIR]
python tools/units/checklf.py --selftest
```
Exit codes: 0 every path matches its blob's line-ending style, 1 at least one differs, 2 usage.

## Lib dependencies

None directly; `tools/agents/edit.py` (the layering allow-list carries `checklf.py -> agents/edit.py` in place of the
deleted `checklf.py -> checklf_selftest.py`).

## Test contract

Tier: fixture. `tools/tests/units/test_checklf.py` on `GitFixture` (the old `checklf_selftest.py` re-homed, plus a
`--rev HEAD` case: checklf asked git for `HEADpath`, so `--rev HEAD` never compared a blob).

## Known gaps

Retired: `docs/tools/retired.md`; the file goes with WP6's sweep.
