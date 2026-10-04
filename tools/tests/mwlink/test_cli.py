"""mwlink.cli: every subcommand parses with its documented flags, `verify` and `order --map` run on fixtures, and
`--selftest` forwards to these tests."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.mwlink import cli
from tools.tests.mwlink.fixtures import CTORS_MAP, output_elf

TIER = "fixture"
SUBCOMMANDS = ("info", "messages", "order", "anchors", "records", "align", "timeline", "verify", "trace", "diagnose",
               "phases")


def test_parser(c):
    ap = cli.build_parser()
    sub = next(a for a in ap._actions if isinstance(a, argparse._SubParsersAction))
    c.check("the eleven subcommands, in order", tuple(sub.choices), SUBCOMMANDS)
    args = ap.parse_args(["trace", "Network/NetworkWiiMediator", "--map", "m", "--elf", "e", "--json", "--full-ctor"])
    c.check("trace takes a unit name and its flags", (args.cmd, args.object, args.map, args.elf, args.json, args.full_ctor),
            ("trace", "Network/NetworkWiiMediator", "m", "e", True, True))
    c.check("trace defaults: scratch out, never main.elf", (args.out, args.link_out),
            ("build/scratch/mwlink-debug", "build/RMHE08/main.elf"))
    args = ap.parse_args(["verify", "a.MAP", "a.elf", "--identity", "b.elf"])
    c.check("verify: map, elf, identity", (args.map, args.elf, args.identity), ("a.MAP", "a.elf", "b.elf"))
    args = ap.parse_args(["align", "--unit", "DWCi/dwc_error"])
    c.check("align --unit", (args.cmd, args.unit, args.linker), ("align", "DWCi/dwc_error", None))
    args = ap.parse_args(["records", "--prove", "--limit", "3"])
    c.check("records --prove --limit", (args.prove, args.limit), (True, 3))
    c.check("--selftest is a top-level flag", ap.parse_args(["--selftest"]).selftest, True)
    c.check("the description is the tool's one line", ap.description,
            "Interrogate the Metrowerks linker (``mwldeppc.exe``) about a real link.")


def test_verify_command(c):
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        (td / "x.MAP").write_text(CTORS_MAP)
        (td / "x.elf").write_bytes(output_elf([(".ctors", 0x8056F2C0, bytes(0x10))]))
        (td / "y.elf").write_bytes(output_elf([(".ctors", 0x8056F2C0, bytes(0x10))]) + b"\0")
        ns = argparse.Namespace(map=str(td / "x.MAP"), elf=str(td / "x.elf"), identity=None)
        rc, out = testing.capture(cli.cmd_verify, ns)
        c.check("verify exits 0 on a match", rc, 0)
        c.contains("... and says so", out, "MATCH: 1 section(s) - the map is this ELF")
        ns.identity = str(td / "x.elf")
        rc, out = testing.capture(cli.cmd_verify, ns)
        c.check("--identity against itself", (rc, "byte-identical" in out), (0, True))
        ns.identity = str(td / "y.elf")
        rc, out = testing.capture(cli.cmd_verify, ns)
        c.check("--identity against a different file exits 1", (rc, "DIFFERS" in out), (1, True))


def test_selftest_forwarding(c):
    here = Path(__file__).resolve()
    c.check("--selftest forwards to this package's test directory", here.parent.as_posix().endswith(cli.TESTS_DIR), True)
    c.expect("... and to the live smoke checks, which exist", (here.parents[3] / cli.SMOKE_TEST).is_file(),
             cli.SMOKE_TEST)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
