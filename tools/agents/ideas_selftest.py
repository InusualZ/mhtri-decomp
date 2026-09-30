#!/usr/bin/env python3
"""Fixture tests for tools/agents/ideas.py.

Covers: `find` ranking and filters on fixtures, `new` scaffolding a valid idea (and a codegen demo) that passes
`check`, id allocation under a thread race (distinct ids, no lost file), the refusals of `new`, every class
`check` must fail on, the demo header parser, and the real tree.

    python tools/agents/ideas_selftest.py
"""
import argparse
import contextlib
import io
import os
import sys
import tempfile
import threading

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ideas  # noqa: E402
import sync_playbook_index as spi  # noqa: E402

RESULTS = []


def check(name, got, want):
    RESULTS.append((name, got == want, got, want))
    return got == want


def idea_text(n, title="An idea", status="works", problem="A problem.", tags="[flags]", applies="[]", demo="",
              h1=None):
    lines = ["---", "id: %d" % n, "title: %s" % title, "status: %s" % status, "problem: %s" % problem,
             "tags: %s" % tags, "applies: %s" % applies, "demo: %s" % demo if demo else "demo:", "---", "",
             h1 or "# %d. %s" % (n, title), "", "**Problem.** %s" % problem, ""]
    return "\n".join(lines)


def fixture(files):
    root = tempfile.mkdtemp(prefix="ideas-selftest-")
    os.makedirs(os.path.join(root, "docs", "matching"))
    open(os.path.join(root, "configure.py"), "w").close()
    for name, text in files.items():
        with open(os.path.join(root, "docs", "matching", name), "w", encoding="utf-8", newline="") as f:
            f.write(text)
    return root


def sync(root):
    ideas.sync_generated(root)


def run(fn, root, **kw):
    out, err = io.StringIO(), io.StringIO()
    a = argparse.Namespace(**kw)
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        rc = fn(root, a, out)
    return rc, out.getvalue(), err.getvalue()


def find_args(*words, tag=None, status=None, applies=None, limit=8):
    return dict(words=list(words), tag=tag, status=status, applies=applies, limit=limit)


FILES = {
    "001-per-unit-instrument.md": idea_text(1, "Use a per-unit instrument", tags="[measurement]",
                                            problem="The project-wide pass/fail signal is useless while any object is NonMatching."),
    "002-fp-contract-off.md": idea_text(2, "Unfused multiply-adds need fp_contract off", tags="[flags]", applies="[Wii/1.3]",
                                        problem="Retail keeps fmuls and fadds where we emit a fused fmadds."),
    "003-stale-object.md": idea_text(3, "Guard against stale objects", tags="[measurement, tooling]", status="ruled-out",
                                     problem="A scripted run diffed a stale object the compiler never wrote."),
    "README.md": "# not an idea\n",
}

# ---- find -----------------------------------------------------------------------------------------------------
root = fixture(FILES)
sync(root)
rc, out, _ = run(ideas.cmd_find, root, **find_args("fmadds", "fused"))
check("find: symptom words in the problem rank the idea first", (rc, out.split()[0]), (0, "2"))
rc, out, _ = run(ideas.cmd_find, root, **find_args("stale"))
check("find: title word", out.split()[0], "3")
rc, out, _ = run(ideas.cmd_find, root, **find_args("measurement"))
check("find: a tag word matches the tag (title hits rank above problem hits)", out.split()[0], "1")
rc, out, _ = run(ideas.cmd_find, root, **find_args("object"))
ids = [ln.split()[0] for ln in out.splitlines() if ln[:3].strip().isdigit()]
check("find: a word in several problems ranks title before problem-only", ids, ["3", "1"])
rc, out, _ = run(ideas.cmd_find, root, **find_args(tag="tooling"))
check("find: --tag alone lists by tag", ([ln.split()[0] for ln in out.splitlines() if ln[:3].strip().isdigit()]), ["3"])
rc, out, _ = run(ideas.cmd_find, root, **find_args("object", status="works"))
check("find: --status filters", [ln.split()[0] for ln in out.splitlines() if ln[:3].strip().isdigit()], ["1"])
rc, out, _ = run(ideas.cmd_find, root, **find_args(applies="Wii/1.3"))
check("find: --applies filters", [ln.split()[0] for ln in out.splitlines() if ln[:3].strip().isdigit()], ["2"])
rc, out, err = run(ideas.cmd_find, root, **find_args("zzzznothing"))
check("find: no match exits 1", (rc, out), (1, ""))
rc, out, _ = run(ideas.cmd_find, root, **find_args("fmadds"))
check("find: prints the path", "docs/matching/002-fp-contract-off.md" in out, True)

# ---- where / show --------------------------------------------------------------------------------------------
rc, out, _ = run(ideas.cmd_where, root, n=2)
check("where: path", (rc, out.strip()), (0, "docs/matching/002-fp-contract-off.md"))
rc, out, err = run(ideas.cmd_where, root, n=9)
check("where: unknown", (rc, "no idea 9" in err), (1, True))
rc, out, _ = run(ideas.cmd_show, root, n=1)
check("show: prints front matter and body", (rc, "id: 1" in out and "**Problem.**" in out), (0, True))

# ---- new -------------------------------------------------------------------------------------------------------
n, md = ideas.new_idea(root, "A fresh `codegen` idea", ["flags", "pragma"], kind="codegen", applies=["Wii/1.3"])
check("new: next free id after 3", n, 4)
check("new: slug from the title", os.path.basename(md), "004-fresh-codegen-idea.md")
d = os.path.join(root, "docs", "matching")
check("new: codegen kind writes the demo", os.path.isfile(os.path.join(d, "004-fresh-codegen-idea.cpp")), True)
check("new: no reservation left behind", [x for x in os.listdir(d) if x.endswith(".lock")], [])
defects, all_ideas = ideas.check_root(root, skill=False)
check("new: the scaffold passes check", defects, [])
check("new: front matter records status todo and the demo",
      [(i["status"], i["demo"], i["applies"]) for i in all_ideas if i["id"] == 4],
      [("todo", "004-fresh-codegen-idea.cpp", ["Wii/1.3"])])
check("new: the index was regenerated (it lists idea 4)", "004-fresh-codegen-idea.md" in
      spi.read(os.path.join(d, "index.md")), True)
n2, md2 = ideas.new_idea(root, "Process only", ["process"], kind="process", slug="process-only")
check("new: process kind has no demo", (os.path.exists(os.path.join(d, "005-process-only.cpp")), n2), (False, 5))
for label, kw, want in (("unknown tag", dict(tags=["bogus"]), "unknown tag"),
                        ("bad slug", dict(tags=["flags"], slug="Bad_Slug"), "slug"),
                        ("bad kind", dict(tags=["flags"], kind="weird"), "--kind")):
    try:
        ideas.new_idea(root, "x", sync=False, **kw)
        got = ""
    except SystemExit as e:
        got = str(e)
    check("new: refuses %s" % label, want in got, True)
check("new: a refusal takes no id", max(ideas.taken_ids(d)), 5)

# ---- the race ----------------------------------------------------------------------------------------------------
race = fixture({"001-a.md": idea_text(1, "A")})
got_ids, errors = [], []
gate = threading.Barrier(12)


def worker(k):
    try:
        gate.wait()
        # half the racers ask for the same slug, half for distinct ones: neither may share an id
        nid, _p = ideas.new_idea(race, "Racer %d" % k, ["flags"], slug="same" if k % 2 else "slug%d" % k, sync=False)
        got_ids.append(nid)
    except BaseException as e:  # noqa: BLE001
        errors.append(repr(e))


ts = [threading.Thread(target=worker, args=(k,)) for k in range(12)]
for t in ts:
    t.start()
for t in ts:
    t.join()
check("race: no thread failed", errors, [])
check("race: 12 distinct ids, contiguous after the existing one", sorted(got_ids), list(range(2, 14)))
rd = os.path.join(race, "docs", "matching")
md_files = sorted(x for x in os.listdir(rd) if x.endswith(".md"))
check("race: every id has exactly one file", [x[:3] for x in md_files], ["%03d" % k for k in range(1, 14)])
check("race: the raced tree passes the loader", spi.load_ideas(race)[1], [])
lock = os.path.join(rd, ".014.lock")
open(lock, "w").close()
nid, _p = ideas.new_idea(race, "After a crash", ["flags"], sync=False)
check("race: a stale reservation is skipped, never reused", nid, 15)
check("check: a stale reservation is reported",
      any("stale id reservation" in x for x in ideas.check_root(race, skill=False)[0]), True)

# ---- check refusals ------------------------------------------------------------------------------------------------


def defects_of(files, sync_index=True):
    r = fixture(files)
    if sync_index:
        ideas_, dd = spi.load_ideas(r)
        if not dd:
            spi.write(os.path.join(r, "docs", "matching", "index.md"), spi.build_index(ideas_))
    return ideas.check_root(r, skill=False)[0]


check("check: a clean fixture", defects_of({"001-a.md": idea_text(1)}), [])
check("check: the H1 must agree with the title",
      any("H1" in x for x in defects_of({"001-a.md": idea_text(1, h1="# 1. Another title")})), True)
check("check: a stale index", any("stale" in x for x in defects_of({"001-a.md": idea_text(1)}, sync_index=False)), True)
check("check: a missing demo", any("missing file" in x for x in defects_of({"001-a.md": idea_text(1, demo="001-a.cpp")})), True)
check("check: an orphan demo file", any("no idea's" in x for x in defects_of({"001-a.md": idea_text(1), "001-b.cpp": "int x;"})), True)
good_demo = "/* Demo.\n * FLAGS: -O4,p\n * EXPECT: contains fmuls\n * EXPECT: absent fmadds\n */\nint x;\n"
check("check: a well-formed demo passes",
      defects_of({"001-a.md": idea_text(1, demo="001-a.cpp"), "001-a.cpp": good_demo}), [])
check("check: a demo without a header is refused",
      any("header comment" in x for x in defects_of({"001-a.md": idea_text(1, demo="001-a.cpp"), "001-a.cpp": "int x;\n"})), True)
check("check: a demo without EXPECT is refused",
      any("no `EXPECT:`" in x for x in defects_of({"001-a.md": idea_text(1, demo="001-a.cpp"),
                                                    "001-a.cpp": "/* FLAGS: -O4 */\n"})), True)
check("check: a bad EXPECT is refused",
      any("not `contains" in x for x in defects_of({"001-a.md": idea_text(1, demo="001-a.cpp"),
                                                     "001-a.cpp": "/*\n * FLAGS: -O4\n * EXPECT: fast\n */\n"})), True)
check("check: schema errors surface", any("unknown tag" in x for x in defects_of({"001-a.md": idea_text(1, tags="[nope]")})), True)
check("check: duplicate ids surface", any("appears 2 times" in x for x in
                                          defects_of({"001-a.md": idea_text(1), "001-b.md": idea_text(1)})), True)

# ---- demo header ------------------------------------------------------------------------------------------------
info, dd = ideas.demo_header("/* Demo for idea 9.\n * FLAGS: -O4,p -inline auto      note\n * MWCC: Wii/1.0\n"
                             " * EXPECT: size fn_x 0x40\n * EXPECT: contains lwz    prose\n */\n")
check("demo header: parsed", (info["FLAGS"], info["MWCC"], info["EXPECT"], dd),
      ("-O4,p -inline auto", "Wii/1.0", ["size fn_x 0x40", "contains lwz"], []))
info, dd = ideas.demo_header("// FLAGS: -O4\n")
check("demo header: line comments are not a header", (info, dd), (None, ["no opening `/* ... */` header comment"]))
check("scaffold: the shipped skeleton's own header parses",
      ideas.demo_header(ideas.DEMO_SKELETON.format(n=1, title="t", under="t"))[1], [])
check("find: score is word-substring (reloc finds relocations)",
      ideas.score({"title": "x", "tags": ["relocations"], "slug": "x", "applies": [], "problem": "y"}, ["reloc"]) > 0, True)
check("tags: derive_tags suggests from a title", "pragma" in spi.derive_tags("A pragma leaks", ""), True)

# ---- the real tree ------------------------------------------------------------------------------------------------
real = spi.find_root()
defects, real_ideas = ideas.check_root(real)
check("real: ideas.py check is clean", defects, [])
check("real: every status is in the vocabulary and ids are contiguous",
      (all(i["status"] in spi.STATUSES for i in real_ideas), [i["id"] for i in real_ideas] == list(range(1, len(real_ideas) + 1))),
      (True, True))
check("real: every idea has at least one tag", [i["id"] for i in real_ideas if not i["tags"]], [])
check("real: ideas.py did not leave a reservation in the tree",
      [x for x in os.listdir(os.path.join(real, "docs", "matching")) if x.endswith(".lock")], [])

failed = [r for r in RESULTS if not r[1]]
for name, ok, got, want in RESULTS:
    if not ok:
        print("FAIL  %s\n      got  %r\n      want %r" % (name, got, want))
print("%s - %d checks" % ("FAILED" if failed else "ok", len(RESULTS)))
sys.exit(1 if failed else 0)
