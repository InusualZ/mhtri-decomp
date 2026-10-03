# WP0 - the skeleton: what exists and how to use it

WP0 of `migration.md`: the package, the harness, the runner's new discovery and four lints. No tool changed behaviour.

## What exists

| path | what it is |
| --- | --- |
| `tools/lib/__init__.py` | the package; `PROLOGUE`, the one line every entry point carries |
| `tools/lib/testing.py` | `Checker`, `FixtureTree`, `GitFixture`, the tiers, the live-tree guard, `run(globals())` |
| `tools/tests/lib/test_testing.py` | the harness tests itself (fixture tier) |
| `tools/tests/lib/test_prologue.py` + `prologue-pending.json` | every entry point has exactly the prologue, no other `sys.path` change (smoke) |
| `tools/tests/lib/test_layering.py` + `layering-allow.json` | `tools/lib` imports no tool; a tool->tool import must be on the allow-list (smoke) |
| `tools/tests/lib/test_headers.py` | the header template (`design.md` section 9); **advisory**: prints a count, fails only its own rule fixtures (smoke) |
| `tools/tests/smoke/test_cli_compat.py` | every line of `migration.md`'s compatibility list answers `--help`/usage, offers its subcommands, documents its flags (smoke) |
| `tools/selftest.py` | discovers `tools/tests/**/test_*.py` beside the old shapes; `--tier fixture|smoke|all` |

The prologue (exact text in `tools/lib/__init__.py`): docstring first, then `from __future__ import annotations` if
used (it must precede every other statement), then the prologue line, then imports (`from tools.lib import ...`).

## Writing a fixture-tier test

```python
"""What this module tests (one line)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
from tools.lib import testing
from tools.units import sharedfiles          # the tool under test

TIER = "fixture"                              # the default; "smoke" may read the live tree


def test_ranges(c):                           # every test_* gets the module's Checker
    with testing.FixtureTree() as tree:       # under the system temp, never in the repository
        tree.add_unit("Dir/file.c", ranges={".text": (0x80004000, 0x80004100)})
        tree.add_symbol("do_thing", ".text", 0x80004000, 0x100)
        got = sharedfiles.parse_ranges(tree.read("config/RMHE08/splits.txt"))
        c.check("one range", got, [("Dir/file.c", ".text", 0x80004000, 0x80004100)])
        c.raises("bad section", ValueError, tree.claim, "Dir/file.c", ".bogus", 0, 4)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
```

`python tools/tests/<area>/test_<tool>.py` runs it alone; `tools/selftest.py` finds it. Output: `FAIL: <name>: <detail>`
per failure, then `ok - N checks` or `FAIL - M of N checks failed` (the runner's existing count parser reads both).
`GitFixture` (`init`, `commit({path: text|None}, msg)`, `branch`, `checkout`, `worktree`, `conflict(path, a, b)`, `run`,
`git`) uses its own identity and no global/system git config.

## Tier rules

* **fixture** (default): `run()` installs an audit hook that refuses any `open`, `os.listdir/scandir`, `os.chdir` or
  process start inside the live repository, except reading `tools/**/*.py` (imports). A refusal fails the run even when
  the test swallows the exception. `testing.live_root()` and `testing.assert_live_allowed(what)` raise; the tier rides
  into child processes as `TOOLS_TEST_TIER`, and the runner starts the module with its cwd in a fresh temp dir.
* **smoke**: may read the live tree through `testing.live_root()`; skip (`c.skip`) when an input is absent, never pin a
  count. Strict lints of the tree itself (prologue, layering, compat) are smoke because they read the tree.
* The guard cannot see `os.stat`/`os.path.exists`, nor reads at module import time (before `run()`).
* `--tier fixture|smoke` filters the `tools/tests/` modules only; the 89 older entries (`legacy`) and the `--check`
  entries run in every tier until they are re-homed. A re-homed test takes its tool's key and replaces the old entry.

## How the allow-lists shrink

`prologue-pending.json` (146 files) and `layering-allow.json` (258 edges) are snapshots of the tree at WP0 (after the
splits-program retirement, `61a9c80e7`). The tests fail on anything not listed (a new offender, a new edge) **and** on a listed entry that no longer applies
(a file that now conforms, an edge whose files exist but no longer import), so the package that fixes one removes it
in the same batch. `--prune` on either test drops stale and deleted entries; nothing adds to them. A deleted file is
only a note, so a retirement does not break the run.

## Measured

On main `61a9c80e7` + WP0, same tree, `--json`, two alternating rounds on a host shared with live lanes (`--jobs 8`):

| run | entries | checks | wall |
| --- | ---: | ---: | ---: |
| before (main's runner) | 83 (82 pass + 1 parked) | 5 977 | 73.9 / 72.7 s |
| after, `--tier all` | 88 = 83 legacy + 1 fixture + 4 smoke | 6 110 | 74.6 / 75.4 s |
| after, `--tier fixture` | 84 = 83 legacy + 1 fixture | 6 024 | 72.6 / 73.2 s |

* The runner's own selftest: 45 -> 73 checks. The five new modules take 3-6 s each and run in parallel; the wall is
  set by the longest legacy entries (`accessextent`, `land`, `slots`, `mwlink_debugger`, 31-45 s), so `--tier fixture`
  only becomes the fast run as those are re-homed.
* Each lint fails when its rule is broken (mutation runs in a scratch copy): a new tool with a stray
  `sys.path.insert`, a `tools/lib` module importing a tool, a new tool->tool import, a fixture-tier live read, an
  undocumented flag in the compat list, a four-line header on `tools/lib/testing.py`.
* Header advisory today: 1 of 104 tool modules follows the template.
