#!/usr/bin/env python3
"""Fixture tests for tools/agents/sync_playbook_index.py.

The tool exists so the index in the skill cannot drift from `docs/matching.md` a second time, so the tests are
about the drift classes that actually happened plus the ones that must refuse:

* a section number appearing twice (the duplicate 48 that made "playbook 48" ambiguous),
* a section with no problem paragraph in either house style,
* the file's physical order not being numeric order (47, 51, 73, 52... in today's plan),
* a target with no marker pair, or with the markers reversed or duplicated,
* and the real tree: today's plan must be clean and the skill index must be in sync with it.

    python tools/agents/sync_playbook_index_selftest.py
"""
import argparse
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import sync_playbook_index as spi  # noqa: E402

RESULTS = []


def check(name, got, want):
    ok = got == want
    RESULTS.append((name, ok, got, want))
    return ok


def fixture(plan, target_lines):
    """A throwaway repo: configure.py (for the root walk), docs/matching.md, the skill index."""
    root = tempfile.mkdtemp(prefix="spi-selftest-")
    os.makedirs(os.path.join(root, "docs"))
    open(os.path.join(root, "configure.py"), "w").close()
    open(os.path.join(root, "docs", "matching.md"), "w", encoding="utf-8", newline="").write(plan)
    os.makedirs(os.path.dirname(os.path.join(root, spi.TARGET_REL)))
    open(os.path.join(root, spi.TARGET_REL), "w", encoding="utf-8", newline="").write(
        (chr(13) + chr(10)).join(target_lines))
    return root


def args(root, **kw):
    a = argparse.Namespace(check=False, print_block=False, repo=root)
    for k, v in kw.items():
        setattr(a, k, v)
    return a


PLAN = (chr(13) + chr(10)).join([
    "## 3. The third idea",
    "",
    "**Problem.** The third problem, in one line.",
    "",
    "**Why it happens.** Because.",
    "",
    "## 1. The first idea",
    "",
    "Problem: the first problem, in the older house style, and it | holds a pipe.",
    "",
    "## 2. The second idea",
    "",
    "**Problem.** The second problem.",
    "",
])

TARGET = [
    "# CLAUDE.md",
    "",
    spi.BEGIN,
    "stale contents",
    spi.END,
    "",
    "## After the block",
]

# ---- parsing ---------------------------------------------------------------------------------------------
sections, order = spi.parse_sections(PLAN)
check("parse: order is the file's order", order, [3, 1, 2])
check("parse: titles", [sections[n]["title"] for n in (1, 2, 3)],
      ["The first idea", "The second idea", "The third idea"])
check("parse: `**Problem.**` paragraph", spi.problem_sentence(sections[2]["body"]), "The second problem.")
check("parse: older `Problem:` line", spi.problem_sentence(sections[1]["body"]),
      "the first problem, in the older house style, and it | holds a pipe.")
check("parse: clean fixture has no defects", spi.shape_defects(sections, order), [])

# ---- the generated block --------------------------------------------------------------------------------
block = spi.build_block(sections)
rows = [ln for ln in block.split(chr(10))
        if ln.startswith("| ") and not ln.startswith("| # ") and "---" not in ln]
check("block: rows are sorted by number, not file order", [r.split("|")[1].strip() for r in rows], ["1", "2", "3"])
check("block: a pipe in the problem sentence is escaped", "holds a pipe" in rows[0] and
      spi.BS + "|" in rows[0], True)
check("block: starts with the BEGIN marker", block.split(chr(10))[0], spi.BEGIN)
check("block: ends with the END marker", block.rstrip(chr(10)).split(chr(10))[-1], spi.END)

# ---- refusals -------------------------------------------------------------------------------------------
dupe, dupe_order = spi.parse_sections(PLAN + (chr(13) + chr(10)).join(["", "## 3. The third idea again", "",
                                                                      "**Problem.** A clash."]))
defects = spi.shape_defects(dupe, dupe_order)
check("refuse: a duplicate section number is named with its count",
      any("section 3 appears 2 times" in d for d in defects), True)

noprob, noprob_order = spi.parse_sections((chr(13) + chr(10)).join(["## 7. No problem here", "", "Just prose."]))
defects = spi.shape_defects(noprob, noprob_order)
check("refuse: a section with no problem sentence is named",
      any("section 7 has neither" in d for d in defects), True)

check("refuse: an empty plan is a shape change",
      any("shape changed" in d for d in spi.shape_defects({}, [])), True)

for name, lines, want in (
        ("no markers", ["# CLAUDE.md", "", "no block"], "no playbook-index marker pair"),
        ("markers reversed", ["# CLAUDE.md", spi.END, "x", spi.BEGIN], "comes before the BEGIN marker"),
        ("markers duplicated", [spi.BEGIN, spi.END, spi.BEGIN, spi.END], "appears more than once")):
    try:
        spi.splice((chr(13) + chr(10)).join(lines), block)
        got = "no refusal"
    except SystemExit as e:
        got = str(e)
    check("refuse: %s" % name, want in got, True)

# ---- cell() ---------------------------------------------------------------------------------------------
check("cell: collapses newlines", spi.cell("a" + chr(10) + "  b"), "a b")
check("cell: caps at a word boundary",
      spi.cell("word " * 80).endswith("..."), True)
check("cell: a cap never splits a word", "wor..." not in spi.cell("word " * 80), True)

# ---- end to end on a fixture ---------------------------------------------------------------------------
root = fixture(PLAN, TARGET)
check("e2e: write reports an update", spi.cmd_sync(args(root)), 0)
check("e2e: the block was replaced and the rest kept", "stale contents" in spi.read(os.path.join(root, spi.TARGET_REL)),
      False)
check("e2e: the surrounding file survived", "## After the block" in spi.read(os.path.join(root, spi.TARGET_REL)), True)
check("e2e: CRLF preserved", chr(13) + chr(10) in spi.read(os.path.join(root, spi.TARGET_REL)), True)
check("e2e: --check now passes", spi.cmd_sync(args(root, check=True)), 0)
txt = spi.read(os.path.join(root, spi.TARGET_REL)).replace("The second problem.", "tampered", 1)
open(os.path.join(root, spi.TARGET_REL), "w", encoding="utf-8", newline="").write(txt)
check("e2e: --check fails once the target is stale", spi.cmd_sync(args(root, check=True)), 1)

# ---- the real tree -------------------------------------------------------------------------------------
real = spi.find_root()
real_sections, real_order = spi.parse_sections(spi.read(os.path.join(real, spi.PLAN_REL)))
try:
    real_defects = spi.shape_defects(real_sections, real_order)
except Exception as e:  # a parse crash is itself a defect to report
    real_defects = [str(e)]
check("real: docs/matching.md parses with no defect (no duplicate number, every section has a problem)",
      real_defects, [])
check("real: section numbers are unique and contiguous from 1",
      sorted(real_sections) == list(range(1, len(real_sections) + 1)), True)
real_target = spi.read(os.path.join(real, spi.TARGET_REL))
try:
    got = spi.splice(real_target, spi.build_block(real_sections))
except SystemExit as e:  # no marker pair yet is a defect to report, not a crash
    got = str(e)
check("real: the skill's references/index.md is in sync with the plan", got, real_target)

failed = [r for r in RESULTS if not r[1]]
for name, ok, got, want in RESULTS:
    if not ok:
        print("FAIL  %s\n      got  %r\n      want %r" % (name, got, want))
print("%s - %d checks" % ("FAILED" if failed else "ok", len(RESULTS)))
sys.exit(1 if failed else 0)
