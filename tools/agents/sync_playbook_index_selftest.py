#!/usr/bin/env python3
"""Fixture tests for tools/agents/sync_playbook_index.py.

The index is generated from the front matter of docs/matching/NNN-slug.md, so the tests are about the classes
that must refuse (a duplicate id, a missing or malformed key, an unknown tag or status, a file-name/front-matter
mismatch, a demo naming a missing file), about ordering and escaping in the generated table, about `--where`
and `--json`, and about the real tree: it must be clean and index.md must be in sync.

    python tools/agents/sync_playbook_index_selftest.py
"""
import argparse
import io
import contextlib
import json
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


def idea_text(n, title="An idea", status="works", problem="A problem.", tags="[flags]", applies="[]", demo="",
              extra=None, drop=None):
    kv = [("id", str(n)), ("title", title), ("status", status), ("problem", problem), ("tags", tags),
          ("applies", applies), ("demo", demo)]
    lines = ["---"]
    for k, v in kv:
        if drop == k:
            continue
        lines.append("%s: %s" % (k, v) if v else "%s:" % k)
    if extra:
        lines.append(extra)
    lines += ["---", "", "# %d. %s" % (n, title), "", "**Problem.** %s" % problem, ""]
    return "\n".join(lines)


def fixture(files):
    """A throwaway repo: configure.py (for the root walk) and docs/matching/<name>: <text>."""
    root = tempfile.mkdtemp(prefix="spi-selftest-")
    os.makedirs(os.path.join(root, "docs", "matching"))
    open(os.path.join(root, "configure.py"), "w").close()
    for name, text in files.items():
        with open(os.path.join(root, "docs", "matching", name), "w", encoding="utf-8", newline="") as f:
            f.write(text)
    return root


def args(root, **kw):
    a = argparse.Namespace(check=False, print_block=False, repo=root, where=None, json=False)
    for k, v in kw.items():
        setattr(a, k, v)
    return a


def run(root, **kw):
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        rc = spi.cmd_sync(args(root, **kw))
    return rc, out.getvalue(), err.getvalue()


def defects_of(files):
    return spi.load_ideas(fixture(files))[1]


GOOD = {
    "003-third-idea.md": idea_text(3, "The third idea", problem="The third | problem, with a pipe.", tags="[pragma, data]"),
    "001-first-idea.md": idea_text(1, "The first idea", problem="the first problem: lower-case start."),
    "002-second-idea.md": idea_text(2, "The second idea", tags="[]", applies="[Wii/1.3]"),
    "README.md": "# not an idea\n",
}

# ---- parsing ---------------------------------------------------------------------------------------------
fm, body = spi.parse_front_matter(idea_text(7, problem="a: b \"quoted\" c"))
check("front matter: value read raw (colon and quotes kept)", fm["problem"], 'a: b "quoted" c')
check("front matter: empty demo is empty", fm["demo"], "")
check("front matter: body starts after the block", body.startswith("\n# 7."), True)
check("front matter: CRLF accepted", spi.parse_front_matter(idea_text(7).replace("\n", "\r\n"))[0]["id"], "7")
check("tags: bracket list", spi.parse_list("[a, b]"), ["a", "b"])
check("tags: empty list", spi.parse_list("[]"), [])
check("tags: not a list", spi.parse_list("a, b"), None)
check("problem_sentence: continuation lines joined",
      spi.problem_sentence(["", "**Problem.** one", "two", "", "x"]), "one two")

# ---- the good fixture --------------------------------------------------------------------------------------
root = fixture(GOOD)
ideas, defects = spi.load_ideas(root)
check("good: no defects", defects, [])
check("good: sorted by id, README ignored", [i["id"] for i in ideas], [1, 2, 3])
block = spi.build_index(ideas)
rows = [ln for ln in block.split("\n") if ln.startswith("| ") and not ln.startswith("| # ") and "---" not in ln]
check("index: rows sorted by id", [r.split("|")[1].strip() for r in rows], ["1", "2", "3"])
check("index: pipe in the problem escaped", spi.BS + "|" in rows[2], True)
check("index: problem capitalised", "Lower-case start" not in rows[0] and "The first problem" in rows[0], True)
check("index: title links to its file", "[The first idea](001-first-idea.md)" in rows[0], True)
check("index: tags column", "| pragma, data |" in rows[2], True)
check("index: starts with the generated banner", block.split("\n")[0], spi.BEGIN)
check("index: 220-char cap with ellipsis",
      spi.cell("word " * 80).endswith("...") and len(spi.cell("word " * 80)) <= spi.CELL_CAP + 3, True)

# ---- grouping by status and the tag suggestion ------------------------------------------------------------
gfiles = dict(GOOD)
gfiles["004-tried.md"] = idea_text(4, "Tried and dropped", status="ruled-out")
gfiles["005-later.md"] = idea_text(5, "Not yet tried", status="todo")
gblock = spi.build_index(spi.load_ideas(fixture(gfiles))[0])
heads = [ln for ln in gblock.split(spi.NL) if ln.startswith("## ")]
check("index: one section per non-empty status, in fixed order",
      [h.split(" (")[0] for h in heads], ["## Ideas that work", "## Ruled out - tried and it did not work, do not re-run",
                                           "## Not tried yet"])
check("index: the section counts", [h.rsplit("(", 1)[1] for h in heads], ["3)", "1)", "1)"])
check("index: an idea sits under its own status heading",
      gblock.index("004-tried.md") > gblock.index("## Ruled out") and gblock.index("004-tried.md") < gblock.index("## Not tried"), True)
check("index: no empty section for an unused status", "## Superseded" in gblock, False)
check("tags: derive_tags is ranked and capped", len(spi.derive_tags("pragma flag linker section symbol reloc measure", "")) <= spi.MAX_TAGS, True)
check("tags: derive_tags stays inside the vocabulary", set(spi.derive_tags("a pragma and a vtable", "")) <= set(spi.TAGS), True)

# ---- refusals ----------------------------------------------------------------------------------------------
dup = dict(GOOD)
dup["003-again.md"] = idea_text(3, "Clash")
check("refuse: duplicate id named with its files",
      any("id 3 appears 2 times" in d and "003-again.md" in d for d in defects_of(dup)), True)
for label, files, want in (
        ("missing key", {"001-a.md": idea_text(1, drop="problem")}, "key `problem` is missing"),
        ("unknown key", {"001-a.md": idea_text(1, extra="colour: red")}, "unknown front-matter key `colour`"),
        ("unknown tag", {"001-a.md": idea_text(1, tags="[flags, bogus]")}, "unknown tag `bogus`"),
        ("bad status", {"001-a.md": idea_text(1, status="maybe")}, "status `maybe`"),
        ("tags not a list", {"001-a.md": idea_text(1, tags="flags")}, "tags must be a bracket list"),
        ("id/name mismatch", {"005-a.md": idea_text(6)}, "does not match front-matter id 6"),
        ("bad file name", {"001-Bad_Slug.md": idea_text(1)}, "is not `NNN-slug.md`"),
        ("no front matter", {"001-a.md": "# 1. no fm\n"}, "no front matter"),
        ("unclosed front matter", {"001-a.md": "---\nid: 1"}, "never closed"),
        ("empty problem", {"001-a.md": idea_text(1, problem="")}, "empty problem"),
        ("demo names a missing file", {"001-a.md": idea_text(1, demo="001-a.cpp")}, "missing file `001-a.cpp`"),
        ("demo of another id", {"001-a.md": idea_text(1, demo="002-x.cpp"), "002-x.cpp": ""}, "must be a `001-*` file"),
        ("empty dir", {}, "no NNN-slug.md idea file")):
    check("refuse: %s" % label, any(want in d for d in defects_of(files)), True)
# optional stage-4 keys: reviewed / related / superseded_by
check("accept: reviewed, related and superseded_by",
      defects_of({"001-a.md": idea_text(1, extra="reviewed: 2026-09-29" + chr(10) + "related: [2]" + chr(10)
                                        + "superseded_by: 2"),
                  "002-b.md": idea_text(2)}), [])
for label, extra, want in (
        ("a malformed reviewed date", "reviewed: yesterday", "reviewed `yesterday`"),
        ("related that is not a list of ids", "related: 2", "related must be a bracket list of idea ids"),
        ("related naming a missing idea", "related: [9]", "related id 9"),
        ("related naming itself", "related: [1]", "related id 1"),
        ("superseded_by naming a missing idea", "superseded_by: 9", "superseded_by 9"),
        ("superseded_by that is not an id", "superseded_by: two", "superseded_by `two`")):
    check("refuse: %s" % label, any(want in d for d in defects_of({"001-a.md": idea_text(1, extra=extra),
                                                                     "002-b.md": idea_text(2)})), True)
check("refuse: status superseded without superseded_by",
      any("needs `superseded_by: N`" in d for d in defects_of({"001-a.md": idea_text(1, status="superseded")})), True)
check("json: the optional keys are exposed",
      [ (i["reviewed"], i["related"], i["superseded_by"]) for i in
        [x for x in spi.load_ideas(tempfile.mkdtemp(prefix="spi-x-"))[0]] ], [])
check("accept: a demo that exists", defects_of({"001-a.md": idea_text(1, demo="001-a.cpp"), "001-a.cpp": "int x;\n"}), [])
check("refuse: a missing docs/matching dir",
      spi.load_ideas(tempfile.mkdtemp(prefix="spi-empty-"))[1], ["docs/matching does not exist"])

# ---- end to end ----------------------------------------------------------------------------------------------
root = fixture(GOOD)
rc, out, err = run(root)
check("e2e: write reports an update", (rc, "updated" in out), (0, True))
target = os.path.join(root, "docs", "matching", "index.md")
check("e2e: index.md written", os.path.isfile(target), True)
check("e2e: --check passes", run(root, check=True)[0], 0)
check("e2e: index.md is not itself parsed as an idea", spi.load_ideas(root)[1], [])
with open(target, "a", encoding="utf-8", newline="") as f:
    f.write("tampered\n")
check("e2e: --check fails once index.md is stale", run(root, check=True)[0], 1)
check("e2e: a second write repairs it", (run(root)[0], run(root, check=True)[0]), (0, 0))
rc, out, err = run(root, where=2)
check("where: prints the path", (rc, out.strip()), (0, "docs/matching/002-second-idea.md"))
rc, out, err = run(root, where=99)
check("where: unknown id exits 1", (rc, out, "no idea 99" in err), (1, "", True))
rc, out, err = run(root, json=True)
check("json: parsed index", [(i["id"], i["file"]) for i in json.loads(out)],
      [(1, "001-first-idea.md"), (2, "002-second-idea.md"), (3, "003-third-idea.md")])
bad = fixture(dup)
rc, out, err = run(bad)
check("e2e: a refusal writes nothing and exits 1",
      (rc, os.path.exists(os.path.join(bad, "docs", "matching", "index.md"))), (1, False))

# ---- the real tree -------------------------------------------------------------------------------------
real = spi.find_root()
real_ideas, real_defects = spi.load_ideas(real)
check("real: docs/matching parses with no defect", real_defects, [])
check("real: ids are unique and contiguous from 1",
      [i["id"] for i in real_ideas] == list(range(1, len(real_ideas) + 1)), True)
try:
    have = spi.read(os.path.join(real, *spi.TARGET_REL.split("/")))
except OSError:
    have = None
check("real: docs/matching/index.md is in sync with the idea files", have, spi.build_index(real_ideas))

failed = [r for r in RESULTS if not r[1]]
for name, ok, got, want in RESULTS:
    if not ok:
        print("FAIL  %s\n      got  %r\n      want %r" % (name, got, want))
print("%s - %d checks" % ("FAILED" if failed else "ok", len(RESULTS)))
sys.exit(1 if failed else 0)
