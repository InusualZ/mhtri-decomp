"""unitinfo: the unit listing and one unit's resolution, on a fixture tree with a stub ninja."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import subprocess

from tools.lib import testing
from tools.units import unitinfo

TIER = "fixture"

COMMAND = ("build/tools/sjiswrap.exe build/compilers/Wii/1.3/mwcceppc.exe -O4,p -inline auto "
           "-MMD -c src/Dir/file.c -o build/RMHE08/src/Dir")


def _tree(tree):
    tree.add_unit("Dir/file.c")
    tree.add_unit("Dir/other.cpp")
    tree.write("src/top.cpp", "int x;\n")


def test_list(c):
    with testing.FixtureTree() as tree:
        _tree(tree)
        lines = unitinfo.list_lines(str(tree.root))
        c.check("one count line, then every unit with source, nested and top-level",
                [ln.split()[0] for ln in lines[1:]], ["main/Dir/file", "main/Dir/other", "main/top"])
        c.check("... the count names them", lines[0], "3 unit(s) with source in this repo:")
        c.contains("... each with its source path", lines[1].replace("\\", "/"), "src/Dir/file.c")


def test_info(c):
    seen = []

    def ninja(argv, **kwargs):
        seen.append(list(argv))
        return subprocess.CompletedProcess(argv, 0, COMMAND + "\n", "")

    with testing.FixtureTree() as tree:
        _tree(tree)
        lines = unitinfo.info_lines("file", str(tree.root), runner=ninja)
        rows = dict(ln.split(None, 1) for ln in lines)
        c.check("a bare stem resolves to the unit", rows["unit"], "main/Dir/file")
        c.check("... with its source, object and split target", [rows[k].replace("\\", "/") for k in ("src", "obj", "target")],
                ["src/Dir/file.c", "build/RMHE08/src/Dir/file.o", "build/RMHE08/obj/Dir/file.o"])
        c.check("the ninja command is split into compiler, flags and tail",
                (rows["compiler"].split()[-1], rows["flags"], rows["tail"].split()[:2]),
                ("build/compilers/Wii/1.3/mwcceppc.exe", "-O4,p -inline auto", ["-MMD", "-c"]))
        c.check("... asked of the unit's own ninja target", seen[0][-1], "build/RMHE08/src/Dir/file.o")
        c.raises("an unknown unit is refused", SystemExit, unitinfo.info_lines, "nope", str(tree.root), ninja)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
