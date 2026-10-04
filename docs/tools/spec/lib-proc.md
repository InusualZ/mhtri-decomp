# `lib/proc` - Subprocess with the codec rule: every text run decodes UTF-8 with replacement; spawn retry on Windows; process-tree kill

## Purpose

Runs a process the one way the tools may: text decoded as UTF-8 with replacement, never the host's locale codec. Retries the
transient Windows launch refusal and kills a whole process tree.

## Users

`lib.git`, the tools that start processes (each installs the retry explicitly: claims, slots, verifyunit, callees, unitinfo,
the ideas demo compile), `land.run`, `tools/selftest_site/sitecustomize.py` (installs the retry in every process the runner
starts); the live trap scan is `tools/tests/smoke/test_proc_traps.py` (WP6 deleted the shims `units/subproc.py`, `spawnretry.py`).

## Public API

* `run(args, cwd=None, check=False, input=None, timeout=None, **kw) -> CompletedProcess` (text, `encoding="utf-8"`,
  `errors="replace"`, `capture_output` unless the caller routes `stdout`/`stderr`); `run_bytes(...)` (bytes).
* `TEXT_KWARGS`, `TEXT_KEYWORDS`.
* `kill_tree(proc)`: `taskkill /F /T` on Windows, the process group elsewhere.
* `install_spawn_retry() -> bool` (no-op off Windows, idempotent), `retrying(init, sleep, attempts, backoff)`,
  `is_transient(exc)`, `SPAWN_ATTEMPTS = 6`, `SPAWN_BACKOFF_S = 0.1`.
* `trap_sites(root, subdir="tools", skip_selftests=False)`, `module_traps(tree)`, `call_traps(node)`: the AST scan for a
  text-mode call without `encoding=`.

## Invariants and rules

* **The codec is said, never inherited (F34).** `text=True` without `encoding=` decodes with the locale codec (`cp1252`
  here); `git show HEAD:CLAUDE.md` then spelled an em dash as three characters, the clean-tree comparison never matched,
  and the gate refused every landing. One non-ASCII byte was the whole difference, and a pure-ASCII fixture passes either way.
* `errors="replace"`: a stray byte in a tool's output never aborts a gate halfway.
* The trap scan requires the literal `text=True` (a `text=` field of a dataclass is not a subprocess), does not claim a
  `**` splat or a `kwargs=` dict, and skips the vendored `tools/m2c/` and `tools/mwcc-debugger/`.
* **The spawn retry** (2026-09-30): under parallel load `CreateProcess` raised `PermissionError [WinError 5]` for an
  executable that was fine a moment later. Only `winerror == 5` is retried (six attempts, 0.1 s growing backoff, ~1.5 s);
  anything else is raised at once.

## Absorbs (today's implementations)

`subproc.run/trap_sites/module_traps/call_traps`, `spawnretry.install/retrying/is_transient`, `land.run`, and the process
call of the 16 git wrappers (through `lib.git`). `selftest._kill_tree` is the same function and moves when the runner is
rewritten (WP0's runner keeps its copy until then).

## Lib dependencies

None (stdlib).

## Test contract

Tier: fixture (`tools/tests/lib/test_proc.py`). A non-ASCII byte decodes as UTF-8 and a non-UTF-8 byte never raises; a
WinError 5 is retried then raised after `SPAWN_ATTEMPTS`, any other error at once; `kill_tree` ends a process; the trap
scan finds the trap, not the fixed spelling, skips vendored code and reports a broken file. The live tree's trap count is
`land.py`'s own selftest row (`trap_sites(SELF_REPO)` is empty).

## Known gaps

* 176 `text=True, encoding="utf-8", errors="replace"` call sites still spell the rule themselves; they move to `run()` as
  each tool's package thins it.
* `selftest._kill_tree` stays in the runner until WP6.
