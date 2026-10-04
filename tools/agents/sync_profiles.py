#!/usr/bin/env python3
"""Inject docs/plan.md section 6.5's rule table into the subagent profiles between their BEGIN/END markers.
Spec: docs/tools/spec/sync_profiles.md. CLI: sync_profiles.py [--check | --print | --selftest]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import re
import sys
from tools.lib import text as libtext

# the block carries the plan's text verbatim (em dashes, `<=`, `>=`), and a Windows console is cp1252: without
# this `--print` dies on the first such character while the file write (UTF-8) is fine
try:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
except Exception:
    pass

HERE = os.path.dirname(os.path.abspath(__file__))

BEGIN = ("<!-- SECTION-6.5-RULES-BEGIN - generated from docs/plan.md section 6.5 by "
         "tools/agents/sync_profiles.py; do not edit by hand -->")
END = "<!-- SECTION-6.5-RULES-END -->"

PLAN_HEADING = "### 6.5 Type and naming discipline"
PLAN_REL = os.path.join("docs", "plan.md")
AGENTS_REL = os.path.join(".claude", "agents")

# A leading YAML frontmatter block opens with `---` on line 1 and closes on the next `---` line. Only a
# `name:` inside that block makes a file a profile; prose (`TESTS.md`) has no frontmatter and is skipped.
NAME_RE = re.compile(r"^name:\s*\S")

# The profiles whose section 6.5 text is generated (paths relative to the repository root). merger.md
# used to be excluded here ("it keeps its own hand-written prose"), and the 2026-09-28 profile probe
# measured the cost: a merger child could name rules 2/6/7/10 but had none of their text, while its job -
# hand-typing a merged header - is exactly where rules 3-5 are broken. It cites those rules by number, so
# leaving them hand-written is the drift this tool exists to prevent. All four writer/reviewer profiles
# are now generated.
PROFILES = (
    os.path.join(".claude", "agents", "decompiler.md"),
    os.path.join(".claude", "agents", "fixer.md"),
    os.path.join(".claude", "agents", "merger.md"),
    os.path.join(".claude", "agents", "codereviewer.md"),
    os.path.join(".claude", "agents", "surveyor.md"),
    os.path.join(".claude", "agents", "worker.md"),
)

# A rule table row: `| 1 | **title** | meaning |`. The meaning cell is one physical line in the plan.
RULE_RE = re.compile(r"^\|\s*(\d+)\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|\s*$")

# Sentences of section 6.5's enforcement paragraph are kept when they state *how* the rules are
# enforced - the tool, the gate, the grandfather, the absence of an escape. Everything else in that
# paragraph (the audit counts, the backlog policy) belongs to the plan, not to a profile.
ENFORCEMENT_ANCHORS = ("land.py verify", "--diff", "exempts nothing", "no exemption and no deferral")

MIN_RULES = 8   # a table that lost half its rows is a parse failure, not a rule change
# The block's title says `rules 1-N` with N = the table's row count, so a new rule (rule 12, 2026-09-28)
# reaches every profile without a code change here; there is no rule count to keep in step.


def repo_root(start):
    """The repository root, found by walking up for `configure.py` (the pattern sync_reference.py uses)."""
    d = os.path.abspath(start)
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise SystemExit("repo root (configure.py) not found above %s" % start)
        d = parent


def frontmatter_name(text):
    """The `name:` line from a leading YAML frontmatter block, or None when the file is not a profile.

    `TESTS.md` and any prose file have no frontmatter at all, so they are skipped - only a file that the
    runtime would actually load as an agent (a frontmatter `name:`) is a profile this tool must cover.
    """
    lines = to_lf(text).split("\n")
    if not lines or lines[0].rstrip() != "---":
        return None
    for line in lines[1:]:
        if line.rstrip() == "---":
            return None
        if NAME_RE.match(line):
            return line.split(":", 1)[1].strip()
    return None


def uncovered_profiles(root, profiles=None):
    """Profile files in `.claude/agents/` that carry a frontmatter `name:` but are not in `profiles`.

    This is the tripwire for the failure this tool exists to prevent, one level up: a profile added to the
    directory but not to `PROFILES` keeps whatever section 6.5 text it has, and `--check` still prints the
    happy line. A file without frontmatter (prose) is not a profile and is not reported. `profiles` is an
    injection point for the selftest; the real call uses the module's `PROFILES`.
    """
    listed = {os.path.normpath(p) for p in (profiles if profiles is not None else PROFILES)}
    d = os.path.join(root, AGENTS_REL)
    if not os.path.isdir(d):
        return []
    out = []
    for name in sorted(os.listdir(d)):
        path = os.path.join(d, name)
        rel = os.path.normpath(os.path.join(AGENTS_REL, name))
        if rel in listed or not os.path.isfile(path):
            continue
        try:
            text = read(path)
        except OSError:
            continue
        if frontmatter_name(text):
            out.append(rel)
    return out


def plan_section(root):
    """`docs/plan.md` section 6.5, verbatim.

    The same extraction `brief.py` performs for the worker briefs (`brief.plan_section`); duplicated here
    so this tool runs in a bare checkout with no `units/` import chain, and pinned equal to it by the
    selftest - one source, two readers.
    """
    path = os.path.join(root, PLAN_REL)
    if not os.path.exists(path):
        raise SystemExit("no %s under %s" % (PLAN_REL, root))
    text = open(path, encoding="utf-8", errors="replace").read()
    start = text.find(PLAN_HEADING)
    if start < 0:
        raise SystemExit("%s does not contain %r - the section was renamed; update PLAN_HEADING"
                         % (PLAN_REL, PLAN_HEADING))
    nxt = re.search(r"\n## ", text[start + len(PLAN_HEADING):])
    return text[start:start + len(PLAN_HEADING) + (nxt.start() if nxt else len(text))].strip()


def parse_rules(section):
    """[(number, title, meaning)] from the section's rule table, verbatim."""
    rows = []
    for line in section.split("\n"):
        m = RULE_RE.match(line)
        if m:
            rows.append((int(m.group(1)), m.group(2).strip(), m.group(3).strip()))
    if len(rows) < MIN_RULES:
        raise SystemExit("section 6.5's table parsed to %d rule row(s) - expected at least %d; the table "
                         "changed shape, so fix parse_rules rather than write a stale block"
                         % (len(rows), MIN_RULES))
    if [n for n, _t, _m in rows] != list(range(1, len(rows) + 1)):
        raise SystemExit("section 6.5's table is not numbered 1..%d: %s"
                         % (len(rows), [n for n, _t, _m in rows]))
    return rows


def enforcement(section):
    """The enforcement sentences: how the rules are enforced, taken from the section, not re-argued.

    The paragraph is taken whole (its contiguous, non-blank lines), then filtered sentence by sentence, so
    the audit table and the backlog policy that follow it in the plan can never leak into a profile.
    """
    lines = section.split("\n")
    start = next((i for i, line in enumerate(lines) if "**Enforcement is a tool" in line), None)
    if start is None:
        start = next((i for i, line in enumerate(lines) if "land.py verify" in line), None)
    if start is None:
        raise SystemExit("section 6.5 has no enforcement paragraph (no 'land.py verify' sentence); the "
                         "enforcement wording moved - update the selector rather than write a block without it")
    end = start
    while end < len(lines) and lines[end].strip():
        end += 1
    para = re.sub(r"\s+", " ", " ".join(lines[start:end]))
    sentences = re.split(r"(?<=[.!?])\s+(?=[A-Z*`(])", para)
    kept = [s.strip() for s in sentences if any(a in s for a in ENFORCEMENT_ANCHORS)]
    if not kept:
        raise SystemExit("section 6.5's enforcement paragraph matched none of %s - the wording changed; "
                         "update ENFORCEMENT_ANCHORS rather than write a block without enforcement"
                         % (ENFORCEMENT_ANCHORS,))
    return " ".join(kept)


def build_block(section):
    """The generated block: the table's rules, one line each, then the enforcement sentence(s)."""
    rows = parse_rules(section)
    lines = [BEGIN]
    lines.append("The canonical table for rules 1-%d is `docs/plan.md` section 6.5; this block is generated "
                 "from it - do not edit it by hand, run `tools/agents/sync_profiles.py`." % len(rows))
    lines.append("")
    for number, title, meaning in rows:
        lines.append("%d. %s - %s" % (number, title, meaning))
    lines.append("")
    lines.append(enforcement(section))
    lines.append(END)
    return "\n".join(lines)


def markers(text):
    """(begin_index, end_index) line numbers of the block's markers, matched on whole lines.

    Matched on whole lines on purpose: the marker text may also be quoted in prose, and a substring
    search would take that quotation for a marker.
    """
    lines = text.split("\n")
    begins = [i for i, line in enumerate(lines) if line.rstrip("\r") == BEGIN]
    ends = [i for i, line in enumerate(lines) if line.rstrip("\r") == END]
    if not begins and not ends:
        return None
    if len(begins) != 1 or len(ends) != 1:
        raise SystemExit("refusing: the profile does not contain exactly one of each section-6.5 marker line")
    if ends[0] < begins[0]:
        raise SystemExit("refusing: the END marker comes before the BEGIN marker")
    return begins[0], ends[0]


def inject(text, block):
    """`text` with the block between its markers replaced. Everything outside the markers is untouched."""
    found = markers(text)
    if not found:
        raise SystemExit("refusing: no section-6.5 marker pair - insert `%s` and `%s` around the rule list "
                         "first" % (BEGIN, END))
    begin, end = found
    lines = text.split("\n")
    return "\n".join(lines[:begin] + block.split("\n") + lines[end + 1:])


def read(path):
    """The file's text, newlines and all (no universal-newline translation)."""
    with open(path, "r", encoding="utf-8", newline="") as fh:
        return fh.read()


def nl_of(text):
    """The file's dominant line ending, so a rewrite keeps the file's own convention."""
    crlf = text.count("\r\n")
    return "\r\n" if crlf > text.count("\n") - crlf else "\n"


def to_lf(text):
    return text.replace("\r\n", "\n").replace("\r", "\n")


def write(path, lf_text, nl):
    """Write `lf_text` back with the file's dominant ending - never a half-CRLF file."""
    libtext.atomic_write(path, libtext.with_ending(lf_text, nl))


def check_profile(root, rel, block):
    """None when in sync, else a one-line reason (the profile is stale)."""
    path = os.path.join(root, rel)
    if not os.path.exists(path):
        return "missing profile %s" % rel
    text = to_lf(read(path))
    if not markers(text):
        return "%s has no section-6.5 marker pair - run sync_profiles.py to be told where they go" % rel
    want = inject(text, block)
    if want == text:
        return None
    got = text.split("\n")
    exp = want.split("\n")
    for i, (a, b) in enumerate(zip(got, exp)):
        if a != b:
            return "%s: line %d differs from docs/plan.md section 6.5" % (rel, i + 1)
    return "%s: the block is stale (%d vs %d lines)" % (rel, len(got), len(exp))


def cmd_sync(a):
    root = repo_root(a.repo or HERE)
    # fail closed on the profile SET before doing anything else: an unlisted profile is never checked, so
    # reporting "all profiles in sync" while one is unlisted is the drift this tool is supposed to stop
    unlisted = uncovered_profiles(root)
    if unlisted:
        for rel in unlisted:
            print("error: %s has frontmatter `name:` but is not in PROFILES - add it to "
                  "tools/agents/sync_profiles.py, or its section 6.5 block is never generated or checked" % rel,
                  file=sys.stderr)
        return 1
    block = build_block(plan_section(root))
    if a.print_block:
        print(block)
        return 0
    status = 0
    for rel in PROFILES:
        path = os.path.join(root, rel)
        raw = read(path)
        nl = nl_of(raw)
        new = inject(to_lf(raw), block)
        out = new.replace("\n", nl) if nl != "\n" else new
        if out == raw:
            print("%s: in sync" % rel)
            continue
        if a.check:
            print("stale: %s" % rel, file=sys.stderr)
            status = 1
        else:
            write(path, new, nl)
            print("updated %s" % rel)
    if a.check and status:
        print("run: python tools/agents/sync_profiles.py", file=sys.stderr)
    elif a.check:
        print("all profiles in sync with docs/plan.md section 6.5 (rules 1-%d)"
              % len(parse_rules(plan_section(root))))
    return status


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true", help="do not write; exit 1 when a profile is stale")
    ap.add_argument("--print", dest="print_block", action="store_true", help="print the generated block")
    ap.add_argument("--repo", help="repository root (default: walk up for configure.py)")
    ap.add_argument("--selftest", action="store_true", help="run tools/agents/sync_profiles_selftest.py")
    a = ap.parse_args()
    if a.selftest:
        import subprocess
        return subprocess.call([sys.executable, os.path.join(HERE, "sync_profiles_selftest.py")])
    return cmd_sync(a)


if __name__ == "__main__":
    sys.exit(main())