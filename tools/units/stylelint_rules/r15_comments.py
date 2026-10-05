"""Rule 15: comment hygiene - stale paths and wrong function-comment addresses refuse; narrative markers are advisory.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import bisect
import os
import re

from tools.lib import comments as _comments
from tools.lib import cscan
from tools.lib import repo as _repo
from tools.units.stylelint_rules.common import Source, _finding

RULE = 15
#: The sub-checks whose finding is refused when a batch adds it (add-only by identity, like every rule).
REFUSING = ("stale-path", "address")
#: The sub-checks that only count: a `--budget` column, never a `--diff` effect (`diff.judge` drops them).
ADVISORY = tuple(name for name, _rx in _comments.HISTORY_MARKERS) + ("percent", "self-name")
#: The vendor directory keeps the vendor's comments (MPL-1.1); rule 15 never reads it.
EXCLUDED = ("src/Camellia/",)

#: The function-comment address prefix (section 6.5 rule 15's allowed leading form): `/* 0x8XXXXXXX (0xSS): ...`.
#: The canonical separator is `:`; the legacy `-`, `.` and a bare `*/` are read too, so every spelling of the form is
#: held to the same truth. A comment that opens with several addresses (`0x...C/0x...0`, a range `0x...0..0x...C`) or
#: an address and no separator is not the form and is not read.
ADDRESS_PREFIX_RE = re.compile(
    r"/\*+[ \t]*0x(8[0-9A-Fa-f]{7})(?![0-9A-Fa-f])(?:[ \t]*\((0x[0-9A-Fa-f]+|\d+)\))?[ \t]*(?::|-|\.(?!\.)|\*/)")
#: A percentage next to a scoring word, either side, in the same clause (`92.3 % fuzzy`, `match 100%`): a game
#: probability (`a 30 % chance`) has no scoring word near it, and a `,`/`;`/`.` ends the clause.
_SCORE_WORDS = r"(?:match\w*|fuzzy|score\w*|rows?|objdiff)"
_CLAUSE = r"[^\n%,;.]{0,20}?"
PERCENT_RE = re.compile(r"\b%s\b%s\d+(?:\.\d+)?[ \t]*%%|\d+(?:\.\d+)?[ \t]*%%%s\b%s\b"
                        % (_SCORE_WORDS, _CLAUSE, _CLAUSE, _SCORE_WORDS), re.I)
#: A function comment restates its own symbol only when the name is distinctive enough to be the symbol: a short
#: generic member name (`init`, `draw`) is ordinary prose.
SELF_NAME_MIN = 6

# --------------------------------------------------------------------------------------------------
# the tree the stale-path check asks "does this path exist?" of
# --------------------------------------------------------------------------------------------------
_ROOT: "str | None" = None


def set_rule15_context(root: "str | None") -> None:
    """The tree `is_live` reads (None: no path exists, so every marker hit is stale). Both sides of a comparison are
    judged against this one tree, so only a change of comment text can add a finding."""
    global _ROOT
    _ROOT = None if root is None else str(root)


def _exists(path: str) -> bool:
    return _ROOT is not None and os.path.exists(os.path.join(_ROOT, *path.split("/")))


# --------------------------------------------------------------------------------------------------
# the checks
# --------------------------------------------------------------------------------------------------
def _r15(src: Source, line: int, check: str, detail: str, token: str, remedy: "str | None" = None) -> dict:
    f = dict(_finding(src, RULE, line, detail, token=token), check=check, advisory=check not in REFUSING)
    if remedy:
        f["remedy"] = remedy
    return f


def is_advisory(f: dict) -> bool:
    """Whether a finding only counts (a rule-15 advisory class) - the one test `diff.judge` and the reports read."""
    return bool(f.get("advisory"))


def stale_path_findings(src: Source, comments: str) -> list[dict]:
    """A stale path in comment text (`lib.comments.stale_hits`: the hand-curated markers minus a path the tree has).
    The token is the path, so the identity survives a reflow; an `include/X` the move relocated carries the live
    spelling as its remedy (`lib.repo.moved_header`), never as part of the identity."""
    out = []
    for h in _comments.stale_hits(comments, _exists):
        remedy = None
        if h.marker == "include/" and h.token.startswith("include/") and len(h.token) > len("include/"):
            target = _repo.moved_header(h.token.rstrip("/"))
            if target != h.token.rstrip("/") and _exists(target):
                remedy = "the live path is `%s`" % _repo.include_spelling(target)
        out.append(_r15(src, src.line_of(h.pos), "stale-path",
                        "stale path `%s` in a comment (%s) - name the live path or drop the reference"
                        % (h.token, h.marker), h.token, remedy))
    return out


def function_comments(src: Source) -> list[tuple[dict, int, str]]:
    """`(definition, comment_start, comment_text)` for every function definition with a block comment **directly**
    above it: only whitespace between the comment's end and the definition's first line, and no blank line."""
    blocks = [sp for sp in cscan.spans(src.text) if sp.kind == "block"]
    ends = [sp.end for sp in blocks]
    out = []
    for d in cscan.function_declarations(src):
        if d.body is None:
            continue
        first = src._starts[d.start_line - 1]
        j = bisect.bisect_right(ends, first) - 1
        if j < 0:
            continue
        sp = blocks[j]
        gap = src.text[sp.end:first]
        if gap.strip() or gap.count("\n") > 1:
            continue
        out.append((d.to_dict(), sp.start, src.text[sp.start:sp.end]))
    return out


def address_findings(src: Source, ownership, fcomments) -> list[dict]:
    """A function comment's `0xADDR [(SIZE)]` prefix must name the function that follows (`symbols.txt` through
    `ownership.functions`): no function row at the address, the definition's own name at another address, or a size
    that is not the row's is a finding. Without the map (or a map built without its rows) nothing is judged."""
    functions = getattr(ownership, "functions", None) if ownership is not None else None
    if not functions:
        return []
    out = []
    for d, start, text in fcomments:
        m = ADDRESS_PREFIX_RE.match(text)
        if not m:
            continue
        addr = int(m.group(1), 16)
        size = int(m.group(2), 0) if m.group(2) else None
        stated = "0x%08X" % addr + (" (0x%X)" % size if size is not None else "")
        line = src.line_of(start)
        here = functions.get(addr, [])
        name = d["name"]
        if not here:
            out.append(_r15(src, line, "address", "function comment states %s above `%s`: no function symbol starts "
                                                  "there" % (stated, name), name))
            continue
        own = [(n, s) for n, s in here if n == name]
        if not own:
            rows = (ownership.symbols.get(name) or []) if hasattr(ownership, "symbols") else []
            elsewhere = [a for sec, a, t in rows if t == "function"]
            if len(rows) == 1 and elsewhere and elsewhere[0] != addr:
                out.append(_r15(src, line, "address", "function comment states %s above `%s`, which the map has "
                                                      "at 0x%08X" % (stated, name, elsewhere[0]), name))
                continue
        sizes = sorted({s for _n, s in (own or here)})
        if size is not None and size not in sizes:
            out.append(_r15(src, line, "address", "function comment states %s above `%s`: the symbol's size is %s"
                            % (stated, name, "/".join("0x%X" % s for s in sizes)), name))
    return out


def advisory_findings(src: Source, comments: str, fcomments) -> list[dict]:
    """The advisory classes: the narrative markers (`lib.comments.HISTORY_MARKERS`), a percentage beside a scoring
    word, and a function comment that names its own symbol. Counted in `--budget`, never refused."""
    out = [_r15(src, src.line_of(h.pos), h.marker, "narrative marker `%s` in a comment (%s)" % (h.token, h.marker),
                h.token) for h in _comments.marker_hits(comments)]
    for m in PERCENT_RE.finditer(comments):
        out.append(_r15(src, src.line_of(m.start()), "percent", "a matching percentage in a comment", m.group()))
    for d, start, text in fcomments:
        name = d["name"]
        if len(name) >= SELF_NAME_MIN and re.search(r"(?<![\w:])%s(?!\w)" % re.escape(name), text):
            out.append(_r15(src, src.line_of(start), "self-name",
                            "function comment restates its own symbol `%s`" % name, name))
    return out


def findings(src: Source, ownership=None) -> list[dict]:
    """Every rule-15 finding of one file: refusing (stale path, function-comment address) and advisory, each
    carrying `check` (its class) and `advisory`. `src/Camellia/` is excluded (the vendor's comments)."""
    if src.rel.replace("\\", "/").startswith(EXCLUDED):
        return []
    comments = _comments.comment_only(src.text)
    fcomments = function_comments(src)
    return (stale_path_findings(src, comments) + address_findings(src, ownership, fcomments)
            + advisory_findings(src, comments, fcomments))
