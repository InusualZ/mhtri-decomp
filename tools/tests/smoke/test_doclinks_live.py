"""doclinks on the live tree: every relative markdown link in docs/**, CLAUDE.md and .claude/** resolves, none is
root-absolute. A finding names the file, line and target; fix the link (or the file it points at)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import testing
from tools.agents import doclinks

TIER = "smoke"


def test_live_docs_links_resolve(c):
    root = str(testing.live_root())
    res = doclinks.run(root)
    if not res["files"]:
        c.skip("no markdown files", "this checkout carries no docs")
        return
    c.check("every relative markdown link resolves (doclinks.py lists them)",
            ["%s:%d %s - %s" % (f["file"], f["line"], f["target"], f["reason"]) for f in res["findings"]], [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
