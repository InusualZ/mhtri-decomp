"""doclinks: every relative markdown link resolves, root-absolute links are flagged, code and URLs are not links."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json

from tools.lib import testing
from tools.agents import doclinks

TIER = "fixture"

PAGE = """# Page

A [good link](other.md), a [section](other.md#part), an [anchor](#here), a [site](https://example.com/x.md),
a [mail](mailto:a@b.c), an ![image](img/pic.png "a title"), a [spaced](<with space.md>), a [percent](with%20space.md).
A [broken](missing.md) one, a [root-absolute](/docs/other.md) one, and [up](../CLAUDE.md).

`[in a span](missing-span.md)` is code, and so is this fence:

```
[in a fence](missing-fence.md)
```

[ref]: missing-ref.md
[okref]: other.md
"""


def tree_with(t):
    t.write("CLAUDE.md", "[docs](docs/page.md)\n")
    t.write("docs/page.md", PAGE)
    t.write("docs/other.md", "# Other\n")
    t.write("docs/with space.md", "x\n")
    t.write("docs/img/pic.png", b"\x89PNG")
    t.write(".claude/agents/a.md", "[bad](../../nowhere.md)\n")
    t.write(".claude/worktrees/agent-x/docs/z.md", "[ignored](gone.md)\n")


def test_links_and_verdicts(c):
    c.check("code spans and fences carry no links",
            [t for _l, t in doclinks.links(PAGE) if "span" in t or "fence" in t], [])
    with testing.FixtureTree() as t:
        tree_with(t)
        res = doclinks.run(str(t.root))
        got = sorted((f["file"], f["line"], f["target"], f["reason"].split(" (")[0]) for f in res["findings"])
        c.check("a missing target, a root-absolute link and a broken reference definition are findings",
                got, [(".claude/agents/a.md", 1, "../../nowhere.md", "does not resolve"),
                      ("docs/page.md", 5, "/docs/other.md", "root-absolute link"),
                      ("docs/page.md", 5, "missing.md", "does not resolve"),
                      ("docs/page.md", 13, "missing-ref.md", "does not resolve")])
        c.check("anchors, URLs, mailto, titles, <spaced> and %20 targets, ../ and images resolve or are skipped",
                res["links"], 15)
        c.check("another checkout under .claude/worktrees is not this repository's docs",
                any("worktrees" in f for f in doclinks.markdown_files(str(t.root))), False)

        def cli(*argv):
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                rc = doclinks.main(["--root", str(t.root), *argv])
            return rc, out.getvalue()
        rc, out = cli()
        c.check("the CLI exits 1 on a broken link and counts them", (rc, "4 broken" in out), (1, True))
        rc, out = cli("docs/other.md", "--json")
        c.check("named files only; a clean set exits 0 with the JSON shape",
                (rc, json.loads(out)["findings"], json.loads(out)["files"]), (0, [], 1))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
