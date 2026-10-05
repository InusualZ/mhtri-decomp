"""The comment vocabulary rule 15 and the comment sweep share: stale-path markers, narrative markers, the stale judgement.
Spec: docs/tools/spec/lib-comments.md. CLI: none (library)."""
from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Callable, Iterable

from tools.lib import cscan

#: The stale-path vocabulary: section 6.5 rule 15's refusing set and `sweepcomments --list-stale`'s census.
#: Hand-curated: the retired tool names are the ones the comments still cite, not derived from docs/tools/retired.md
#: (which names live tools as replacements).
STALE_MARKERS = (
    ("include/", re.compile(r"(?<![\w/.\-#])include/(?![*{])")),
    ("src/auto", re.compile(r"(?<![\w/])src/auto\b")),
    ("auto/<hex>_", re.compile(r"(?<![\w/])auto/[0-9A-Fa-f]{8}_")),
    ("proposal/", re.compile(r"(?<![\w/])proposal/")),
    (".pi/", re.compile(r"(?<![\w/])\.pi/")),
    ("docs/splits/phase4", re.compile(r"docs/splits/phase4(?!/homebutton-carried-notes\.md)")),
    ("retired tool", re.compile(r"\b(?:attribute\.py|applysplits|dataattach|matchinggain|promote\.py|promote_batch|"
                                r"herdr|applybranch|union\.py|mergelane)\b")),
)
#: Files whose paths describe the tree as it was (the census never counts them).
STALE_WHITELIST = ("docs/splits-program.md", "docs/tools/retired.md", "docs/splits/phase4/homebutton-carried-notes.md")
#: Prefixes a path that exists can still never be live under: retired roots, and the gitignored scratch `.pi/`, whose
#: presence differs between MAIN and a worktree (a judgement that read it would differ by checkout).
NEVER_LIVE = ("include/", "proposal/", "auto/", "src/auto", "docs/splits/phase4", ".pi/")

#: The narrative markers (advisory: game text can say "round" or "lane"): `sweepcomments --markers` and rule 15's
#: advisory classes.
HISTORY_MARKERS = (
    ("phase 4", re.compile(r"phase[ -]?4", re.I)),
    ("next pass", re.compile(r"next pass", re.I)),
    ("date", re.compile(r"\b20\d\d-\d\d-\d\d\b")),
    ("round N", re.compile(r"\bround \d+\b", re.I)),
    ("pilot", re.compile(r"\bpilot\b", re.I)),
    ("wave", re.compile(r"\bwave\b", re.I)),
    ("lane", re.compile(r"\blane\b", re.I)),
    ("header inherited", re.compile(r"header inherited from")),
)

_PATH_TOKEN_RE = re.compile(r"[\w.+\-]*(?:/[\w.+\-]*)+")
_TOKEN_OPEN = "`'\"("
_TOKEN_CLOSE = "`'\".,;:()"


@dataclass(frozen=True)
class Hit:
    """One marker occurrence: the marker's name, the at-fault token (the whole path around the hit, or the matched
    text when it is not a path) and the hit's offset in the text."""
    marker: str
    token: str
    pos: int


def token_start(text: str, pos: int) -> int:
    """The start of the path-like run `pos` sits in (word characters and `_.+-/`)."""
    while pos > 0 and (text[pos - 1].isalnum() or text[pos - 1] in "_.+-/"):
        pos -= 1
    return pos


def path_token(text: str, pos: int) -> str:
    """The path token around `pos`, stripped of quoting and trailing punctuation (a leading `.` is part of the path:
    `.pi/`); `""` when the run holds no `/`."""
    tok = _PATH_TOKEN_RE.match(text, token_start(text, pos))
    return tok.group().lstrip(_TOKEN_OPEN).rstrip(_TOKEN_CLOSE) if tok else ""


def is_live(path: str, exists: Callable[[str], bool]) -> bool:
    """Whether a path token names something the tree has: it contains a `/`, `exists(path)` (a trailing `/`
    dropped), and it is not under a `NEVER_LIVE` prefix."""
    return bool(path) and "/" in path and not path.startswith(NEVER_LIVE) and exists(path.rstrip("/"))


def stale_hits(text: str, exists: Callable[[str], bool], markers: Iterable = STALE_MARKERS) -> list[Hit]:
    """Every stale-path marker hit in `text`, per marker in text order, minus the hits whose path the tree has
    (`is_live`). The token is the path around the hit, or the matched text when there is no path (a retired tool
    named bare). One implementation for the census and rule 15."""
    out = []
    for name, rx in markers:
        for m in rx.finditer(text):
            p = path_token(text, m.start())
            if is_live(p, exists):
                continue
            out.append(Hit(name, p if "/" in p else m.group(), m.start()))
    return out


def marker_hits(text: str, markers: Iterable = HISTORY_MARKERS) -> list[Hit]:
    """Every narrative marker hit in `text`, per marker in text order (the token is the matched text)."""
    return [Hit(name, m.group(), m.start()) for name, rx in markers for m in rx.finditer(text)]


def comment_only(text: str) -> str:
    """`text` with everything but its comments blanked (newlines kept, length unchanged): code, string and character
    literals are not comment text."""
    out = list(re.sub(r"[^\n]", " ", text))
    for sp in cscan.spans(text):
        if sp.kind in ("block", "line"):
            out[sp.start:sp.end] = text[sp.start:sp.end]
    return "".join(out)
