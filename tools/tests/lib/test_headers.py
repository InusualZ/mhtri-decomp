"""The in-code header template (design.md section 9): a tool's module docstring is at most three lines and names
its spec, docs/tools/spec/<name>.md (lib-<name>.md for tools/lib). ADVISORY until WP6: the live count is printed and
never fails; the rule itself is checked on fixtures and does fail.
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import ast
import re
from pathlib import Path

from tools.lib import testing
from tools.tests.lib.test_prologue import tool_files

TIER = "smoke"
MAX_LINES = 3
SPEC_RE = re.compile(r"Spec:\s*(docs/tools/spec/[\w.-]+\.md)")


def expected_spec(rel: str) -> str:
    """`docs/tools/spec/<stem>.md`; `lib-<stem>.md` for a lib module, `lib-<package>.md` for a module of a lib
    package (`tools/lib/binary/elf.py` -> `lib-binary.md`: one spec per package, design.md section 3)."""
    parts = rel.split("/")
    if rel.startswith("tools/lib/") and len(parts) > 3:
        return "docs/tools/spec/lib-%s.md" % parts[2]
    stem = parts[-1][:-3]
    return "docs/tools/spec/%s%s.md" % ("lib-" if rel.startswith("tools/lib/") else "", stem)


def in_scope(rel: str) -> bool:
    """A tool or lib module: not a package marker, not a test, not an old-style selftest (re-homed later)."""
    name = rel.rsplit("/", 1)[-1]
    return not (name == "__init__.py" or rel.startswith("tools/tests/") or name.endswith("_selftest.py")
                or rel.startswith("tools/selftest_site/"))


def header_problems(rel: str, text: str, spec_exists=lambda path: True) -> list[str]:
    """Why `rel`'s module docstring does not follow the template (empty when it does)."""
    try:
        doc = ast.get_docstring(ast.parse(text), clean=False)
    except SyntaxError as exc:
        return ["does not parse: %s" % exc]
    if not doc:
        return ["no module docstring"]
    out = []
    lines = doc.strip("\n").splitlines()
    if len(lines) > MAX_LINES:
        out.append("module docstring is %d lines (at most %d)" % (len(lines), MAX_LINES))
    m = SPEC_RE.search(doc)
    want = expected_spec(rel)
    if not m:
        out.append("no `Spec: %s`" % want)
    elif m.group(1) != want:
        out.append("names %s, expected %s" % (m.group(1), want))
    elif not spec_exists(want):
        out.append("%s does not exist" % want)
    return out


def test_rule_on_fixtures(c):
    good = '"""Does a thing. Spec: docs/tools/spec/flipcheck.md. CLI: flipcheck.py [unit..]."""\n'
    c.check("the template conforms", header_problems("tools/units/flipcheck.py", good), [])
    c.check("three lines are allowed",
            header_problems("tools/units/flipcheck.py", '"""A.\nSpec: docs/tools/spec/flipcheck.md.\nCLI: x."""\n'), [])
    c.check("four lines are refused",
            header_problems("tools/units/flipcheck.py", '"""A.\nB.\nSpec: docs/tools/spec/flipcheck.md.\nCLI: x."""\n'),
            ["module docstring is 4 lines (at most 3)"])
    c.check("a missing spec path is refused", header_problems("tools/units/flipcheck.py", '"""Does a thing."""\n'),
            ["no `Spec: docs/tools/spec/flipcheck.md`"])
    c.check("another tool's spec is refused", header_problems("tools/units/datagap.py", good),
            ["names docs/tools/spec/flipcheck.md, expected docs/tools/spec/datagap.md"])
    c.check("a lib module names lib-<name>.md",
            header_problems("tools/lib/testing.py", '"""H. Spec: docs/tools/spec/lib-testing.md. CLI: none."""\n'), [])
    c.check("a lib package's module names the package's spec",
            header_problems("tools/lib/binary/elf.py", '"""H. Spec: docs/tools/spec/lib-binary.md. CLI: none."""\n'), [])
    c.check("... not its own stem",
            header_problems("tools/lib/binary/elf.py", '"""H. Spec: docs/tools/spec/lib-elf.md. CLI: none."""\n'),
            ["names docs/tools/spec/lib-elf.md, expected docs/tools/spec/lib-binary.md"])
    c.check("a spec that does not exist is refused",
            header_problems("tools/units/flipcheck.py", good, spec_exists=lambda p: False),
            ["docs/tools/spec/flipcheck.md does not exist"])
    c.check("no docstring is refused", header_problems("tools/units/x.py", "import os\n"), ["no module docstring"])
    c.check("tests, selftests, package markers are out of scope",
            [in_scope(p) for p in ("tools/tests/lib/test_x.py", "tools/units/x_selftest.py", "tools/lib/__init__.py",
                                   "tools/units/x.py")], [False, False, False, True])


def test_live_tree_advisory(c):
    root = testing.live_root()
    rows = [(rel, header_problems(rel, (root / rel).read_text(encoding="utf-8", errors="replace"),
                                  lambda p: (root / p).is_file()))
            for rel in tool_files(root) if in_scope(rel)]
    bad = [(rel, p) for rel, p in rows if p]
    print("advisory: %d of %d tool modules follow the header template (refusing from WP6)"
          % (len(rows) - len(bad), len(rows)))
    if "--verbose" in sys.argv[1:]:
        for rel, p in bad:
            print("  %s: %s" % (rel, "; ".join(p)))
    for rel in ("tools/lib/testing.py",):
        if (root / rel).is_file():
            c.check("the WP0 module %s follows the template" % rel, dict(rows).get(rel), [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
