"""mt.py permdecl: the alias builds tryvar's argv from the positional form, the spelled-out form and a mix of both.
Mutation: the old `["-u", rest[0], "--permdecl", rest[1]] + rest[2:]` fails the spelled-out and mixed checks."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import importlib.util

from tools.lib import testing

TIER = "smoke"


def load_mt():
    path = testing.LIVE_ROOT / ".claude" / "skills" / "mwcc-unit-matching" / "scripts" / "mt.py"
    spec = importlib.util.spec_from_file_location("mt_under_test", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def test_permdecl_argv(c):
    argv = load_mt().permdecl_argv
    c.check("positional unit and functions", argv(["g3d/x", "f,g"]), ["-u", "g3d/x", "--permdecl", "f,g"])
    c.check("options after the positionals are forwarded, their values kept",
            argv(["g3d/x", "f", "--max-perms", "20", "--plain-only", "--apply"]),
            ["-u", "g3d/x", "--permdecl", "f", "--max-perms", "20", "--plain-only", "--apply"])
    c.check("--unit spelled out: the first positional is the function, no second -u",
            argv(["--unit", "g3d/x", "f"]), ["--permdecl", "f", "--unit", "g3d/x"])
    c.check("-u and --permdecl both spelled out are forwarded untouched",
            argv(["-u", "g3d/x", "--permdecl", "f", "--seed", "3"]), ["-u", "g3d/x", "--permdecl", "f", "--seed", "3"])
    c.check("the = spelling is one token", argv(["--permdecl=f", "g3d/x"]), ["-u", "g3d/x", "--permdecl=f"])
    c.check("a value of a valued option is not taken for the unit", argv(["--flags-extra", "-O3", "g3d/x", "f"]),
            ["-u", "g3d/x", "--permdecl", "f", "--flags-extra", "-O3"])
    c.check("no function at all asks for --help", argv(["g3d/x"]), ["--help"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
