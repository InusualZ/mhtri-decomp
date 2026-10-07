"""seed_essentials / `slots.py seed-worktree`: the toolchain, `_vmx`, the DOL and the m2c files are copied into a plain
worktree; the build tree is not; MAIN is never written; the m2c `.git` pointer stays behind.
Mutation: copying m2c's `.git` fails the pointer check, copying the whole build fails the build/RMHE08 check."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os

from tools.lib import testing
from tools.lib.lanes import seed

TIER = "fixture"


def tree_of(root):
    return sorted(os.path.relpath(os.path.join(d, f), root).replace(os.sep, "/")
                  for d, _dirs, fs in os.walk(root) for f in fs)


def test_seed_essentials(c):
    with testing.FixtureTree() as main, testing.FixtureTree() as wt:
        for rel in ("build/tools/dtk.exe", "build/compilers/Wii/1.3/mwcceppc.exe", "build/binutils/objdump.exe",
                    "build/_vmx/table.bin", "build/RMHE08/main.elf", "build/RMHE08/obj/a/b.o", "build/RMHE08/config.json",
                    "orig/RMHE08/sys/main.dol", "orig/RMHE08/files/mh3.sel", "orig/RMHE08/files/readme.txt",
                    "tools/m2c/m2c.py", "tools/m2c/m2c/x.py", "tools/m2c/.git"):
            main.write(rel, rel)
        before = tree_of(str(main.root))
        wt.write("keep.txt", "k")
        wt.write("build/tools/dtk.exe", "mine")
        out = seed.seed_essentials(str(main.root), str(wt.root))
        got = tree_of(str(wt.root))
        c.check("the toolchain, _vmx, the DOL, the selfile and m2c arrive",
                [g for g in got if g != "keep.txt" and not g.startswith("config/") and g != "configure.py"],
                ["build/_vmx/table.bin", "build/binutils/objdump.exe", "build/compilers/Wii/1.3/mwcceppc.exe",
                 "build/tools/dtk.exe", "orig/RMHE08/files/mh3.sel", "orig/RMHE08/sys/main.dol",
                 "tools/m2c/m2c.py", "tools/m2c/m2c/x.py"])
        c.check("the build tree and the split objects stay behind, m2c's .git pointer too",
                [g for g in got if g.startswith("build/RMHE08") or g.endswith("m2c/.git")], [])
        c.check("a file already there is kept", (wt.root / "build/tools/dtk.exe").read_text(), "mine")
        c.check("counts per item", (out["build/tools"], out["build/_vmx"], out["orig/RMHE08"], out["tools/m2c"], out["missing"]),
                (0, 1, 2, 2, []))
        c.check("MAIN is untouched", tree_of(str(main.root)), before)
        again = seed.seed_essentials(str(main.root), str(wt.root))
        c.check("a second run copies nothing", [v for k, v in again.items() if k != "missing"], [0, 0, 0, 0, 0, 0])
        with_obj = seed.seed_essentials(str(main.root), str(wt.root), with_obj=True)
        c.check("--obj adds the split objects and config",
                sorted(g for g in tree_of(str(wt.root)) if g.startswith("build/RMHE08")),
                ["build/RMHE08/config.json", "build/RMHE08/obj/a/b.o"])
        c.check("... and names what MAIN lacks", with_obj["missing"], ["build/RMHE08/ldscript.lcf"])
        c.raises("MAIN is never its own target", ValueError, seed.seed_essentials, str(main.root), str(main.root))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
