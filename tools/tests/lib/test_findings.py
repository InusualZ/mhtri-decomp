"""lib.findings: Finding identity and the add-only credit model, Row rendering, Verdict, the JSON schema, exit codes."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json

from tools.lib import findings as fd
from tools.lib import testing

TIER = "fixture"


def _f(rule, file, token, line=1, detail=None):
    return {"rule": rule, "file": file, "line": line, "token": token,
            "detail": detail if detail is not None else "`%s` has no registered owner" % token}


def test_finding(c):
    f = fd.Finding(7, "src/a.c", 12, "fn_80001234", "auto-generated name `fn_80001234`", text="fn_80001234();")
    c.check("identity ignores the line", f.identity(), (7, "src/a.c", "fn_80001234", "auto-generated name `fn_80001234`"))
    c.check("to_dict key order is the lint's schema", list(f.to_dict()), ["rule", "file", "line", "token", "text", "detail"])
    c.check("remedy appears only when set", "remedy" in fd.Finding(1, "f", 1, remedy="x").to_dict(), True)
    c.check("dict and Finding share one identity", fd.identity(f.to_dict()), f.identity())
    c.check("round trip", fd.Finding.from_dict(f.to_dict()), f)


def test_added_credit_model(c):
    before = [_f(7, "src/a.c", "fn_1", 3), _f(7, "src/a.c", "fn_1", 9)]
    after = before + [_f(7, "src/a.c", "fn_1", 20)] * 30
    c.check("spelling a known token 30 more times adds nothing", fd.added(before, after), {})
    after = before + [_f(7, "src/a.c", "fn_2", 40), _f(7, "src/a.c", "fn_2", 5)]
    got = fd.added(before, after)
    c.check("a new token is one addition, its first occurrence by line",
            {k: [x["line"] for x in v] for k, v in got.items()}, {(7, "src/a.c"): [5]})
    c.check("identity is per (rule, file): the same token in another file is new",
            list(fd.added(before, [_f(7, "src/b.c", "fn_1")])), [(7, "src/b.c")])
    c.check("two complaints about one token are two identities",
            len(fd.added([], [_f(7, "x", "unk1", detail="a"), _f(7, "x", "unk1", detail="b")])[(7, "x")]), 2)
    renamed = lambda f: [dict(f, token="good_name", detail=f["detail"].replace("`fn_1`", "`good_name`"))] \
        if f["token"] == "fn_1" else []
    c.check("without a credit a rename is an addition", list(fd.added(before, [_f(7, "src/a.c", "good_name")])),
            [(7, "src/a.c")])
    c.check("a credited rename is not", fd.added(before, [_f(7, "src/a.c", "good_name")], renamed), {})
    c.check("... and a referrer that kept the old spelling is not either",
            fd.added(before, [_f(7, "src/a.c", "fn_1"), _f(7, "src/a.c", "good_name")], renamed), {})
    c.check("Findings work like dicts", fd.added([fd.Finding.from_dict(x) for x in before],
                                                  [fd.Finding.from_dict(_f(7, "src/a.c", "fn_9"))])[(7, "src/a.c")][0].token,
            "fn_9")


def test_removed(c):
    before = [_f(2, "src/f.c", "lbl_1"), _f(2, "src/f.c", "lbl_1", 5), _f(2, "src/f.c", "lbl_2")]
    c.check("one removal per identity per file", fd.removed(before, [_f(2, "src/f.c", "lbl_2")]),
            {(2, "lbl_1", "`lbl_1` has no registered owner"): ["src/f.c"]})
    credit = lambda f: [dict(f, token="new", detail="`new` has no registered owner")] if f["token"] == "lbl_1" else []
    c.check("a credited spelling still present is no removal",
            fd.removed(before, [_f(2, "src/f.c", "new"), _f(2, "src/f.c", "lbl_2")], credit), {})


def test_rows_and_rendering(c):
    rows = [("ground truth", True, "", "sha ok"), ("style lint", False, "rule 7 +1", "", fd.KIND_GATE, "fix it"),
            ("base recorded", False, "", "", "bookkeeping", ""), ("odd kind", False, "x", "", "weird")]
    rs = fd.rows_of(rows)
    c.check("a 4-tuple and an unknown kind read as GATE", [r.kind for r in rs], ["gate", "gate", "bookkeeping", "gate"])
    c.check("status from good", [r.status for r in rs], ["PASS", "FAIL", "FAIL", "FAIL"])
    table = fd.render_table(rows).splitlines()
    c.check("table header", table[0], "%-58s %s" % ("check", "result"))
    c.check("a PASS shows its evidence", table[1], "%-58s PASS  sha ok" % "ground truth")
    c.check("a FAIL shows its detail", table[2], "%-58s FAIL  rule 7 +1" % "style lint")
    c.check("no note, no trailing spaces", table[3], "%-58s FAIL" % "base recorded")
    c.check("names cut at 58, notes at 80", fd.render_table([("n" * 70, False, "d" * 100, "")], header=False),
            "n" * 58 + " FAIL  " + "d" * 80)
    c.check("compact lines", fd.render_lines(rows).splitlines()[:2], ["PASS ground truth - sha ok", "FAIL style lint - rule 7 +1"])
    c.check("a None detail falls back to the evidence", fd.Row.from_tuple(("x", False, None, "info")).note(), "info")


def test_verdict_json_exit(c):
    v = fd.Verdict.of([("a", True, "", ""), ("b", False, "bad", "", "bookkeeping")])
    c.check("verdict", (v.ok, v.failed_kinds, v.summary), (False, {"bookkeeping"}, "2 row(s): 1 PASS, 1 FAIL"))
    c.check("a Finding is a GATE failure", fd.Verdict.of([fd.Finding(1, "f", 1)]).failed_kinds, {"gate"})
    c.check("an empty verdict is ok", (fd.Verdict().ok, fd.Verdict().summary), (True, "0 row(s)"))
    payload = json.loads(fd.render_json("land", v))
    c.check("the one schema", sorted(payload), ["ok", "rows", "summary", "tool"])
    c.check("rows round-trip", fd.Row(**payload["rows"][1]), v.rows[1])
    c.check("exit codes", [fd.exit_code(x) for x in (v, fd.Verdict(), True, False, None, 3, ValueError("x"),
                                                     SystemExit(1), SystemExit(None), SystemExit("msg"))],
            [1, 0, 0, 1, 0, 3, 2, 1, 0, 2])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
