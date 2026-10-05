"""lib.comments: the stale-path judgement, the marker hits and the comment-only view rule 15 and the sweep share."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import comments, testing

TIER = "fixture"


def test_stale_hits(c):
    none = lambda _p: False
    text = "see include/Net/net.h, `.pi/notes/x.md` and mergelane; tools/units/herdr/README."
    c.check("each stale marker hit, its token the whole path (quoting and trailing punctuation stripped)",
            [(h.marker, h.token) for h in comments.stale_hits(text, none)],
            [("include/", "include/Net/net.h"), (".pi/", ".pi/notes/x.md"), ("retired tool", "mergelane"),
             ("retired tool", "tools/units/herdr/README")])
    live = lambda p: p == "tools/units/herdr/README"
    c.check("a path the tree has is live and not reported",
            [h.token for h in comments.stale_hits(text, live)], ["include/Net/net.h", ".pi/notes/x.md", "mergelane"])
    every = lambda _p: True
    c.check("... but a NEVER_LIVE prefix is stale even where it exists (`.pi/` differs by checkout)",
            [h.token for h in comments.stale_hits(text, every)], ["include/Net/net.h", ".pi/notes/x.md", "mergelane"])
    c.check("the homebutton notes path is not a phase-4 pointer",
            comments.stale_hits("docs/splits/phase4/homebutton-carried-notes.md", none), [])
    c.check("a nested `include/` directory is not the retired root", comments.stale_hits("src/sdk/include/x.h", none), [])


def test_marker_hits_and_comment_only(c):
    c.check("narrative markers, in marker order",
            [(h.marker, h.token) for h in comments.marker_hits("round 3 of the pilot lane, 2026-10-05")],
            [("date", "2026-10-05"), ("round N", "round 3"), ("pilot", "pilot"), ("lane", "lane")])
    src = 'int lane; /* the lane */ const char* s = "pilot"; // wave\n'
    view = comments.comment_only(src)
    c.check("comment_only keeps the length and newlines", (len(view), view.count("\n")), (len(src), 1))
    c.check("... and only comment text: code and literals are blanked",
            [h.token for h in comments.marker_hits(view)], ["wave", "lane"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
