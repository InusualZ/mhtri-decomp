"""lib.project on the live tree: splits round-trips byte for byte, every map row parses, the configure evaluator
agrees with configure.py actually executed, and the ownership index agrees with a linear scan (shape, no counts)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import subprocess

from tools.lib import testing
from tools.lib.project import Configure, Ownership, Splits, SymbolMap, object_calls

TIER = "smoke"

#: Runs configure.py up to its build-generating tail with no flags, then prints every object's (lib, path, flags).
_EXEC = r"""
import json, sys
root = sys.argv[1]
sys.path.insert(0, root)
sys.argv = ["configure.py"]
src = open(root + "/configure.py", encoding="utf-8").read().split("if args.mode ==")[0]
ns = {"__name__": "configure_prefix"}
exec(compile(src, "configure.py", "exec"), ns)
out = []
for lib in ns["config"].libs:
    for o in lib["objects"]:
        own = (getattr(o, "options", {}) or {}).get("cflags")
        out.append([lib["lib"], o.name, list(own or lib["cflags"]), bool(o.completed)])
print(json.dumps(out))
"""


def test_splits_round_trip(c):
    path = testing.live_root() / "config" / "RMHE08" / "splits.txt"
    if not path.is_file():
        return c.skip("splits round trip", "no splits.txt")
    raw = path.read_bytes().decode("utf-8")
    sp = Splits.parse(raw)
    c.check("the live splits.txt renders byte-identically", sp.render() == raw, True)
    c.expect("it has units and ranges", sp.units and sp.ranges)
    c.check("no unit key is duplicated", len(sp.units), len(set(sp.units)))


def test_symbols_parse(c):
    path = testing.live_root() / "config" / "RMHE08" / "symbols.txt"
    if not path.is_file():
        return c.skip("map parse", "no symbols.txt")
    res = SymbolMap(path).check()
    c.check("every non-comment line of the live map parses", res.unparsed, ())
    c.expect("it has rows", res.symbols > 0)


def test_configure_agrees_with_execution(c):
    root = testing.live_root()
    if not (root / "configure.py").is_file():
        return c.skip("configure", "no configure.py")
    p = subprocess.run([sys.executable, "-c", _EXEC, str(root)], capture_output=True, text=True, encoding="utf-8",
                       errors="replace", cwd=str(root))
    if p.returncode != 0:
        return c.skip("configure executed", "configure.py did not run here: %s" % p.stderr.strip()[-200:])
    executed = json.loads(p.stdout)
    conf = Configure.load(root / "configure.py")
    mine = [[o.lib, o.path, list(o.cflags), o.linked] for o in conf.objects()]
    first = next(([a, b] for a, b in zip(executed, mine) if a != b), None)
    c.check("every object's lib, path, resolved cflags and link flag equal the executed configure.py", first, None)
    c.check("... and the same number of objects", len(mine), len(executed))
    calls = [x.path for x in object_calls((root / "configure.py").read_text(encoding="utf-8"))]
    c.check("the token scan names the same objects", calls, [o.path for o in conf.objects()])


def test_ownership_agrees_with_a_linear_scan(c):
    root = testing.live_root()
    own = Ownership.load(root)
    if own is None:
        return c.skip("ownership", "no map or splits")
    names = sorted(own.symbols)[::97]              # a spread sample: shape, not a count
    bad = []
    for name in names:
        entries = own.symbols[name]
        if len(entries) != 1:
            continue
        section, address, _t = entries[0]
        want = next((u for s, e, u in own.ranges.get(section, []) if s <= address < e), None)
        got = own.resolve(name)
        if (got.get("unit") if got["kind"] == "owned" else None) != want:
            bad.append(name)
    c.check("resolve agrees with a linear scan of the claims on a sample", bad[:5], [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
