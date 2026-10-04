"""declclash: one case per rule and per normalisation step, on a hand-built include web (rename, retype, cycle)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.units import declclash as dc

TIER = "fixture"

# A root file that includes a small web of headers: two declare the same name with the same shape (a parameter
# rename), two with a genuinely different shape, a `struct`/`void`-spelling pair, a name that only exists in the
# generated include tree, a two-line prototype that must be ignored, and an include cycle. The real acceptance
# case - the 30-name clash set of `src/menu/fn_802E4978.cpp`, 25 headers - is in the commit that added the tool.
FIXTURES = {
    "root.cpp": '#include "root.h"\n\nvoid use(void) {\n    dup(1);\n}\n',
    "include/root.h": (
        '#include "a.h"\n#include "b.h"\n#include "cyc1.h"\n#include "multi.h"\n#include "shadow.h"\n'
        '#include "stmt.h"\n'
    ),
    "include/a.h": (
        "void dup(s32 a);\n"
        "void changed(s32 x);\n"
        "void both(s32 a);\n"
        "void same(void);\n"
        "void strct(struct Big* p);\n"
    ),
    "include/b.h": (
        "void dup(s32 b);\n"
        "void changed(u32 x);\n"
        "void both(u32 b);\n"
        "void same();\n"
        "void strct(Big* q);\n"
    ),
    "include/cyc1.h": '#include "cyc2.h"\nvoid cyc_one(void);\n',
    "include/cyc2.h": '#include "cyc1.h"\nvoid cyc_two(void);\n',
    "include/multi.h": "void multi(\n    s32 a);\n",
    "include/stmt.h": "void real_call(void);\n\nvoid user(void) {\n    return hidden(a, b);\n}\n",
    "include/shadow.h": "void shadowed(s32 a);\n",
    "build/RMHE08/include/shadow.h": "void shadowed(u32 a);\n",
}


def build_tree(root: Path) -> tuple[str, list[str]]:
    for rel, text in FIXTURES.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8", newline="\n")
    roots = [str(root / "include"), str(root / "build" / "RMHE08" / "include")]
    return str(root / "root.cpp"), roots


def quiet_main(argv):
    buffer = io.StringIO()
    with contextlib.redirect_stdout(buffer):
        code = dc.main(argv)
    return code, buffer.getvalue()


def test_report(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        start, roots = build_tree(root)
        findings = dc.report([start], roots)
        by_name = {f["name"]: f for f in findings}

        c.check("a parameter rename is SAME", by_name["dup"]["kind"], "SAME")
        c.check("a changed parameter type is DIFFERENT", by_name["changed"]["kind"], "DIFFERENT")
        c.check("a changed parameter type, two params", by_name["both"]["kind"], "DIFFERENT")
        c.check("`(void)` and `()` are SAME", by_name["same"]["kind"], "SAME")
        c.check("`struct X*` and `X*` are SAME", by_name["strct"]["kind"], "SAME")
        c.check("the two-line prototype is ignored", "multi" in by_name, False)
        c.check("the cycle does not duplicate a file", by_name["dup"]["shapes"][0]["files"],
                [str(root / "include" / "a.h"), str(root / "include" / "b.h")])
        c.check("a call in a definition is not a declaration", "use" in by_name, False)
        c.check("a `return fn(...);` line is not a declaration", "hidden" in by_name, False)
        c.check("`real_call` is seen once", "real_call" in by_name, False)
        c.check("the closure stops at the cycle", len(dc.closure(start, roots)), 9)
        c.check("the source tree wins over the generated one", dc.resolve("shadow.h", roots),
                str(root / "include" / "shadow.h"))
        c.check("a name only declared once is not a finding", "shadowed" in by_name, False)
        c.check("cycle-only names are not findings", "cyc_one" in by_name, False)

        only_different = dc.report([start], roots, only_different=True)
        c.check("--only-different hides SAME", [f["name"] for f in only_different], ["both", "changed"])

        code, out = quiet_main([str(start), "--root", str(root), "--json"])
        c.check("--json exits 0", code, 0)
        c.check("--json carries every finding", len(json.loads(out)["findings"]), len(findings))

        code, out = quiet_main([str(start), "--root", str(root), "--fail-on-different"])
        c.check("--fail-on-different exits 1 on a DIFFERENT name", code, 1)
        c.contains("the text report summarises the split", out,
                   "5 name(s) with more than one declaration: 2 DIFFERENT, 3 SAME")

        empty = root / "clean.cpp"
        empty.write_text("void lonely(void);\n", encoding="utf-8")
        code, _out = quiet_main([str(empty), "--root", str(root), "--fail-on-different"])
        c.check("--fail-on-different exits 0 with nothing clashing", code, 0)

        code, out = quiet_main([str(root / "missing.cpp"), "--root", str(root)])
        c.check("a missing file is reported, not fatal", code, 0)
        c.contains("the missing-file report says so", out, "no such file")


def test_shape(c):
    c.check("shape() drops the parameter name", dc.shape("s32 fn(const u16* p)"), "s32 fn(const u16*)")
    c.check("shape() folds void to empty", dc.shape("void fn(void)"), "void fn()")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
