"""stylelint rule 8's case-in-block check: a case/default label inside another block of its switch, and its ratchet."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import testing
from tools.units.stylelint_rules import diff, r08_case_label
from tools.units.stylelint_rules.common import Source
from tools.units.stylelint_rules.lint import lint_source

TIER = "fixture"


def _sites(text):
    return [(s["line"], s["label"], s["form"], s["function"]) for s in r08_case_label.sites(Source("x.c", "x.c", text))]


def _rule8(text, rel="src/m/x.c"):
    return [f for f in lint_source(Source("x.c", rel, text)) if f["rule"] == 8]


IF0 = """void step(int s) {
    switch (s) {
    case 0:
        a();
        if (0) {
    case 1:
            b();
        }
        c();
        break;
    }
}
"""

NESTED_CASE_BLOCK = """void step(int s) {
    switch (s) {
    case 0: {
        a();
    case 1:
        b();
        break;
    }
    }
}
"""

DUFF = """void copy(char* to, char* from, int count) {
    int n = (count + 7) / 8;
    switch (count % 8) {
    case 0: do { *to++ = *from++;
    case 7:      *to++ = *from++;
            } while (--n > 0);
    }
}
"""

IN_IF = """void step(int s, int k) {
    switch (s) {
    case 0:
        if (k) {
    default:
            b();
        }
        break;
    }
}
"""

IN_ELSE = """void step(int s, int k) {
    switch (s) {
    case 0:
        if (k) {
            a();
        } else {
    case 2:
            b();
        }
        break;
    }
}
"""

FOR_LOOP = """void step(int s) {
    switch (s) {
    case 0:
        for (;;) {
            a();
            break;
    case 3:
            b();
        }
    }
}
"""

BLOCK_AFTER_LABEL = """void step(int s) {
    switch (s) {
    case 0: {
        int x = a();
        b(x);
        break;
    }
    case 1:
    {
        c();
        break;
    }
    default: {
        break;
    }
    }
}
"""

NESTED_SWITCH = """void step(int s, int t) {
    switch (s) {
    case 0:
        if (t) {
            switch (t) {
            case 1:
                a();
                break;
            default:
                b();
                break;
            }
        }
        break;
    case 1: {
        switch (t) { case 4: c(); break; }
        break;
    }
    }
}
"""

COMMENTED = """void step(int s) {
    switch (s) {
    case 0:
        /* the old shape was `if (0) { case 1: ... }`
        if (0) {
    case 1:
            b();
        }
        */
        // if (0) { case 2: b(); }
        a("if (0) { case 3: }");
        break;
    }
}
"""

QUALIFIED = """void Foo::step(int s) {
    switch (s) {
    case Kind::A: {
        a();
        break;
    }
    case Kind::B:
        if (0) {
    case Kind::C:
            b();
        }
        break;
    }
}
"""


def test_positive_forms(c):
    c.check("`if (0) { case N: }` is a site, named by its function", _sites(IF0), [(6, "case 1", "if (0)", "step")])
    c.check("a case label inside another case's block is a site", _sites(NESTED_CASE_BLOCK),
            [(5, "case 1", "block", "step")])
    c.check("Duff's device: a label inside a `do` loop is a site", _sites(DUFF), [(5, "case 7", "loop", "copy")])
    c.check("a `default` inside an `if` is a site", _sites(IN_IF), [(5, "default", "if", "step")])
    c.check("a label inside an `else` block is a site", _sites(IN_ELSE), [(7, "case 2", "else", "step")])
    c.check("a `for (;;)` loop's parentheses do not hide the loop", _sites(FOR_LOOP), [(7, "case 3", "loop", "step")])
    c.check("a `::` in a case label is not its colon; a member keeps its class", _sites(QUALIFIED),
            [(9, "case Kind::C", "if (0)", "Foo::step")])


def test_negative_forms(c):
    c.check("`case X: { ... }` opens its block after the label", _sites(BLOCK_AFTER_LABEL), [])
    c.check("a nested switch's labels belong to the nested switch", _sites(NESTED_SWITCH), [])
    c.check("a commented-out or quoted `if (0) { case }` is not code", _sites(COMMENTED), [])
    c.check("a file with no switch has no site", _sites("void f(void) { if (0) { a(); } }\n"), [])


def test_lint_finding(c):
    got = _rule8(IF0)
    c.check("the lint reports it as rule 8 with the check, the function as token",
            [(f["line"], f.get("check"), f["token"]) for f in got], [(6, "case-in-block", "step")])
    c.contains("... and the detail names the label and the block", got[0]["detail"], "`case 1:` label inside a nested "
               "`if (0) { ... }` block")
    c.check("MSL_C's goto exemption does not exempt a nested label", len(_rule8(IF0, "src/MSL_C/scanf.c")), 1)
    c.check("a header carries the check too", len(_rule8(IF0, "src/m/x.h")), 1)


def test_ratchet(c):
    base = _rule8(IF0)
    moved = _rule8("/* a header line */\n\n" + IF0)
    c.check("an existing site moved down the file is not an addition", diff.added_identities(base, moved), {})
    grown = _rule8(IF0 + "\n" + IF0.replace("void step(", "void other(").replace("case 1:", "case 9:"))
    added = diff.added_identities(base, grown)
    c.check("a site in another function is an addition", [(f["token"], f["line"]) for fs in added.values() for f in fs],
            [("other", 19)])
    second = _rule8(IF0.replace("        c();\n", "        if (0) {\n    case 2:\n            d();\n        }\n        c();\n"))
    c.check("a second label in the same function is an addition",
            [f["detail"].split("`")[1] for fs in diff.added_identities(base, second).values() for f in fs], ["case 2:"])
    c.check("removing the site adds nothing", diff.added_identities(base, []), {})


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
