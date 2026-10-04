"""landlog: the CLI over `lib.lanes.landlog` - the summary text, `list`, `--last` and `--json` on a fixture log."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import tempfile

from tools.lib import testing
from tools.lib.lanes import landlog as lib
from tools.units import landlog

TIER = "fixture"


def _run(argv):
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        rc = landlog.main(argv)
    return rc, out.getvalue()


def test_cli(c):
    with tempfile.TemporaryDirectory() as main:
        rc, text = _run(["--main", main])
        c.check("an empty log says so", (rc, "0 attempt(s)" in text), (0, True))
        lib.append(main, lib.Attempt("worker/a-1", "landed", 40.0, commit="abc1234"))
        lib.append(main, lib.Attempt("worker/b-2", "refused", 10.0, refused_row="flipcheck READY"))
        lib.append(main, lib.Attempt("worker/c-3", "refused", 20.0, refused_row="flipcheck READY"))
        rc, text = _run(["summary", "--main", main])
        c.check("the summary names the outcomes and the refusing row",
                ("landed 1, refused 2" in text, "  2  flipcheck READY" in text, "landed ratio 33%" in text),
                (True, True, True))
        rc, text = _run(["list", "--main", main, "--last", "1"])
        c.check("list --last 1 prints the newest attempt only", (text.count("\n"), "worker/c-3" in text), (1, True))
        rc, text = _run(["--main", main, "--json"])
        c.check("--json is the lib's summary plus the unreadable lines",
                (json.loads(text)["outcomes"]["refused"], json.loads(text)["unreadable_lines"]), (2, []))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
