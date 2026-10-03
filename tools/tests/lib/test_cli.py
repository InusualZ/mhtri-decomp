"""lib.cli: the common flags, the exit policy, JSON on --json, selftest forwarding and the discovery regex."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import tempfile

from tools.lib import cli, findings, testing

TIER = "fixture"


def _run(tool, main, argv):
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        rc = tool.run(main, argv)
    return rc, out.getvalue(), err.getvalue()


def test_parser(c):
    tool = cli.Tool("demo", "docs/tools/spec/demo.md")
    args = tool.parser().parse_args(["--json", "--root", "R", "--main", "M", "--limit", "3", "--dry-run", "--quiet"])
    c.check("every common flag parses", (args.json, args.root, args.main, args.limit, args.dry_run, args.quiet),
            (True, "R", "M", 3, True, True))
    d = tool.parser().parse_args([])
    c.check("defaults", (d.json, d.root, d.main, d.limit, d.dry_run, d.quiet), (False, ".", None, None, False, False))
    c.check("no --selftest without tests", hasattr(d, "selftest"), False)
    some = cli.Tool("demo", common=("json",)).parser().parse_args([])
    c.check("a subset of the common flags", sorted(vars(some)), ["json"])
    c.raises("an unknown common flag is refused", ValueError, cli.Tool, "demo", common=("bogus",))


def test_run_exit_policy(c):
    tool = cli.Tool("demo", common=("json",))
    c.check("None is 0", _run(tool, lambda a: None, [])[0], 0)
    c.check("an int passes through", _run(tool, lambda a: 3, [])[0], 3)
    bad = findings.Verdict.of([("row", False, "bad", "")])
    rc, out, _ = _run(tool, lambda a: bad, ["--json"])
    c.check("a failed Verdict is 1 and prints the one schema on --json", (rc, json.loads(out)["tool"], json.loads(out)["ok"]),
            (1, "demo", False))
    c.check("... and prints nothing without --json", _run(tool, lambda a: bad, [])[1], "")

    def boom(a):
        raise RuntimeError("no build tree")
    rc, _, err = _run(tool, boom, [])
    c.check("an exception is 'could not run' (2) and names the tool", (rc, "demo: could not run: no build tree" in err),
            (2, True))


def test_selftest_forwarding(c):
    calls = []
    tool = cli.Tool("demo", tests=lambda: calls.append(1) or 0, common=())
    c.check("--selftest forwards to a callable", (_run(tool, lambda a: 5, ["--selftest"])[0], calls), (0, [1]))
    c.check("without --selftest main runs", _run(tool, lambda a: 5, [])[0], 5)
    with tempfile.TemporaryDirectory() as tmp:
        script = os.path.join(tmp, "t.py")
        with open(script, "w", encoding="utf-8") as fh:
            fh.write("raise SystemExit(4)\n")
        c.check("--selftest runs a path with this interpreter", cli.Tool("demo", tests=script).selftest(cwd=tmp), 4)


def test_discovery(c):
    rx = cli.SELFTEST_FLAG
    c.check("discovery sees the old flag and a Tool with tests",
            [bool(rx.search(s)) for s in ('ap.add_argument("--selftest", action="store_true")',
                                          'TOOL = cli.Tool("x", "spec", tests="tools/units/x_selftest.py")',
                                          'TOOL = cli.Tool("x", "spec")', "# mentions --selftest in prose")],
            [True, True, False, False])
    tool = cli.Tool("x", tests=lambda: 0)
    c.check("a Tool with tests registers --selftest in its parser", tool.parser().parse_args(["--selftest"]).selftest,
            True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
