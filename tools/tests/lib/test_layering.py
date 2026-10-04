"""The layering rule: tools/lib never imports a tool, and a tool imports another tool only on the allow-list.

layering-allow.json (beside this file) is the tool->tool import graph as it stood when the rule arrived; it only
shrinks: a new edge fails, and a listed edge whose two files exist but no longer import fails as stale. Tests under
tools/tests/ may import anything. `--prune` drops stale and deleted edges; nothing ever adds one.
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import ast
import json
from pathlib import Path

from tools.lib import cli, testing
from tools.tests.lib.test_prologue import tool_files

TIER = "smoke"
ALLOW_FILE = Path(__file__).with_name("layering-allow.json")
STDLIB = set(sys.stdlib_module_names)


def imported_names(tree: ast.AST) -> list[tuple[str, int]]:
    """Every statically named import as (dotted name, relative level): `import a.b`, `from a import b`
    (as `a.b`), `importlib.import_module("a")` and `__import__("a")` with a constant string."""
    out = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            out += [(a.name, 0) for a in node.names]
        elif isinstance(node, ast.ImportFrom):
            base = node.module or ""
            out.append((base, node.level))
            out += [((base + "." if base else "") + a.name, node.level) for a in node.names if a.name != "*"]
        elif isinstance(node, ast.Call) and node.args and isinstance(node.args[0], ast.Constant) \
                and isinstance(node.args[0].value, str):
            f = node.func
            name = f.attr if isinstance(f, ast.Attribute) else f.id if isinstance(f, ast.Name) else ""
            if name in ("import_module", "__import__"):
                out.append((node.args[0].value, 0))
    return out


class Resolver:
    """Maps an import in a tools file to the tools file it loads, the way the tools' sys.path inserts do:
    `tools.x.y` by path; a bare name from the importer's own directory first, then `tools/`, then the one
    `tools/*/` directory that has it (never for a standard-library name)."""

    def __init__(self, files: list[str]) -> None:
        self.files = set(files)
        self.by_stem: dict[str, list[str]] = {}
        for f in files:
            stem = f.rsplit("/", 1)[-1][:-3]
            if stem != "__init__":
                self.by_stem.setdefault(stem, []).append(f)

    def _module(self, dotted_path: str) -> str | None:
        for cand in (dotted_path + ".py", dotted_path + "/__init__.py"):
            if cand in self.files:
                return cand
        return None

    def resolve(self, importer: str, name: str, level: int) -> str | None:
        here = importer.rsplit("/", 1)[0]
        if level:
            base = here
            for _ in range(level - 1):
                base = base.rsplit("/", 1)[0]
            return self._module(base + ("/" + name.replace(".", "/") if name else "")) if name else None
        parts = name.split(".")
        if parts[0] == "tools":
            return self._module("/".join(parts)) if len(parts) > 1 else None
        first = parts[0]
        local = self._module(here + "/" + "/".join(parts))
        if local:
            return local
        top = self._module("tools/" + "/".join(parts))
        if top:
            return top
        if first in STDLIB or len(parts) > 1:
            return None
        cands = sorted(c for c in self.by_stem.get(first, []) if c.count("/") == 2)
        return cands[0] if cands else None


#: The tool package a file belongs to (`lib.cli.package_of`): an import inside one package is not an edge.
package_of = cli.package_of


def edges(root: Path) -> tuple[set[tuple[str, str]], list[tuple[str, str]]]:
    """(tool->tool edges, lib->non-lib edges) of the tree under `root`."""
    files = tool_files(root)
    res = Resolver(files)
    file_set = set(files)
    tool_edges, lib_bad = set(), []
    for f in files:
        if f.startswith("tools/tests/"):
            continue
        try:
            tree = ast.parse((root / f).read_text(encoding="utf-8", errors="replace"))
        except SyntaxError:
            continue
        for name, level in imported_names(tree):
            target = res.resolve(f, name, level)
            if target is None or target == f or target.endswith("/__init__.py") and target == "tools/__init__.py":
                continue
            if f.startswith("tools/lib/"):
                if not target.startswith("tools/lib/"):
                    lib_bad.append((f, target))
                continue
            if target.startswith("tools/lib/"):
                continue
            pkg = package_of(f, file_set)
            if pkg is not None and pkg == package_of(target, file_set):
                continue
            tool_edges.add((f, target))
    return tool_edges, sorted(set(lib_bad))


def key(edge: tuple[str, str]) -> str:
    return "%s -> %s" % edge


def load_allow(path: Path = ALLOW_FILE) -> list[str]:
    return list(json.loads(path.read_text(encoding="utf-8"))["edges"])


def verdict(found: set[tuple[str, str]], allow: list[str], root: Path) -> tuple[list[str], list[str], list[str]]:
    """(new edges, stale allowed edges, allowed edges with a deleted file)."""
    have = {key(e) for e in found}
    new = sorted(have - set(allow))
    gone, stale = [], []
    for k in allow:
        a, b = k.split(" -> ")
        if not ((root / a).is_file() and (root / b).is_file()):
            gone.append(k)
        elif k not in have:
            stale.append(k)
    return new, sorted(stale), sorted(gone)


# --- the rule on fixtures -----------------------------------------------------------------------------------

def test_rule_on_fixtures(c):
    with testing.FixtureTree() as tree:
        w = tree.write
        w("tools/__init__.py", "")
        w("tools/lib/__init__.py", "")
        w("tools/lib/good.py", "import os\nfrom tools.lib import other\nfrom . import other as o2\n")
        w("tools/lib/other.py", "import json\n")
        w("tools/unitutil.py", "import os\n")
        w("tools/units/queue.py", "import os\n")
        w("tools/units/claims.py", "import queue\nimport unitutil\nfrom tools.lib import good\n")
        w("tools/units/slots.py", "import queue\n")
        w("tools/objdiff/symdiff.py", "import queue\nimport json\nimport elfsect\n")
        w("tools/elf/elfsect.py", "import struct\n")
        w("tools/tests/units/test_claims.py", "from tools.units import claims\n")
        found, lib_bad = edges(tree.root)
        c.check("lib->lib, stdlib and test imports are not edges; a bare name resolves like the sys.path inserts",
                sorted(key(e) for e in found),
                ["tools/objdiff/symdiff.py -> tools/elf/elfsect.py",
                 "tools/units/claims.py -> tools/units/queue.py",
                 "tools/units/claims.py -> tools/unitutil.py",
                 "tools/units/slots.py -> tools/units/queue.py"])
        c.check("a clean lib has no outward edge", lib_bad, [])
        allow = sorted(key(e) for e in found) + ["tools/units/gone.py -> tools/unitutil.py",
                                                 "tools/units/slots.py -> tools/unitutil.py"]
        c.check("the current graph passes; a removed import is stale; a deleted file is noted",
                verdict(found, allow, tree.root),
                ([], ["tools/units/slots.py -> tools/unitutil.py"], ["tools/units/gone.py -> tools/unitutil.py"]))
        w("tools/units/slots.py", "import queue\nimport claims\n")
        c.check("a NEW tool->tool edge fails", verdict(edges(tree.root)[0], allow, tree.root)[0],
                ["tools/units/slots.py -> tools/units/claims.py"])
        w("tools/units/slots.py", "import queue\n\ndef f():\n    import importlib\n    importlib.import_module('claims')\n")
        c.check("... also when it is imported lazily by name", verdict(edges(tree.root)[0], allow, tree.root)[0],
                ["tools/units/slots.py -> tools/units/claims.py"])
        w("tools/lib/bad.py", "import unitutil\nfrom tools.units import claims\n")
        c.check("a lib->tools import is always a finding", edges(tree.root)[1],
                [("tools/lib/bad.py", "tools/units/claims.py"), ("tools/lib/bad.py", "tools/unitutil.py")])
        w("tools/pkg/__init__.py", "")
        w("tools/pkg/a.py", "from tools.pkg import b\nfrom . import c\n")
        w("tools/pkg/b.py", "import os\n")
        w("tools/pkg/c.py", "from tools.units import queue\n")
        w("tools/pkgshim.py", "from tools.pkg import a\n")
        found = {key(e) for e in edges(tree.root)[0]}
        c.check("an import inside one tool package (a directory with __init__.py) is not an edge",
                sorted(k for k in found if k.startswith("tools/pkg/") and "/pkg/" in k.split(" -> ")[1]), [])
        c.contains("... while the package importing another tool is", found, "tools/pkg/c.py -> tools/units/queue.py")
        c.contains("... and so is a shim importing the package", found, "tools/pkgshim.py -> tools/pkg/a.py")
        w("tools/units/nopkg.py", "import queue\n")
        c.contains("a directory without __init__.py is not a package: its imports stay edges",
                   {key(e) for e in edges(tree.root)[0]}, "tools/units/nopkg.py -> tools/units/queue.py")


# --- the live tree ------------------------------------------------------------------------------------------

def test_live_tree(c):
    root = testing.live_root()
    found, lib_bad = edges(root)
    for a, b in lib_bad:
        c.fail("layering: %s" % a, "tools/lib imports the tool %s (lib never imports tools)" % b)
    if not lib_bad:
        c.expect("tools/lib imports no tool", True)
    allow = load_allow()
    new, stale, gone = verdict(found, allow, root)
    for k in new:
        c.fail("layering: %s" % k, "a new tool->tool import (move the shared code into tools/lib)")
    for k in stale:
        c.fail("layering: %s" % k, "no longer imported - remove it from %s (or run with --prune)" % ALLOW_FILE.name)
    if not new and not stale:
        c.expect("no new tool->tool edge, no stale allowed edge", True)
    for k in gone:
        print("note: allowed edge %s names a deleted file - `--prune` drops it" % k)
    print("layering: %d tool->tool edge(s) allowed pending migration" % (len(allow) - len(gone) - len(stale)))
    c.check("the allow-list is sorted and has no duplicates", allow, sorted(set(allow)))


def prune() -> int:
    root = testing.live_root()
    have = {key(e) for e in edges(root)[0]}
    allow = load_allow()
    keep = [k for k in allow if k in have]
    data = json.loads(ALLOW_FILE.read_text(encoding="utf-8"))
    data["edges"] = keep
    testing.rewrite_json(ALLOW_FILE, data)   # the file's own indent: a prune is a minimal diff
    print("pruned %d edge(s); %d allowed" % (len(allow) - len(keep), len(keep)))
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--prune"]:
        testing.set_tier("smoke")
        raise SystemExit(prune())
    raise SystemExit(testing.run(globals()))
