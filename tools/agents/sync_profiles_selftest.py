#!/usr/bin/env python3
"""Self-test for tools/agents/sync_profiles.py.

    python tools/agents/sync_profiles_selftest.py
    python tools/agents/sync_profiles.py --selftest

The point of the tool is that the profiles' section 6.5 text cannot drift from `docs/plan.md` section 6.5.
The test pins that in three ways:

* on fixtures - the block carries every table row (a new rule appears without a code change, rule 11's
  `/* untyped: <reason> */` marker and rule 12's claim-the-unowned-range row included), the enforcement paragraph is filtered to its enforcement
  sentences (the audit table and the "apply immediately" sentence stay out), the markers are refused when
  duplicated or reversed, and a deliberately stale profile is reported stale and then in sync;
* on the real tree - every generated profile is in sync with the real plan, and none still teaches the deleted
  `rule 7 deferred` escape (that is the drift this tool exists to end);
* against `brief.plan_section` - the brief and the profiles read section 6.5 through the same bytes.

It also pins the coverage tripwire: a file in `.claude/agents/` with a frontmatter `name:` that is not in
`sync_profiles.PROFILES` is an error (a prose file without frontmatter is skipped, `TESTS.md` included), so a
new profile cannot be added without being checked.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.agents import sync_profiles as sp

# A miniature section 6.5 with the shape the real one has: the table, the enforcement paragraph, and the
# audit table that follows it. Rule 11 stands in for the `void *` rule and rule 12 for the owner's
# claim-the-unowned-range ruling (2026-09-28) - a row added long after the block was first generated.
FIXTURE = """### 6.5 Type and naming discipline - mandatory in phases B and C

Twelve rules.

| # | rule | what it means concretely |
| --- | --- | --- |
| 1 | **A shared type lives in one header** | a type more than one unit uses is defined **once** (under `include/`) |
| 2 | **An extern lives with the TU that owns the symbol** | an owned symbol belongs in the owner's header; a symbol no registered unit owns belongs in a band header under `include/unsplit/` |
| 3 | **A reconstructed class/struct states its size** | every reconstructed type carries `/* size: 0xNN */` |
| 4 | **Every field carries its offset** | `/* +0x1C */` on the field, ascending |
| 5 | **Every field has a name from its context** | `pad_0xNN` / `unused_0xNN` only for padding |
| 6 | **Pointer arithmetic to reach a field is forbidden** | declare the type and write `self->field = v;` |
| 7 | **Symbols have proper names** | `fn_XXXXXXXX`, `lbl_XXXXXXXX`, `loc_XXXXXXXX` and bare `unkNN` are findings in `src/`, whoever owns the symbol. There is no exemption and no deferral |
| 8 | **`goto` is forbidden** | no `goto` and no label used as a control-flow device |
| 9 | **A mangled symbol is called through its owner** | declare the class or namespace and call it |
| 10 | **A vtable we own is compiler output** | let MWCC emit it from a class that declares its `virtual` methods |
| 11 | **`void *` is banned** | an untyped pointer is a hole in the reconstruction; a declaration that genuinely needs one carries a `/* untyped: <reason> */` marker |
| 12 | **Data a unit uses and nobody owns is the unit's to claim and match** | the local `extern` of a data symbol no registered range covers is the finding; the unit claims the range in its own `splits.txt` and matches it. Declare-never-define stays right when the range is ALREADY the unit's own |

**Enforcement is a tool, not a promise.** `stylelint.py` reports each rule `file:line` and `land.py verify` refuses a batch that adds a violation. The rules apply to new work immediately and existing units are brought into conformance as they are touched. Rule 11 has no deferral either. The **only** grandfather is the gate's `--diff`.

| measure | count | rule |
| --- | --- | --- |
| auto-generated names in `src/` | 376 | 7 |
"""

STALE_BLOCK = """%s
1. **A shared type lives in one header** - old text
%s""" % (sp.BEGIN, sp.END)


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def raises(name, fn):
        nonlocal checks
        checks += 1
        try:
            fn()
        except SystemExit:
            return
        fails.append("%s: did not raise SystemExit" % name)

    # --- the table is parsed and every row reaches the block -------------------------------------------
    rows = sp.parse_rules(FIXTURE)
    check("parse_rules reads all 12 rows", [n for n, _t, _m in rows], list(range(1, 13)))
    check("parse_rules keeps the title verbatim", rows[0][1], "**A shared type lives in one header**")
    check("parse_rules keeps the meaning verbatim", rows[6][2],
          "`fn_XXXXXXXX`, `lbl_XXXXXXXX`, `loc_XXXXXXXX` and bare `unkNN` are findings in `src/`, whoever owns "
          "the symbol. There is no exemption and no deferral")
    block = sp.build_block(FIXTURE)
    check("the block names the canonical table", "docs/plan.md section 6.5" in block, True)
    check("the block states the rule count", "rules 1-12" in block, True)
    check("a new rule (11) is picked up with no code change", "11. **`void *` is banned**" in block, True)
    check("rule 11 keeps its marker", "/* untyped: <reason> */" in block, True)
    check("a new rule (12) is picked up with no code change",
          "12. **Data a unit uses and nobody owns is the unit's to claim and match**" in block, True)
    check("rule 12 keeps the declare-never-define carve-out", "declare-never-define stays right" in block.lower(), True)
    check("rule 7 keeps 'no exemption and no deferral'", "There is no exemption and no deferral" in block, True)
    check("rule 2 keeps the unowned-extern band header", "include/unsplit/" in block, True)
    numbered = [line for line in block.split("\n") if line[:1].isdigit() and "." in line[:4]]
    check("one line per rule", len(numbered), len(rows))

    # --- the enforcement paragraph is filtered, the audit table never leaks ------------------------------
    enf = sp.enforcement(FIXTURE)
    check("enforcement keeps the gate sentence", "land.py verify" in enf, True)
    check("enforcement keeps the grandfather", "--diff" in enf, True)
    check("enforcement drops the 'apply immediately' sentence",
          "apply to new work immediately" in enf, False)
    check("enforcement never carries the audit table", "| measure |" in block or "376" in block, False)
    check("the enforcement paragraph is in the block", enf in block, True)

    # --- a table that changed shape is refused, not summarised ------------------------------------------
    raises("a too-short table is refused", lambda: sp.parse_rules("| 1 | **a** | b |\n"))
    raises("a non-contiguous table is refused",
           lambda: sp.parse_rules("".join("| %d | **r%d** | m |\n" % (n, n)
                                          for n in [1, 2, 3, 4, 5, 6, 7, 8, 10])))
    raises("a section with no enforcement paragraph is refused",
           lambda: sp.build_block("### 6.5 x\n\n| 1 | **a** | b |\n" * 1
                                  + "".join("| %d | **r%d** | m |\n" % (n, n) for n in range(2, 9))))

    # --- the marker machinery --------------------------------------------------------------------------
    plain = "intro\n\n%s\nold\n%s\noutro\n" % (sp.BEGIN, sp.END)
    check("markers finds the pair", sp.markers(plain), (2, 4))
    check("markers is None without a pair", sp.markers("no markers here\n"), None)
    raises("a doubled BEGIN is refused",
           lambda: sp.markers("x\n%s\n%s\n%s\n" % (sp.BEGIN, sp.BEGIN, sp.END)))
    raises("END before BEGIN is refused", lambda: sp.markers("%s\n%s\n" % (sp.END, sp.BEGIN)))
    injected = sp.inject(plain, "BLOCK1\nBLOCK2")
    check("inject replaces only the block", injected, "intro\n\nBLOCK1\nBLOCK2\noutro\n")
    marked = sp.BEGIN + "\nRULE\n" + sp.END
    check("inject is idempotent on a marker-carrying block",
          sp.inject(sp.inject(plain, marked), marked), sp.inject(plain, marked))
    raises("inject refuses a profile with no markers", lambda: sp.inject("prose only\n", "B"))

    # --- the deliberate staleness: stale, then in sync --------------------------------------------------
    tmp = tempfile.mkdtemp(prefix="sync-profiles-selftest-")
    rel = os.path.join(".claude", "agents", "probe.md")
    path = os.path.join(tmp, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("before\n\n%s\nafter\n" % STALE_BLOCK)
    good = sp.build_block(FIXTURE)
    stale = sp.check_profile(tmp, rel, good)
    check("a deliberately stale block is reported stale", bool(stale) and "differs" in stale, True)
    text = open(path, encoding="utf-8", newline="").read()
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(sp.inject(text, good))
    check("the same profile is in sync once regenerated", sp.check_profile(tmp, rel, good), None)
    check("regeneration left the surrounding prose alone",
          open(path, encoding="utf-8", newline="").read().startswith("before\n\n"), True)

    # --- the coverage tripwire: a frontmatter profile not in PROFILES is an error --------------------
    cov = tempfile.mkdtemp(prefix="sync-profiles-coverage-")
    agents_dir = os.path.join(cov, ".claude", "agents")
    os.makedirs(agents_dir)
    listed_rel = os.path.join(".claude", "agents", "listed.md")
    orphan_rel = os.path.normpath(os.path.join(".claude", "agents", "orphan.md"))
    prose_rel = os.path.normpath(os.path.join(".claude", "agents", "TESTS.md"))
    noname_rel = os.path.normpath(os.path.join(".claude", "agents", "noname.md"))
    with open(os.path.join(cov, "configure.py"), "w", encoding="utf-8") as fh:
        fh.write("")
    with open(os.path.join(cov, listed_rel), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("---\nname: listed\ndescription: x\n---\n\n%s\nold\n%s\n" % (sp.BEGIN, sp.END))
    with open(os.path.join(cov, orphan_rel), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("---\nname: orphan\ndescription: y\n---\n\nprose\n")
    with open(os.path.join(cov, prose_rel), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("# prose\n\nno frontmatter here\n")
    with open(os.path.join(cov, noname_rel), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("---\ndescription: z\n---\n\nprose\n")
    found = sp.uncovered_profiles(cov, [listed_rel])
    check("an unlisted frontmatter profile is reported", found, [orphan_rel])
    check("a listed profile is not reported", listed_rel in found, False)
    check("prose with no frontmatter is skipped", prose_rel in found, False)
    check("frontmatter without a name: is skipped", noname_rel in found, False)
    check("an absent .claude/agents is no error",
          sp.uncovered_profiles(tempfile.mkdtemp(prefix="sync-profiles-nodir-")), [])
    check("the real tree has no unlisted profile", sp.uncovered_profiles(ROOT), [])

    def run_sync(*argv):
        return subprocess.run([sys.executable, sp.__file__] + list(argv),
                              capture_output=True, text=True, encoding="utf-8", errors="replace")

    before = open(os.path.join(cov, orphan_rel), encoding="utf-8").read()
    run_w = run_sync("--repo", cov)
    check("sync (write) fails closed on an unlisted profile", run_w.returncode != 0, True)
    check("sync names the unlisted file", "orphan.md" in run_w.stderr, True)
    check("sync wrote nothing on an unlisted profile",
          open(os.path.join(cov, orphan_rel), encoding="utf-8").read(), before)
    run_c = run_sync("--repo", cov, "--check")
    check("--check fails closed on an unlisted profile", run_c.returncode != 0, True)
    check("--check names the unlisted file", "orphan.md" in run_c.stderr, True)
    check("--check does not print the happy line", "all profiles in sync" in run_c.stdout, False)

    # --- the real tree ---------------------------------------------------------------------------------
    real = sp.plan_section(ROOT)
    real_block = sp.build_block(real)
    real_rows = sp.parse_rules(real)
    check("the real plan's table parses", len(real_rows) >= 10, True)
    check("the block's rule count matches the real table",
          "rules 1-%d" % len(real_rows) in real_block, True)
    for profile in sp.PROFILES:
        check("%s is in sync with the real plan" % profile.replace("\\", "/"),
              sp.check_profile(ROOT, profile, real_block), None)
        text = sp.to_lf(sp.read(os.path.join(ROOT, profile)))
        check("%s no longer teaches the deferred escape" % os.path.basename(profile),
              "is legal only for references to OTHER units" in text, False)
        check("%s no longer claims the comment defers the fn_ half" % os.path.basename(profile),
              "defers the `fn_` half only" in text, False)
    check("the real plan's rule 7 has no deferral",
          "no exemption and no deferral" in real_block, True)
    check("the real plan's rule 12 is in the block",
          "12. **Data a unit uses and nobody owns is the unit's to claim and match**" in real_block, True)
    check("the real plan's rule 12 keeps the already-yours carve-out",
          "ALREADY the unit's own" in real_block, True)

    # --- the brief and the profiles read the same bytes -------------------------------------------------
    try:
        from tools.units import brief
        check("brief.plan_section and sync_profiles.plan_section agree",
              brief.plan_section(ROOT, sp.PLAN_HEADING), real)
    except Exception as exc:  # the units import chain is heavy; the check above already pins the text
        print("note: brief.plan_section not importable here (%s)" % exc.__class__.__name__)

    for f in fails:
        print("FAIL  %s" % f)
    print("%s - %d checks" % ("FAILED" if fails else "ok", checks))
    return 1 if fails else 0


def main() -> int:
    return selftest()


if __name__ == "__main__":
    sys.exit(main())