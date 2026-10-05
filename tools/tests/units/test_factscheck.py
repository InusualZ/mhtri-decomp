"""factscheck on a temp git repo: a removed comment whose facts survive elsewhere passes, a unique fact fails (and passes
once a doc carries it - the mutation that distinguishes the two), inert classes are named, the diff is taken from the
merge base, and the CLI exits 1/0 with the lib.findings JSON."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json

from tools.lib import facts, testing
from tools.units import factscheck

TIER = "fixture"

SYMBOLS = ("fn_80001000 = .text:0x80001000; // type:function size:0x40\n"
           "known_symbol_name = .text:0x80001040; // type:function size:0x20\n")
SPLITS = "Sections:\n\t.text type:code\n\nmod/a.c:\n\t.text       start:0x80001000 end:0x80001060\n"
HEADER = """/*
 * mod/a.c - the unit.
 *
 * REMOVED.  `known_symbol_name` lives at 0x80001040 and calls fn_80001000; docs name
 * documented_fact_word and sibling_only_field; the old unit was auto/80001000_fn_80001000.c (2026-09-25), its siblings
 * fn_8000Bxxx; the header is mod/a.h; unique_lost_fact sits at 0xDEADBEEF and 31337 more.
 *
 * KEPT.  stays here.
 */
int x;
"""
BASE = {"configure.py": "# cflags_main lives here\n", "config/RMHE08/symbols.txt": SYMBOLS,
        "config/RMHE08/splits.txt": SPLITS, "docs/a.md": "documented_fact_word\n",
        "src/mod/a.c": HEADER, "src/mod/a.h": "int sibling_only_field;\n"}


def _without_removed(text):
    lines = text.split("\n")
    return "\n".join(ln for ln in lines if not any(k in ln for k in ("REMOVED", "documented_fact", "fn_8000B",
                                                                         "unique_lost")))


def _repo():
    g = testing.GitFixture().init()
    g.commit(BASE, "base")
    return g


def _cli(g, *argv):
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        rc = factscheck.TOOL.run(factscheck.main, ["--root", str(g.root), *argv], parser=factscheck.build_parser())
    return rc, out.getvalue()


def test_tokens_and_classes(c):
    toks = dict(facts.fact_tokens("see 0x8009CD64..0x8009CDBC, 31337 B; fn_8004Bxxx; 2026-09-25; x_y; "
                                       "drawTextRuns; Ab; 12; 0x10; auto/80001000_fn_80001000.c"))
    c.check("a hex after `..` is a fact (the range end)", "0x8009CDBC" in toks, True)
    c.check("a glob over generated names is inert", toks.get("fn_8004Bxxx"), "inert: name glob")
    c.check("the provenance path's name is inert", toks.get("80001000_fn_80001000"), "inert: provenance path")
    c.check("short words, two-digit numbers and short hex are not facts",
            [t for t in ("Ab", "12", "0x10") if t in toks], [])
    c.check("camel humps of six or more and underscored names are facts",
            ("drawTextRuns" in toks, "x_y" in toks), (True, True))
    c.check("a date's digits are inert, not numbers", toks.get("2026"), "inert: date")


def test_parse_diff(c):
    diff = ("diff --git a/f.c b/f.c\n--- a/f.c\n+++ b/f.c\n@@ -3,2 +3,1 @@\n-one\n-two\n+new\n"
            "@@ -9 +8,0 @@\n-three\ndiff --git a/n.c b/n.c\n--- /dev/null\n+++ b/n.c\n@@ -0,0 +1 @@\n+x\n")
    got = [(s.file, s.line, s.text) for s in facts.parse_diff(diff)]
    c.check("one span per run of removed lines, old line numbers; an added file has none",
            got, [("f.c", 3, "one\ntwo"), ("f.c", 9, "three")])


def test_removed_facts(c):
    g = _repo()
    try:
        (g.root / "src/mod/a.c").write_text(_without_removed(HEADER), encoding="utf-8", newline="\n")
        res = factscheck.run(str(g.root), "main")
        bad = [t for f in res["failures"] for t in f["tokens"]]
        c.check("only the facts that survive nowhere fail", sorted(bad), sorted(["unique_lost_fact", "0xDEADBEEF",
                                                                                 "31337"]))
        verdicts = {t: w for e in res["explained"] for t, w in e["tokens"]}
        c.check("a token in the map survives", verdicts["known_symbol_name"], "config/RMHE08/symbols.txt")
        c.check("a token in docs survives", verdicts["documented_fact_word"], "docs/**")
        c.check("a token in the unit's own header survives", verdicts["sibling_only_field"], "new text of the unit")
        c.check("an existing path is accepted whole", verdicts.get("mod/a.h", verdicts.get("a")), None)
        c.check("a generated stem whose address the map holds is derivable",
                verdicts["fn_80001000"].startswith(("new text", "config/", "derivable")), True)
        rc, out = _cli(g)
        c.check("the CLI exits 1 and names the unmatched tokens", (rc, "unique_lost_fact" in out), (1, True))
        rc, out = _cli(g, "--json")
        data = json.loads(out)
        c.check("the JSON is the lib.findings schema", (data["tool"], data["ok"], len(data["rows"])),
                ("factscheck", False, 3))
        rc, out = _cli(g, "--explain")
        c.check("--explain names every token's source", ("UNMATCHED" in out, "docs/**" in out), (True, True))
        # the mutation: the same removal passes once the facts live in a doc
        (g.root / "docs/b.md").write_text("unique_lost_fact 0xDEADBEEF 31337\n", encoding="utf-8")
        g.git("add", "docs/b.md")
        rc, _out = _cli(g)
        c.check("with the facts in docs the removal passes", rc, 0)
    finally:
        g.cleanup()


def test_merge_base(c):
    g = _repo()
    try:
        g.branch("work", checkout=True)
        g.commit({"src/mod/a.c": HEADER.replace(" *\n * KEPT.  stays here.\n", "\n")}, "drop KEPT")
        g.checkout("main")
        g.commit({"src/mod/a.c": _without_removed(HEADER)}, "main drops facts")
        g.checkout("work")
        res = factscheck.run(str(g.root), "main", "HEAD")
        c.check("main's own removal is not this branch's (merge base, not main's tip)", res["failures"], [])
        c.check("the branch's removal is checked", res["removed_spans"] >= 1, True)
    finally:
        g.cleanup()


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
