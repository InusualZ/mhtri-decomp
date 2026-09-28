#!/usr/bin/env python3
"""sync_playbook_index.py - generate AGENTS.md's matching-playbook index from docs/matching.md.

`docs/matching.md` is the playbook: one section per idea, headed `## N. Title`. `AGENTS.md` carries the index
a worker actually reads. That index was hand-maintained, and it drifted the way a hand copy does - rows out of
numeric order, **two sections numbered 48** (so "playbook 48" was genuinely ambiguous in the notes that cite
it: one note means the `extern "C"` row, another the float-varargs row), and it stopped at 70 while the
playbook kept growing. This tool derives it instead - the number, the section title and the section's own
problem sentence - between a marker pair in AGENTS.md, sorted by number.

Two properties are the point. The index cannot describe an idea the playbook does not hold, and a **duplicate
section number is refused loudly** instead of emitted twice: that refusal is the tripwire which would have
caught the duplicate 48 the day it was written.

    python tools/agents/sync_playbook_index.py             # write the block into AGENTS.md
    python tools/agents/sync_playbook_index.py --check     # exit 1 when the block is stale; write nothing
    python tools/agents/sync_playbook_index.py --print     # print the block
    python tools/agents/sync_playbook_index.py --selftest  # fixtures: order, duplicates, missing problem
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

BEGIN = ("<!-- PLAYBOOK-INDEX-BEGIN - generated from docs/matching.md by "
         "tools/agents/sync_playbook_index.py; do not edit by hand -->")
END = "<!-- PLAYBOOK-INDEX-END -->"

PLAN_REL = os.path.join("docs", "matching.md")
TARGET_REL = "AGENTS.md"

# `## 12. Title` (and `## 12. Title - subtitle`); a section with no number is not indexed.
SECTION_RE = re.compile(r"^##\s+(\d+)\.\s+(.*?)\s*$")
# The problem sentence in both house styles: the current `**Problem.**` and the older `Problem:`.
PROBLEM_RES = (re.compile(r"^\*\*Problem\.\*\*\s*(.*)$"), re.compile(r"^Problem:\s*(.*)$"))

CELL_CAP = 220
BS = chr(92)  # a literal backslash, built rather than written so no escape can be mangled in transit

LEAD = [
    "Every **numbered** idea in the playbook is listed below - a short unnumbered section (the paired-single",
    "note, the RSO worked example) has no row of its own. The number **is** the section in",
    "`docs/matching.md`, the idea column is that section's title, and the problem column is the opening of",
    "that section's own problem sentence, truncated at %d characters - so this index cannot describe an idea"
    " the playbook does not have, and a duplicate section number is refused rather than listed twice."
    % CELL_CAP,
]


def read(path):
    """Read a repository text file newline-preserving (AGENTS.md and the plan are CRLF)."""
    with open(path, "r", encoding="utf-8", newline="") as f:
        return f.read()


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def find_root(start=None):
    """Walk up from `start` (default: this file) to the directory holding configure.py."""
    cur = os.path.abspath(start or HERE)
    while True:
        if os.path.isfile(os.path.join(cur, "configure.py")):
            return cur
        parent = os.path.dirname(cur)
        if parent == cur:
            raise SystemExit("refusing: no configure.py above %s" % (start or HERE))
        cur = parent


def parse_sections(text):
    """({number: {'title', 'body'}}, [numbers in file order]).

    The order list is kept because the file's physical order is not numeric in places, and because a repeated
    number must be *detectable* - a dict keyed by number would silently collapse it.
    """
    sections, order, cur = {}, [], None
    for raw in text.split(chr(10)):
        line = raw.rstrip(chr(13))
        m = SECTION_RE.match(line)
        if m:
            cur = int(m.group(1))
            order.append(cur)
            sections.setdefault(cur, {"title": m.group(2), "body": []})
            continue
        if cur is not None:
            sections[cur]["body"].append(line)
    return sections, order


def problem_sentence(body):
    """The section's problem paragraph, first line onwards until a blank line, or None when it has none."""
    for i, line in enumerate(body):
        for rx in PROBLEM_RES:
            m = rx.match(line)
            if not m:
                continue
            paras = [m.group(1).strip()]
            for cont in body[i + 1:]:
                if not cont.strip():
                    break
                paras.append(cont.strip())
            return " ".join(paras)
    return None


def cell(text, cap=CELL_CAP, sentence=False):
    """One markdown table cell: single-line, `|` escaped, capped at a word boundary.

    `sentence=True` capitalises the first character, so a problem column reads as sentences rather than as a
    block of lower-case clauses (a backtick or a quote at position 0 is left alone).
    """
    t = " ".join(text.split()).replace("|", BS + "|")
    if sentence and t and (t[0].isalpha() and t[0].islower()):
        t = t[0].upper() + t[1:]
    if cap and len(t) > cap:
        t = t[:cap].rsplit(" ", 1)[0] + "..."
    return t


def shape_defects(sections, order):
    """Everything that must refuse rather than produce a table. Returns a list of messages."""
    out = []
    dupes = sorted({n for n in order if order.count(n) > 1})
    for n in dupes:
        out.append("section %d appears %d times (%s)"
                   % (n, order.count(n), sections[n]["title"]))
    for n in sorted(sections):
        if not problem_sentence(sections[n]["body"]):
            out.append("section %d has neither a `**Problem.**` paragraph nor an older `Problem:` line (%s)"
                       % (n, sections[n]["title"]))
    if not sections:
        out.append("no numbered `## N. Title` section was parsed - the plan's shape changed")
    return out


def build_block(sections):
    lines = list(LEAD) + ["", "| # | idea | problem it solves |", "| --- | --- | --- |"]
    for n in sorted(sections):
        lines.append("| %d | %s | %s |"
                     % (n, cell(sections[n]["title"], cap=None), cell(problem_sentence(sections[n]["body"]), sentence=True)))
    lines.append(END)
    return BEGIN + chr(10) + chr(10).join(lines) + chr(10)


def splice(target_text, block):
    """Replace the block between the markers (inclusive), or refuse."""
    nl = chr(13) + chr(10) if chr(13) + chr(10) in target_text else chr(10)
    lines = target_text.split(nl)
    begins = [i for i, ln in enumerate(lines) if ln.rstrip(chr(13)) == BEGIN]
    ends = [i for i, ln in enumerate(lines) if ln.rstrip(chr(13)) == END]
    if not begins or not ends:
        raise SystemExit("refusing: no playbook-index marker pair in AGENTS.md - insert `%s` and `%s` around "
                         "the table first" % (BEGIN, END))
    if len(begins) > 1 or len(ends) > 1:
        raise SystemExit("refusing: the marker appears more than once (begin=%d, end=%d)" % (len(begins), len(ends)))
    if ends[0] < begins[0]:
        raise SystemExit("refusing: the END marker comes before the BEGIN marker")
    body = block.rstrip(chr(10)).split(chr(10))
    out = lines[:begins[0]] + body + lines[ends[0] + 1:]
    return nl.join(out)


def cmd_sync(a):
    root = find_root(a.repo)
    sections, order = parse_sections(read(os.path.join(root, PLAN_REL)))
    defects = shape_defects(sections, order)
    if defects:
        for d in defects:
            print("refusing: %s" % d, file=sys.stderr)
        return 1
    block = build_block(sections)
    target_path = os.path.join(root, TARGET_REL)
    new_text = splice(read(target_path), block)
    if a.print_block:
        sys.stdout.write(block)
        return 0
    if a.check:
        old = read(target_path)
        if old != new_text:
            print("stale: %s's playbook index differs from %s (%d ideas) - run "
                  "python tools/agents/sync_playbook_index.py" % (TARGET_REL, PLAN_REL, len(sections)),
                  file=sys.stderr)
            return 1
        print("%s's playbook index is in sync with %s (%d ideas, numbers %d-%d)"
              % (TARGET_REL, PLAN_REL, len(sections), min(sections), max(sections)))
        return 0
    if read(target_path) == new_text:
        print("%s: in sync" % TARGET_REL)
        return 0
    write(target_path, new_text)
    print("updated %s (%d ideas, numbers %d-%d)"
          % (TARGET_REL, len(sections), min(sections), max(sections)))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true", help="do not write; exit 1 when the index is stale")
    ap.add_argument("--print", dest="print_block", action="store_true", help="print the generated block")
    ap.add_argument("--repo", help="repository root (default: walk up for configure.py)")
    ap.add_argument("--selftest", action="store_true", help="run tools/agents/sync_playbook_index_selftest.py")
    a = ap.parse_args()
    if a.selftest:
        import subprocess
        return subprocess.call([sys.executable, os.path.join(HERE, "sync_playbook_index_selftest.py")])
    return cmd_sync(a)


if __name__ == "__main__":
    sys.exit(main())
