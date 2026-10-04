#!/usr/bin/env python3
"""Fixture tests for tools/agents/ideas.py.

Covers: `find` ranking and filters on fixtures, `new` scaffolding a valid idea (and a codegen demo) that passes
`check`, id allocation under a thread race (distinct ids, no lost file), the refusals of `new`, every class
`check` must fail on, the demo header parser, and the real tree.

    python tools/agents/ideas_selftest.py
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import contextlib
import io
import os
import sys
import tempfile
import threading

from tools.agents import ideas
from tools.agents import sync_playbook_index as spi

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
      any("not one of" in x for x in defects_of({"001-a.md": idea_text(1, demo="001-a.cpp"),
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

# ---- demo-check: the EXPECT grammar, the object model, the runner ---------------------------------------------------
from tools.agents import ideas_demo  # noqa: E402

DUMP = "\n".join([
    "", "x.o:     file format elf32-powerpc", "", "Sections:", "Idx Name          Size      VMA       LMA       File off  Algn",
    "  0 .text         00000028  00000000  00000000  00000040  2**4", "                  CONTENTS, ALLOC, LOAD, RELOC, READONLY, CODE",
    "  1 .data         00000020  00000000  00000000  00000068  2**3", "SYMBOL TABLE:",
    "00000000 l    df *ABS*\t00000000 x.cpp",
    "00000000 l     O .data\t00000010 g_first", "00000010 l     O .data\t00000010 @9",
    "00000018 g     O .data\t00000004 g_late", "00000000         *UND*\t00000000 callee",
    "00000000 g     F .text\t0000001c f__Fi", "00000020 g     F .text\t00000008 h",
    "Contents of section .data:", " 0000 41420a00 00000000 00000000 00000000  AB..............",
    " 0010 0d0a0000 00000000 00000000 00000000  ................", "",
    "Disassembly of section .text:", "", "00000000 <f__Fi>:",
    "   0:\t94 21 ff f0 \tstwu    r1,-16(r1)", "   4:\t48 00 00 01 \tbl      4 <f__Fi+0x4>",
    "\t\t\t4: R_PPC_REL24\tcallee", "   8:\t54 60 06 30 \trlwinm  r0,r3,0,24,24", "   c:\t54 60 06 30 \trlwinm  r0,r3,0,24,24",
    "  10:\t4e 80 00 20 \tblr", "", "00000020 <h>:", "  20:\t4e 80 00 20 \tblr", ""])
O = ideas_demo.Obj(DUMP)


def ev(line):
    return ideas_demo.evaluate(O, line)[0]


check("obj: sections, symbols and functions are read", (O.sections, sorted(O.funcs), len(O.symbols)),
      ({".text": 0x28, ".data": 0x20}, ["f__Fi", "h"], 6))
check("obj: section contents are read as hex", O.contents[".data"][:8], "41420a00")
check("expect: contains a mnemonic, a symbol and a relocation target", (ev("contains rlwinm"), ev("contains g_first"), ev("contains callee")),
      (True, True, True))
check("expect: a mangled symbol is found by its C++ prefix", (ev("size f 0x1c"), ev("size f__Fi 28")), (True, True))
check("expect: absent is the negation", (ev("absent fmadds"), ev("absent rlwinm")), (True, False))
check("expect: size, and a wrong size fails", (ev("size f 0x1c"), ev("size f 0x20"), ev("size nosuch 4")), (True, False, False))
check("expect: section size, missing section is 0", (ev("section .text 0x28"), ev("section .rodata 0"), ev("section .rodata 4")),
      (True, True, False))
check("expect: order is by address inside the section", (ev("order .data g_first < g_late"), ev("order .data g_late < g_first"),
                                                          ev("order .data @9 < g_late"), ev("order .text g_first < g_late")),
      (True, False, True, False))
check("expect: reloc", (ev("reloc callee"), ev("reloc other")), (True, False))
check("expect: seq is an ordered subsequence, optionally inside a function",
      (ev("seq stwu bl blr"), ev("seq blr stwu"), ev("seq stwu blr in f"), ev("seq stwu blr in h"), ev("seq blr in nosuch")),
      (True, False, True, False, False))
check("expect: count, optionally inside a function", (ev("count rlwinm 2"), ev("count blr 1 in h"), ev("count blr 2"), ev("count rlwinm 1")),
      (True, True, True, False))
check("expect: insn matches mnemonic and operands", (ev("insn rlwinm r0,r3,0,24,24"), ev("insn rlwinm r0,r3,0,24,24 in f"),
                                                     ev("insn rlwinm r3,r0,0,24,24"), ev("insn rlwinm r0,r3,0,24,24 in h")),
      (True, True, False, False))
check("expect: bytes and nobytes read the section contents", (ev("bytes .data 41 42 0a"), ev("bytes .data 0d0a"), ev("nobytes .data 0d0a"),
                                                              ev("nobytes .data ffff"), ev("bytes .rodata 00")),
      (True, True, False, True, False))
check("expect: every vocabulary form is accepted by the grammar",
      all(ideas_demo.expect_ok(x) for x in ("contains a", "absent b", "size f 0x10", "section .data 8", "order .data a < b", "reloc s",
                                            "seq a b c", "seq a b in f", "count x 2", "count x 2 in f", "insn li r3,0", "insn li r3,0 in f",
                                            "bytes .data 0a0d", "nobytes .data 0d0a")), True)
check("expect: malformed lines are refused", [x for x in ("fast", "size f", "order .data a b", "count x", "bytes .data zz")
                                              if ideas_demo.expect_ok(x)], [])
check("expect: an unknown verb is a failed EXPECT, not a crash", ideas_demo.evaluate(O, "wobble x")[0], False)
rep_failing = ideas_demo.excerpt(O, "size f 0x20")
check("report: a failed size shows the symbol row", "f__Fi" in rep_failing, True)

good = "/*\n * FLAGS: -O4,p\n * EXPECT: contains rlwinm\n * EXPECT: size f 0x1c\n */\nint x;\n"
bad = "/*\n * FLAGS: -O4,p\n * EXPECT: contains rlwinm\n * EXPECT: size f 0x99\n */\nint x;\n"
droot = fixture({"001-a.md": idea_text(1, demo="001-a.cpp"), "001-a.cpp": good, "002-b.md": idea_text(2, demo="002-b.cpp"), "002-b.cpp": bad})
sync(droot)
stub = lambda root_, info_, path_: (O, "")  # noqa: E731
stub_skip = lambda root_, info_, path_: (None, "SKIP compiler missing: build/compilers/x")  # noqa: E731
stub_fail = lambda root_, info_, path_: (None, "compile failed (rc 1):\nerror")  # noqa: E731


def dc(compile_fn, **kw):
    out, err = io.StringIO(), io.StringIO()
    args = dict(ids=[], all=False, changed=None, dump=False)
    args.update(kw)
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        rc = ideas.cmd_demo_check(droot, argparse.Namespace(**args), out, compile_fn)
    return rc, out.getvalue(), err.getvalue()


rc, out, _ = dc(stub, all=True)
check("demo-check: a failing EXPECT fails the run and names it", (rc, "FAIL  002" in out, "PASS  001" in out, "size f 0x99" in out,
                                                                   "`f` is 28 (0x1C) bytes, want 153" in out), (1, True, True, True, True))
rc, out, _ = dc(stub, ids=[1])
check("demo-check: naming an idea checks only it", (rc, "PASS  001" in out, "002" in out), (0, True, False))
rc, out, err = dc(stub, ids=[7])
check("demo-check: an idea with no demo is refused", (rc, "no demo for idea" in err), (1, True))
rc, out, err = dc(stub)
check("demo-check: no selection is a usage error", rc, 2)
rc, out, _ = dc(stub_skip, ids=[1])
check("demo-check: a missing compiler is a SKIP with a distinct exit code, never a silent pass", (rc, "SKIP  001" in out), (3, True))
rc, out, _ = dc(stub_fail, ids=[1])
check("demo-check: a compile error is a FAIL that shows the log", (rc, "FAIL  001" in out, "compile failed" in out), (1, True, True))
orig_changed = ideas.changed_paths
ideas.changed_paths = lambda root_, ref: {"docs/matching/001-a.cpp"}
rc, out, _ = dc(stub, changed="main")
check("demo-check: --changed selects the demo whose file changed", (rc, "PASS  001" in out, "002" in out), (0, True, False))
ideas.changed_paths = lambda root_, ref: {"docs/matching/002-b.md"}
rc, out, _ = dc(stub, changed="main")
check("demo-check: --changed also selects a demo by its idea file", (rc, "FAIL  002" in out), (1, True))
ideas.changed_paths = lambda root_, ref: {"tools/whatever.py"}
rc, out, _ = dc(stub, changed="main")
check("demo-check: --changed with no demo touched passes without compiling", (rc, "no demo changed" in out), (0, True))
ideas.changed_paths = orig_changed
rc, out, _ = dc(stub, ids=[1], dump=True)
check("demo-check: --dump prints the object instead of checking", (rc, "==== 001" in out, "PASS" in out), (0, True, False))
noexp = fixture({"001-a.md": idea_text(1, demo="001-a.cpp"), "001-a.cpp": "/*\n * FLAGS: -O4\n */\nint x;\n"})
out, err = io.StringIO(), io.StringIO()
with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
    rc = ideas.run_demos(noexp, [({"id": 1, "demo": "001-a.cpp"}, os.path.join(noexp, "docs", "matching", "001-a.cpp"))], out, stub)
check("demo-check: a malformed header is a FAIL naming the defect", (rc[1], "no `EXPECT:`" in out.getvalue()), (1, True))

# ---- the base flags come from configure.py, and FLAGS replace a family -----------------------------------------------------
real_root = spi.find_root()
base = ideas_demo.base_flags(real_root)
check("flags: cflags_base is read from configure.py (no hand copy)", ("-O4,p" in base, "-inline" in base, "cats off" in base, "-DNDEBUG=1" in base),
      (True, True, True, True))
check("flags: -pragma \"cats off\" stays two tokens", base[base.index("-pragma") + 1], "cats off")
argv, comp = ideas_demo.demo_command(real_root, {"FLAGS": "-O3 -str reuse,pool", "MWCC": "Wii/1.0"}, "x.cpp", "out")
check("flags: a demo's -O3 replaces the base -O4,p and -str replaces -str", ("-O3" in argv, "-O4,p" in argv, "reuse,pool" in argv, argv.count("-str")),
      (True, False, True, 1))
check("flags: MWCC selects the compiler", comp.replace("\\", "/").endswith("build/compilers/Wii/1.0/mwcceppc.exe"), True)

# ---- ONE real compile of a tiny demo (skipped without build/compilers) ---------------------------------------------------
if os.path.isfile(ideas_demo.demo_command(real_root, {"FLAGS": ""}, "x", "o")[1]) and os.path.isfile(ideas_demo.objdump_exe(real_root)):
    tiny = tempfile.mkdtemp(prefix="ideas-demo-real-")
    path = os.path.join(tiny, "tiny.cpp")
    with open(path, "w", newline="") as f:
        f.write('/*\n * FLAGS: -O4,p\n * EXPECT: contains rlwinm\n * EXPECT: size masked 12\n * EXPECT: absent fmadds\n'
                ' * EXPECT: insn rlwinm r3,r0,0,24,24 in masked\n */\nstruct S { char pad[8]; unsigned char f; };\n'
                'extern "C" int masked(S *s) { return s->f & 0x80; }\n')
    info_, dd_ = ideas.demo_header(spi.read(path))
    status, rep = ideas_demo.check_demo(real_root, info_, path)
    check("real compile: a tiny demo compiles with the real MWCC and every EXPECT holds", (dd_, status, rep), ([], "PASS", []))
    info_["EXPECT"].append("size masked 99")
    status, rep = ideas_demo.check_demo(real_root, info_, path)
    check("real compile: the same object fails a wrong EXPECT", (status, "size masked 99" in "".join(rep)), ("FAIL", True))
else:
    print("note: build/compilers or build/binutils is absent - the real-compile checks are SKIPPED")

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
