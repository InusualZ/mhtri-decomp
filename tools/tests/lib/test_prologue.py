"""Every tool entry point carries exactly the one-line prologue (tools.lib.PROLOGUE) and no other sys.path change.

The pending list (prologue-pending.json, beside this file) names the entry points that predate the rule; it only
shrinks: a file not on it must conform, and a listed file that now conforms must be removed from it.
`--prune` drops listed files that conform or no longer exist; nothing ever adds to the list.
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import ast
import json
import os
from pathlib import Path

from tools.lib import PROLOGUE, testing

TIER = "smoke"
PENDING_FILE = Path(__file__).with_name("prologue-pending.json")
#: dtk-template files (design.md section 2: "template, untouched") and the m2c submodule are not ours.
TEMPLATE = {"tools/project.py", "tools/ninja_syntax.py", "tools/download_tool.py", "tools/transform_dep.py",
            "tools/decompctx.py", "tools/changes_fmt.py"}
SKIP_DIRS = {"__pycache__", "m2c"}


def tool_files(root: Path) -> list[str]:
    """Every `tools/**/*.py` of ours under `root`, as forward-slash paths relative to `root`."""
    out = []
    for dirpath, dirnames, filenames in os.walk(root / "tools"):
        dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS)
        for fn in filenames:
            if fn.endswith(".py"):
                rel = (Path(dirpath) / fn).relative_to(root).as_posix()
                if rel not in TEMPLATE:
                    out.append(rel)
    return sorted(out)


def _is_sys_path(node: ast.AST) -> bool:
    return (isinstance(node, ast.Attribute) and node.attr == "path" and isinstance(node.value, ast.Name)
            and node.value.id == "sys")


def path_mutations(tree: ast.AST) -> list[int]:
    """Line numbers of every `sys.path` mutation: insert/append/extend/remove, item or slice assignment,
    rebinding."""
    lines = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute) and _is_sys_path(node.func.value) \
                and node.func.attr in ("insert", "append", "extend", "remove", "pop", "clear"):
            lines.append(node.lineno)
        elif isinstance(node, (ast.Assign, ast.AugAssign)):
            targets = node.targets if isinstance(node, ast.Assign) else [node.target]
            for t in targets:
                if _is_sys_path(t) or (isinstance(t, ast.Subscript) and _is_sys_path(t.value)):
                    lines.append(node.lineno)
    return sorted(set(lines))


def is_entry_point(rel: str, tree: ast.Module) -> bool:
    """A file run as a program: a module-level `if __name__ == "__main__":`, or a test module."""
    name = rel.rsplit("/", 1)[-1]
    if rel.startswith("tools/tests/") and name.startswith("test_"):
        return True
    for node in tree.body:
        if isinstance(node, ast.If) and isinstance(node.test, ast.Compare) \
                and isinstance(node.test.left, ast.Name) and node.test.left.id == "__name__":
            return True
    return False


def problems(rel: str, text: str) -> list[str]:
    """Why `rel` does not conform (empty when it does)."""
    try:
        tree = ast.parse(text)
    except SyntaxError as exc:
        return ["does not parse: %s" % exc]
    lines = text.splitlines()
    canonical = [i for i, ln in enumerate(lines, 1) if ln == PROLOGUE]
    stray = [n for n in path_mutations(tree) if n not in canonical]
    out = ["a sys.path change that is not the prologue at line %d" % n for n in stray]
    if is_entry_point(rel, tree) and not rel.startswith("tools/lib/"):
        if len(canonical) != 1:
            out.append("an entry point carries the prologue %d times (exactly once)" % len(canonical))
    elif canonical:
        out.append("a library module carries the prologue (only entry points do)")
    return out


def scan(root: Path) -> dict[str, list[str]]:
    """Every non-conforming file under `root` with its problems."""
    found = {}
    for rel in tool_files(root):
        text = (root / rel).read_text(encoding="utf-8", errors="replace")
        p = problems(rel, text)
        if p:
            found[rel] = p
    return found


def load_pending(path: Path = PENDING_FILE) -> list[str]:
    return list(json.loads(path.read_text(encoding="utf-8"))["pending"])


def verdict(found: dict[str, list[str]], pending: list[str], root: Path) -> tuple[list[str], list[str], list[str]]:
    """(new offenders, stale pending entries that now conform, pending entries whose file is gone)."""
    new = sorted(f for f in found if f not in pending or f.startswith("tools/lib/"))
    gone = sorted(p for p in pending if not (root / p).is_file())
    stale = sorted(p for p in pending if (root / p).is_file() and p not in found)
    return new, stale, gone


# --- the rule on fixtures -----------------------------------------------------------------------------------

def test_rule_on_fixtures(c):
    main = 'if __name__ == "__main__":\n    main()\n'
    c.check("an entry point with the prologue conforms", problems("tools/units/a.py", PROLOGUE + "\n" + main), [])
    c.check("an entry point without it does not", len(problems("tools/units/a.py", "import os\n" + main)), 1)
    c.check("a stray insert next to the prologue is named with its line",
            problems("tools/units/a.py", PROLOGUE + "\nimport sys\nsys.path.insert(0, 'x')\n" + main),
            ["a sys.path change that is not the prologue at line 3"])
    c.check("append, slice assignment and rebinding are changes too",
            len(problems("tools/units/m.py", "import sys\nsys.path.append('x')\nsys.path[0:0] = ['y']\nsys.path = []\n")), 3)
    c.check("an indented copy of the line is not the prologue",
            len(problems("tools/units/a.py", "if True:\n    " + PROLOGUE + "\n" + main)), 2)
    c.check("the prologue twice is refused", len(problems("tools/units/a.py", PROLOGUE + "\n" + PROLOGUE + "\n" + main)), 1)
    c.check("a library module with no sys.path change conforms", problems("tools/units/m.py", "import os\n"), [])
    c.check("a library module carrying the prologue does not",
            problems("tools/units/m.py", PROLOGUE + "\n"), ["a library module carries the prologue (only entry points do)"])
    c.check("a lib module never carries it, even with a main block",
            len(problems("tools/lib/x.py", PROLOGUE + "\n" + main)), 1)
    c.check("a test module is an entry point without a main block",
            len(problems("tools/tests/lib/test_x.py", "import os\n")), 1)

    with testing.FixtureTree() as tree:
        tree.write("tools/__init__.py", "")
        tree.write("tools/units/old.py", "import sys\nsys.path.insert(0, 'x')\n" + main)
        tree.write("tools/units/good.py", PROLOGUE + "\n" + main)
        tree.write("tools/units/fixed.py", PROLOGUE + "\n" + main)
        tree.write("tools/m2c/m2c.py", "import sys\nsys.path.insert(0, 'x')\n")
        tree.write("tools/project.py", "import sys\nsys.path.insert(0, 'x')\n" + main)
        found = scan(tree.root)
        c.check("the submodule and the template files are not scanned", sorted(found), ["tools/units/old.py"])
        pending = ["tools/units/old.py", "tools/units/fixed.py", "tools/units/deleted.py"]
        c.check("a pending offender passes; a fixed pending file is stale; a deleted one is noted",
                verdict(found, pending, tree.root), ([], ["tools/units/fixed.py"], ["tools/units/deleted.py"]))
        tree.write("tools/units/new.py", "import sys\nsys.path.insert(0, 'x')\n" + main)
        c.check("a NEW tool with a stray sys.path.insert fails",
                verdict(scan(tree.root), pending, tree.root)[0], ["tools/units/new.py"])
        tree.write("tools/lib/bad.py", "import sys\nsys.path.insert(0, 'x')\n")
        c.check("a lib module can never be pending",
                verdict(scan(tree.root), pending + ["tools/lib/bad.py"], tree.root)[0],
                ["tools/lib/bad.py", "tools/units/new.py"])


# --- the live tree ------------------------------------------------------------------------------------------

def test_live_tree(c):
    root = testing.live_root()
    found = scan(root)
    pending = load_pending()
    new, stale, gone = verdict(found, pending, root)
    for rel in new:
        c.fail("prologue: %s" % rel, "; ".join(found[rel]) + " (use the one line in tools/lib/__init__.py PROLOGUE)")
    for rel in stale:
        c.fail("prologue: %s" % rel, "conforms now - remove it from %s (or run with --prune)" % PENDING_FILE.name)
    if not new and not stale:
        c.expect("no new offender, no stale pending entry", True)
    for rel in gone:
        print("note: pending entry %s no longer exists - `--prune` drops it" % rel)
    print("prologue: %d file(s) still pending migration" % (len(pending) - len(gone) - len(stale)))
    c.check("the pending list is sorted and has no duplicates", pending, sorted(set(pending)))


def prune() -> int:
    root = testing.live_root()
    found, pending = scan(root), load_pending()
    keep = [p for p in pending if (root / p).is_file() and p in found]
    data = json.loads(PENDING_FILE.read_text(encoding="utf-8"))
    data["pending"] = keep
    PENDING_FILE.write_text(json.dumps(data, indent=1) + "\n", encoding="utf-8", newline="\n")
    print("pruned %d entr(ies); %d pending" % (len(pending) - len(keep), len(keep)))
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--prune"]:
        testing.set_tier("smoke")
        raise SystemExit(prune())
    raise SystemExit(testing.run(globals()))
