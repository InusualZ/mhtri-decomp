#!/usr/bin/env python3
"""Comment-only rewriter for the comment sweep: stale paths, narrative history, header range fixes, `#if 0` blocks.
Spec: docs/tools/spec/sweepcomments.md. CLI: sweepcomments.py (--paths | --history | --fixes | --if0 | --list-stale
| --markers) [--apply] [--json] [--root TREE] [FILE...] [--selftest]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import io
import json
import os
import re
import tokenize
from collections import Counter
from dataclasses import dataclass, field

from tools.lib import cli
from tools.lib import comments as _comments
from tools.lib import cscan
from tools.lib import facts as _facts
from tools.lib import repo as _repo
from tools.lib import text as _text
from tools.lib.git import Git
from tools.lib.project import splits as _splits

TOOL = cli.Tool("sweepcomments", "docs/tools/spec/sweepcomments.md", tests="tools/tests/units/test_sweepcomments.py",
                description=(__doc__ or "").splitlines()[0], common=("json", "root"))

# --- the file sets ------------------------------------------------------------------------------------------------

C_SUFFIXES = (".c", ".cp", ".cpp", ".h", ".hpp", ".inc")
#: Never touched: the MPL-1.1 vendor directory (its license block is the vendor's).
C_EXCLUDED = ("src/Camellia/",)
CONFIGURE = "configure.py"
MD_ROOTS = ("docs/", ".claude/")
#: Generated markdown: rewritten by its generator, never by hand (`sync_playbook_index.py`, `sync_reference.py`).
MD_GENERATED = ("docs/matching/index.md", ".claude/skills/mwcc-unit-matching/references/", ".claude/worktrees/")
#: Records of the tree as it was (lane reports, test records, audits, the move's own spec): their paths are history
#: and stay as written. Hand-curated.
MD_RECORDS = ("docs/tooling-requests.md", "docs/agent-profile-tests.md", "docs/pipeline.md", "docs/process-review.md",
              "docs/tools/prose-audit.md", "docs/tools/questions.md", "docs/tools/wp0-report.md",
              "docs/tools/migration.md", "docs/tools/inventory.md", "docs/tools/duplication.md",
              "docs/tools/README.md", "docs/tools/retired.md", "docs/tools/spec/movehdr.md",
              "docs/tools/spec/stylelint_rules.md", "docs/tools/spec/lib-project.md",
              "docs/tools/spec/sweepcomments.md", "docs/tools/spec/factscheck.md", "docs/splits-program.md",
              "docs/splits/")
#: The profile block generated from docs/plan.md section 6.5 (`sync_profiles.py`): edit the source, regenerate.
PROFILE_BLOCK_RE = re.compile(r"<!-- SECTION-6\.5-RULES-BEGIN[\s\S]*?<!-- SECTION-6\.5-RULES-END -->")


def tracked(root: str) -> list[str]:
    out = Git(root).out("ls-files", "-z", check=False)
    return sorted(f for f in out.split("\0") if f)


def c_files(files) -> list[str]:
    return [f for f in files if f.startswith("src/") and f.endswith(C_SUFFIXES) and not f.startswith(C_EXCLUDED)]


def md_files(files) -> list[str]:
    return [f for f in files if f.endswith(".md") and f.startswith(MD_ROOTS)
            and not f.startswith(MD_GENERATED) and not f.startswith(MD_RECORDS)]


def kind_of(rel: str) -> str:
    if rel == CONFIGURE:
        return "py"
    if rel.endswith(".md"):
        return "md"
    return "c"


# --- the text that is protected -----------------------------------------------------------------------------------

#: Comment text other tools read: an edit that would remove any of it is refused (rules 3, 4, 11, 13, the stopgap
#: markers, GUESS, a band header's declaration-line address).
PROTECTED_RE = re.compile(r"size\s*:\s*0x|/\*\s*\+0x|\+0x[0-9A-Fa-f]+\s*\*/|untyped:|free:|STOPGAP|GUESS")


# --- the __LINE__ lock ----------------------------------------------------------------------------------------------

_DEFINE_RE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)")
_LINE_DIRECTIVE_RE = re.compile(r"^[ \t]*#[ \t]*line[ \t]+\d+")


def line_macros(texts) -> set[str]:
    """Every macro whose definition (continuation lines included) expands `__LINE__`."""
    names = set()
    for text in texts:
        code = cscan.strip_comments(text)
        lines = code.split("\n")
        i = 0
        while i < len(lines):
            m = _DEFINE_RE.match(lines[i])
            if m:
                body = [lines[i]]
                while body[-1].rstrip().endswith("\\") and i + 1 < len(lines):
                    i += 1
                    body.append(lines[i])
                if "__LINE__" in "\n".join(body):
                    names.add(m.group(1))
            i += 1
    return names


def locked_lines(text: str, macros: set[str]) -> set[int]:
    """The 1-based lines whose count a `__LINE__` stamp depends on: for every line that expands `__LINE__` (directly
    or through a macro in `macros`), the lines from the governing `#line` directive (or the file's start) through the
    use and two lines past it (a multi-line call)."""
    code = cscan.strip_comments(text)
    lines = code.split("\n")
    use_re = re.compile(r"__LINE__" + "".join(r"|\b%s\s*\(" % re.escape(m) for m in sorted(macros)))
    out: set[int] = set()
    last_directive = 0
    in_define = False
    for n, line in enumerate(lines, 1):
        if _LINE_DIRECTIVE_RE.match(line):
            last_directive = n
        is_define = in_define or bool(_DEFINE_RE.match(line))
        in_define = is_define and line.rstrip().endswith("\\")
        if not is_define and use_re.search(line):
            out.update(range(last_directive + 1, n + 3))
    return out


# --- the comment model ----------------------------------------------------------------------------------------------

@dataclass
class Line:
    head: str
    content: str
    tail: str
    nl: str
    lineno: int | None
    removable: bool
    touched: bool = False


@dataclass
class Group:
    """One comment: a block comment, a run of full-line `//` (or `#`) comments, or a whole markdown file. `start`
    and `end` delimit the region it owns in the file text (whole lines when the comment owns its lines)."""
    kind: str
    start: int
    end: int
    lines: list = field(default_factory=list)
    locked: bool = False

    def logical(self) -> str:
        return "\n".join(ln.content for ln in self.lines)

    def render(self) -> str:
        out = []
        for ln in self.lines:
            head = ln.head.rstrip() if not ln.content and not ln.tail.strip() else ln.head
            out.append(head + ln.content + ln.tail + ln.nl)
        return "".join(out)


def _line_start(text: str, pos: int) -> int:
    return text.rfind("\n", 0, pos) + 1


def _split_marker(raw: str, marker: str) -> tuple[str, str]:
    """`(head, rest)`: the indentation, the marker run and one following space."""
    m = re.match(r"([ \t]*%s+ ?)" % re.escape(marker), raw)
    return (m.group(1), raw[m.end():]) if m else ("", raw)


def _crsplit(s: str) -> tuple[str, str]:
    return (s[:-1], "\r") if s.endswith("\r") else (s, "")


def _block_group(text: str, sp: cscan.Span, line_no: int) -> Group:
    full = text[_line_start(text, sp.start):sp.start].strip() == ""
    start = _line_start(text, sp.start) if full else sp.start
    raw = text[start:sp.end].split("\n")
    lines = []
    for i, r in enumerate(raw):
        last = i == len(raw) - 1
        if i == 0:
            k = r.index("/*")
            m = re.match(r"/\*+ ?", r[k:])
            if r[k:].startswith("/**/"):
                m = re.match(r"/\*", r[k:])
            head, rest = r[:k] + m.group(), r[k + m.end():]
        else:
            m = re.match(r"[ \t]*(?:\*(?!/) ?)?", r)
            head, rest = m.group(), r[m.end():]
        tail = ""
        if last:
            k = rest.rfind("*/")
            if k >= 0:
                body = rest[:k]
                stripped = body.rstrip()
                tail, rest = body[len(stripped):] + rest[k:], stripped
        rest, cr = _crsplit(rest)
        tail = tail + cr if not last else tail
        removable = (i != 0 and not last)
        lines.append(Line(head, rest, tail, "" if last else "\n", line_no + i, removable))
    return Group("block", start, sp.end, lines)


def _run_group(text: str, items: list[tuple[int, int]], marker: str, line_no: int, kind: str) -> Group:
    """A run of `//`/`#` comments (`items` = `(start, end)` offsets, one per line). When the first one owns its line
    the region starts at the line and ends past the last newline, so a dropped line leaves nothing behind."""
    items = [(s, e + 1 if text[e:e + 1] == "\r" else e) for s, e in items]
    first, last_end = items[0][0], items[-1][1]
    full = text[_line_start(text, first):first].strip() == ""
    start = _line_start(text, first) if full else first
    end = last_end + 1 if full and text[last_end:last_end + 1] == "\n" else last_end
    lines = []
    for i, (s, e) in enumerate(items):
        seg_start = _line_start(text, s) if (full or i > 0) else s
        raw = text[seg_start:e]
        head, rest = _split_marker(raw, marker)
        rest, cr = _crsplit(rest)
        nl = "\n" if (i < len(items) - 1 or end > last_end) else ""
        lines.append(Line(head, rest, cr, nl, line_no + i, full))
    return Group(kind, start, end, lines)


def c_groups(text: str) -> list[Group]:
    """The comment groups of a C/C++ text, in order."""
    out: list[Group] = []
    sps = [s for s in cscan.spans(text) if s.kind in ("block", "line")]
    i = 0
    while i < len(sps):
        sp = sps[i]
        line_no = text.count("\n", 0, sp.start) + 1
        if sp.kind == "block":
            out.append(_block_group(text, sp, line_no))
            i += 1
            continue
        items = [(sp.start, sp.end)]
        own = text[_line_start(text, sp.start):sp.start].strip() == ""
        j = i + 1
        while own and j < len(sps) and sps[j].kind == "line":
            nxt = sps[j]
            between = text[items[-1][1]:nxt.start]
            if between.count("\n") != 1 or between.strip():
                break
            items.append((nxt.start, nxt.end))
            j += 1
        out.append(_run_group(text, items, "/", line_no, "line"))
        i = j
    return out


def py_groups(text: str) -> list[Group]:
    """The `#` comment groups of a Python text (tokenize: never a `#` inside a string)."""
    starts = [0]
    for m in re.finditer("\n", text):
        starts.append(m.end())
    toks = []
    try:
        for t in tokenize.generate_tokens(io.StringIO(text).readline):
            if t.type == tokenize.COMMENT:
                s = starts[t.start[0] - 1] + t.start[1]
                e = starts[t.end[0] - 1] + t.end[1]
                toks.append((s, e))
    except (tokenize.TokenError, IndentationError, SyntaxError):
        return []
    out: list[Group] = []
    i = 0
    while i < len(toks):
        items = [toks[i]]
        own = text[_line_start(text, toks[i][0]):toks[i][0]].strip() == ""
        j = i + 1
        while own and j < len(toks):
            between = text[items[-1][1]:toks[j][0]]
            if between.count("\n") != 1 or between.strip():
                break
            items.append(toks[j])
            j += 1
        out.append(_run_group(text, items, "#", text.count("\n", 0, items[0][0]) + 1, "hash"))
        i = j
    return out


def md_groups(text: str) -> list[Group]:
    """A markdown file is one group of whole lines, minus the generated profile block (rewritten by its generator)."""
    out, pos = [], 0
    blocks = [(m.start(), m.end()) for m in PROFILE_BLOCK_RE.finditer(text)] + [(len(text), len(text))]
    for b0, b1 in blocks:
        seg = text[pos:b0]
        if seg:
            raw = seg.split("\n")
            lines = []
            base = text.count("\n", 0, pos) + 1
            for i, r in enumerate(raw):
                rest, cr = _crsplit(r)
                lines.append(Line("", rest, cr, "\n" if i < len(raw) - 1 else "", base + i, False))
            out.append(Group("md", pos, b0, lines))
        pos = b1
    return out


def groups_of(rel: str, text: str) -> list[Group]:
    k = kind_of(rel)
    return py_groups(text) if k == "py" else md_groups(text) if k == "md" else c_groups(text)


# --- applying edits to a group ---------------------------------------------------------------------------------------

@dataclass
class Edit:
    a: int
    b: int
    repl: str
    rule: str


def _locate(g: Group, pos: int) -> tuple[int, int]:
    for i, ln in enumerate(g.lines):
        if pos <= len(ln.content):
            return i, pos
        pos -= len(ln.content) + 1
    return len(g.lines) - 1, len(g.lines[-1].content)


def apply_edits(g: Group, edits: list[Edit], log: Counter, refused: list) -> None:
    """Apply non-overlapping logical-text edits to the group's lines (last first); an edit spanning lines joins the
    first line's head with the last line's tail. An edit that would remove protected text is refused."""
    logical = g.logical()
    kept, last_a = [], None
    for e in sorted(edits, key=lambda e: (e.a, e.b), reverse=True):
        if last_a is not None and e.b > last_a:
            continue
        removed = logical[e.a:e.b]
        if PROTECTED_RE.search(removed) and not PROTECTED_RE.search(e.repl):
            refused.append((e.rule, removed))
            continue
        kept.append(e)
        last_a = e.a
    for e in kept:
        i, ca = _locate(g, e.a)
        j, cb = _locate(g, e.b)
        log[e.rule] += 1
        if i == j:
            ln = g.lines[i]
            ln.content = ln.content[:ca] + e.repl + ln.content[cb:]
            ln.touched = True
            continue
        first, lastl = g.lines[i], g.lines[j]
        first.content = first.content[:ca] + e.repl + lastl.content[cb:]
        first.tail, first.nl = lastl.tail, lastl.nl
        first.removable = first.removable and lastl.removable
        first.touched = True
        gone = j - i
        del g.lines[i + 1:j + 1]
        if g.locked:
            pad_head = _pad_head(g)
            for k in range(gone):
                g.lines.insert(i + 1, Line(pad_head, "", "", first.nl if k == 0 else "\n", None, False, True))
            first.nl = "\n"


def _pad_head(g: Group) -> str:
    heads = [ln.head for ln in g.lines[1:-1]] or [ln.head for ln in g.lines]
    head = Counter(h.rstrip() for h in heads).most_common(1)
    return head[0][0] if head else ""


def cleanup(g: Group) -> int:
    """Drop the lines an edit emptied (a locked group keeps them blank), and the blank line such a drop leaves
    doubled. Returns the number of lines dropped."""
    if g.locked:
        return 0
    dropped = 0
    out: list[Line] = []
    after_drop = False

    def blank(ln: Line) -> bool:
        return not ln.content.strip()

    for ln in g.lines:
        if ln.touched and ln.removable and blank(ln):
            dropped += 1
            after_drop = True
            continue
        if after_drop and blank(ln) and out and blank(out[-1]):
            if ln.removable:
                dropped += 1
                after_drop = False
                continue
            if out[-1].removable:
                out.pop()
                dropped += 1
        out.append(ln)
        after_drop = False
    g.lines = out
    return dropped


# --- rules --------------------------------------------------------------------------------------------------------

def _sentence_edit(text: str, m: re.Match, rule: str, repl: str = "") -> Edit:
    """Delete a sentence with the whitespace that joined it: the spaces before it when it sits mid-line, else the
    spaces after it (the line then empties and is dropped)."""
    a, b = m.start(), m.end()
    if a > 0 and text[a - 1] in " \t":
        while a > 0 and text[a - 1] in " \t":
            a -= 1
    else:
        while b < len(text) and text[b] in " \t":
            b += 1
    return Edit(a, b, repl, rule)


#: The narrative atoms of a parenthetical: dropped by the history pass (and the phase-4 pointers by the path pass).
POINTER_ATOMS = (r"docs/splits/phase4(?:/(?!homebutton)[\w.\-/]*)?", r"window [a-z]")
NARRATIVE_ATOMS = POINTER_ATOMS + (
    r"phase[ -]?4", r"round \d+", r"(?:the )?(?:network )?pilot(?: round \d+)?(?: L\d+)?", r"pilot lane L?\d*",
    r"(?:recon|merger|seam|network|pilot|registration) lane", r"lane [A-Z]\d*", r"wave \d+",
    r"20\d\d-\d\d(?:-\d\d)?")


def _atoms_re(atoms) -> re.Pattern:
    return re.compile(r"^(?:%s)$" % "|".join(atoms), re.I)


_PAREN_RE = re.compile(r"(\s*)\(([^()]{1,200})\)")


def paren_edits(text: str, atoms_re: re.Pattern, rule: str) -> list[Edit]:
    """Drop the narrative atoms of each parenthetical (split on `,`/`;`): the whole parenthetical with the space
    before it when every atom goes, else each dropped atom with the separator that joined it - a deletion of exactly
    those characters, so the atoms that stay (and any line break inside them) are untouched."""
    out = []
    for m in _PAREN_RE.finditer(text):
        if not m.group(1) and m.start() > 0 and (text[m.start() - 1].isalnum() or text[m.start() - 1] == "_"):
            continue                                        # a call, not a parenthetical
        base = m.start(2)
        atoms, seps, pos = [], [], base
        for sm in re.finditer(r"[,;]", m.group(2)):
            atoms.append((pos, base + sm.start()))
            seps.append((base + sm.start(), base + sm.end()))
            pos = base + sm.end()
        atoms.append((pos, m.end(2)))
        keep = [not atoms_re.match(text[a:b].strip()) for a, b in atoms]
        if all(keep):
            continue
        if not any(keep):
            out.append(Edit(m.start(), m.end(), "", rule))
            continue
        first_kept = keep.index(True)
        if first_kept:
            nxt = atoms[first_kept][0]
            while nxt < len(text) and text[nxt] in " \t":
                nxt += 1
            out.append(Edit(atoms[0][0], nxt, "", rule))                 # the leading "atom, atom, " run
        for i in range(first_kept + 1, len(atoms)):
            if not keep[i]:
                out.append(Edit(seps[i - 1][0], atoms[i][1], "", rule))   # ", atom" after a kept atom
    return out


@dataclass
class Context:
    """What the rules read: the tree, its splits, and the record of what was left alone."""
    root: str
    splits: object = None
    dangling: list = field(default_factory=list)
    unmapped: list = field(default_factory=list)
    inherited_kept: list = field(default_factory=list)
    inherited_removed: list = field(default_factory=list)
    corpora: object = None

    def split_index(self):
        if self.splits is None:
            path = os.path.join(self.root, "config", "RMHE08", "splits.txt")
            self.splits = _splits.read(path) if os.path.isfile(path) else _splits.parse("")
        return self.splits

    def unit_at(self, address: int) -> str | None:
        """The registered unit whose `.text` range STARTS at `address` (the scaffolding or proposal unit named by
        that address became it); None when the address now sits inside another unit (a fold or a re-cut: which
        unit the old name meant is a judgement, left for a person)."""
        r = self.split_index().covering(".text", address)
        if r is None or r.start != address or not os.path.isfile(os.path.join(self.root, "src", r.unit)):
            return None
        return r.unit


INCLUDE_RE = re.compile(r"(?<![\w/.\-])include/([A-Za-z0-9_.+\-/]*[A-Za-z0-9_+\-/])(?![*{\w])")
PLAYBOOK_RE = re.compile(r"`?docs/matching\.md`?,?\s+(?:(?:rows?|sections?|ideas?|idea)\s+)?(\d+(?:\s*-\s*\d+)?)"
                         r"(?![\d.]\d)")
PROMOTED_RE = re.compile(r"Promoted from `?(?:src/)?auto/[\w.]+`? \(docs/plan\.md: an auto unit stops being "
                         r"scaffolding\)\.")
FROM_PROVENANCE_RE = re.compile(r",?\s+from\s+`?(?:src/)?(?:auto|proposal)/[\w./]*\w`?")
BARE_REGISTERED_RE = re.compile(r"(?<![\w-])Registered\.(?=\s|$)")
PROVENANCE_PATH_RE = re.compile(r"(?<![\w/])(?:src/)?(?:auto|proposal)/([0-9A-Fa-f]{8})(?:_\w+)?(?:\.(?:cpp|cp|c))?"
                                r"(?![\w/])")


def include_target(root: str, rel: str) -> str | None:
    """The tree path an `include/...` mention names today (`lib.repo.moved_header`), or None when nothing is there."""
    trailing = rel.endswith("/")
    target = _repo.moved_header("include/" + rel.rstrip("/"))
    if os.path.exists(os.path.join(root, *target.split("/"))):
        return target + ("/" if trailing else "")
    return None


def path_rules(ctx: Context, rel: str, kind: str):
    """The path pass, as passes over a group's logical text: each yields edits."""
    def include_pass(text):
        out = []
        for m in INCLUDE_RE.finditer(text):
            target = include_target(ctx.root, m.group(1))
            if target is None:
                ctx.dangling.append((rel, m.group()))
                continue
            new = target if kind == "md" else _repo.include_spelling(target)
            out.append(Edit(m.start(), m.end(), new, "paths/include"))
        return out

    def playbook_pass(text):
        return [Edit(m.start(), m.end(), "playbook " + re.sub(r"\s*-\s*", "-", m.group(1)), "paths/playbook")
                for m in PLAYBOOK_RE.finditer(text)]

    def pointer_pass(text):
        return paren_edits(text, _atoms_re(POINTER_ATOMS), "paths/phase4-pointer")

    def provenance_pass(text):
        out = [_sentence_edit(text, m, "paths/provenance") for m in PROMOTED_RE.finditer(text)]
        taken = [(e.a, e.b) for e in out]
        for m in FROM_PROVENANCE_RE.finditer(text):
            if not any(a <= m.start() < b for a, b in taken):
                out.append(Edit(m.start(), m.end(), "", "paths/provenance"))
        return out

    def bare_registered_pass(text):
        return [_sentence_edit(text, m, "paths/provenance") for m in BARE_REGISTERED_RE.finditer(text)]

    def provenance_path_pass(text):
        out = []
        for m in PROVENANCE_PATH_RE.finditer(text):
            unit = ctx.unit_at(int(m.group(1), 16))
            if unit is None:
                ctx.unmapped.append((rel, m.group()))
                continue
            out.append(Edit(m.start(), m.end(), ("src/" + unit) if kind == "md" else unit, "paths/provenance-unit"))
        return out

    passes = [include_pass, playbook_pass]
    if kind != "md":
        passes += [pointer_pass, provenance_pass, bare_registered_pass, provenance_path_pass]
    return passes


HISTORY_SENTENCES = (
    ("history/phase4", re.compile(r"Phase 4(?: \(docs/splits/phase4\))?: recut registered unit; the functions of the "
                                  r"neighbouring\s+units\s+were\s+cut\s+out\s+of\s+this\s+file\.")),
    ("history/phase4", re.compile(r"Phase 4(?: \(docs/splits/phase4\))?: recut registered unit, built from `[^`]+`\.")),
    ("history/phase4", re.compile(r"PHASE 4(?: \(docs/splits/phase4, window [a-z]\))?\.\s+Recut of [\w./]+: its "
                                  r"functions\s+whose\s+address\s+lies\s+in\s+this\s+range,\s+in\s+address\s+order;"
                                  r"\s+the\s+rest\s+of\s+the\s+range\s+keeps\s+its\s+original\s+bytes\.")),
    ("history/phase4", re.compile(r"PHASE 4(?: FOLD| RECUT)?\.(?=\s|$)")),
    ("history/next-pass", re.compile(r"They are the next pass's work(?: rather than guesses)?\.")),
)
PHASE4_UNIT_RE = re.compile(r"\bphase 4 unit, ")
#: An inherited header block removed by the mechanical pass is at most this many lines: token survival is not insight
#: survival, so a longer block (evidence prose, a load-bearing shape) is stage 3's to merge by hand.
INHERITED_MAX_LINES = 8
INHERITED_RE = re.compile(r"^/\*\s*-{2,}\s*header inherited from (\S+) \(written against its pre-phase-4 range\)\s*"
                          r"-{2,}\s*\*/$")


def history_rules(ctx: Context, rel: str, kind: str):
    def paren_pass(text):
        return paren_edits(text, _atoms_re(NARRATIVE_ATOMS), "history/narrative-paren")

    def sentence_pass(text):
        out = []
        for rule, rx in HISTORY_SENTENCES:
            out += [_sentence_edit(text, m, rule) for m in rx.finditer(text)]
        out += [Edit(m.start(), m.end(), "unit, ", "history/phase4") for m in PHASE4_UNIT_RE.finditer(text)]
        return out

    return [paren_pass, sentence_pass] if kind == "c" else []


# --- one file -----------------------------------------------------------------------------------------------------

@dataclass
class FileResult:
    rel: str
    old: str
    new: str
    rules: Counter
    refused: list
    dropped: int


def _region_lines(text: str, start: int, end: int) -> tuple[int, int]:
    """The whole-line region `[line start, past the newline]` covering `start..end`."""
    a = _line_start(text, start)
    b = text.find("\n", end)
    return a, (len(text) if b < 0 else b + 1)


def inherited_blocks(ctx: Context, rel: str, text: str, groups: list[Group], locked: set[int]) -> list[tuple[int, int, str]]:
    """The `header inherited from` banners and the block each one introduces, as whole-line regions to delete: a
    pair whose removal keeps every fact token somewhere (factscheck's judgement) goes; any other pair stays and is
    listed. A locked pair is blanked (same line count) instead."""
    out = []
    for gi, g in enumerate(groups):
        if g.kind != "block":
            continue
        banner_text = text[g.start:g.end].strip()
        if not INHERITED_RE.match(banner_text):
            continue
        if gi + 1 >= len(groups) or groups[gi + 1].kind != "block":
            continue
        nxt = groups[gi + 1]
        if text[g.end:nxt.start].strip() or text[g.end:nxt.start].count("\n") > 1:
            continue
        a, b = _region_lines(text, g.start, g.end)
        _a2, b2 = _region_lines(text, nxt.start, nxt.end)
        if text[a:g.start].strip() or text[nxt.end:b2].strip():
            continue
        removed = text[a:b2]
        line = text.count("\n", 0, a) + 1
        if removed.count("\n") - text[a:b].count("\n") > INHERITED_MAX_LINES:
            ctx.inherited_kept.append((rel, line, 0, ["longer than %d lines" % INHERITED_MAX_LINES]))
            continue
        new_text = text[:a] + text[b2:]
        if ctx.corpora is None:
            ctx.corpora = _facts.Corpora(_facts.Tree(ctx.root, None))
        lost = _facts.unmatched(removed, new_text, ctx.corpora)
        if lost:
            ctx.inherited_kept.append((rel, line, len(lost), lost[:6]))
            continue
        first, last = line, line + removed.count("\n") - 1
        if any(n in locked for n in range(first, last + 1)):
            out.append((a, b2, "\n" * removed.count("\n")))
        else:
            out.append((a, b2, ""))
        ctx.inherited_removed.append((rel, line, removed.count("\n")))
    return out


def sweep_text(ctx: Context, rel: str, text: str, op: str, macros: set[str]) -> FileResult:
    kind = kind_of(rel)
    rules = Counter()
    refused: list = []
    if op == "paths":
        rule_set = path_rules(ctx, rel, kind)
    elif op == "history":
        rule_set = history_rules(ctx, rel, kind)
    else:
        raise ValueError(op)
    locked = locked_lines(text, macros) if kind == "c" else set()
    groups = groups_of(rel, text)
    replacements: list[tuple[int, int, str]] = []
    dropped = 0
    if op == "history" and kind == "c":
        replacements += inherited_blocks(ctx, rel, text, groups, locked)
        for a, b, repl in replacements:
            rules["history/inherited-header"] += 1
            if not repl:
                dropped += text.count("\n", a, b)
    taken = [(a, b) for a, b, _r in replacements]
    for g in groups:
        if any(a <= g.start < b for a, b in taken):
            continue
        g.locked = any(ln.lineno in locked for ln in g.lines)
        before = g.render()
        for rule_pass in rule_set:
            edits = rule_pass(g.logical())
            if edits:
                apply_edits(g, edits, rules, refused)
        dropped += cleanup(g)
        after = g.render()
        if after != before:
            assert text[g.start:g.end] == before, "group model does not reproduce %s:%d" % (rel, g.lines[0].lineno or 0)
            replacements.append((g.start, g.end, after))
    new = text
    for a, b, repl in sorted(replacements, reverse=True):
        new = new[:a] + repl + new[b:]
    return FileResult(rel, text, new, rules, refused, dropped)


# --- the fixes: header ranges against splits.txt --------------------------------------------------------------------

#: The five headers whose `.text` range contradicts splits.txt (the design's census at 8cd5ee491): three stale
#: ranges, and two that state a sub-range (a key function, a band) without saying so. Each fix names the unit's range,
#: which `--fixes` re-checks against splits.txt before it writes - a fix that no longer agrees with the tree is refused.
#: The function counts and sizes are the map's (`symbols.txt`, `.text` functions in the range).
RANGE_FIXES = (
    ("src/ef/ef_util.cpp", "ef/ef_util.cpp",
     "the 16 functions at `.text` 0x8009B374..0x8009CD64 (6640 B) of the discovery\n * proposal `8009B374_fn_8009B374`.",
     "the 17 functions at `.text` 0x8009B374..0x8009CDBC (6728 B): the discovery proposal\n"
     " * `8009B374_fn_8009B374`'s 16 (..0x8009CD64) and the 88-byte tail 0x8009CD64..0x8009CDBC.",
     (0x8009B374, 0x8009CDBC)),
    ("src/ef/eft053.cpp", "ef/eft053.cpp", "`.text` 0x80366618..0x8036A690 (15 functions)",
     "`.text` 0x80366618..0x8036A828 (18 functions)", (0x80366618, 0x8036A828)),
    ("src/quest/arenatask.cpp", "quest/arenatask.cpp", "`.text` 0x804459E4..0x80448404 (19 functions, 10784 B;",
     "`.text` 0x804459DC..0x80448404 (20 functions, 10792 B;", (0x804459DC, 0x80448404)),
    ("src/Network/NetworkSessionManagerPat.cpp", "Network/NetworkSessionManagerPat.cpp",
     " * Network/NetworkSessionManagerPat.cpp - `NetworkSessionManagerPat`'s key function, i.e. the TU in\n"
     " * which the class's vtable is emitted (`.text` 0x803D70B8..0x803D72F4, 572 B).",
     " * Network/NetworkSessionManagerPat.cpp - `.text` 0x803D70B8..0x803DF2EC (106 functions); its first function\n"
     " * (0x803D70B8..0x803D72F4, 572 B) is `NetworkSessionManagerPat`'s key function, so the class's vtable is emitted here.",
     (0x803D70B8, 0x803DF2EC)),
    ("src/Network/network_pat_control.cpp", "Network/network_pat_control.cpp",
     " * Sections: .text 0x80429B94..0x8043065C, extab",
     " * Sections (the unit's own are its splits.txt block, `.text` 0x80423E74..0x80432104; these are the pat-control\n"
     " * band's): .text 0x80429B94..0x8043065C, extab",
     (0x80423E74, 0x80432104)),
)


def plan_fixes(ctx: Context) -> tuple[list[FileResult], list[str]]:
    out, refusals = [], []
    sp = ctx.split_index()
    for rel, unit, old, new, (lo, hi) in RANGE_FIXES:
        path = os.path.join(ctx.root, *rel.split("/"))
        if not os.path.isfile(path):
            refusals.append("%s: missing" % rel)
            continue
        claims = {r.section: (r.start, r.end) for r in sp.claims(unit)}
        if claims.get(".text") != (lo, hi):
            refusals.append("%s: splits.txt .text is %s, the fix says 0x%08X..0x%08X" % (rel, claims.get(".text"), lo, hi))
            continue
        text = _read(path)
        n = text.count(old)
        if n == 0 and text.count(new) == 1:
            continue                                        # already fixed
        if n != 1:
            refusals.append("%s: the old header text occurs %d times" % (rel, n))
            continue
        out.append(FileResult(rel, text, text.replace(old, new), Counter({"fixes/range": 1}), [], 0))
    return out, refusals


# --- #if 0 blocks ---------------------------------------------------------------------------------------------------

_IF_RE = re.compile(r"^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")


def _comment_above(text: str, line: int) -> int:
    """The 0-based first line of the full-line block comment that ends on the line just above `line` (the comment
    that describes the block), else `line` itself."""
    if line == 0:
        return line
    starts = [0] + [m.end() for m in re.finditer("\n", text)]
    above_start, above_end = starts[line - 1], starts[line] - 1
    for sp in cscan.spans(text):
        if sp.kind != "block" or not (above_start <= sp.end - 1 <= above_end):
            continue
        first = text.count("\n", 0, sp.start)
        if text[starts[first]:sp.start].strip() or text[sp.end:above_end].strip():
            return line
        return first
    return line


def if0_text(rel: str, text: str, macros: set[str]) -> FileResult:
    """Delete every `#if 0 ... #endif` block (nested conditionals followed; a block with an `#else`/`#elif` arm is
    left alone and reported), with the full-line block comment directly above it that describes it."""
    lines = text.split("\n")
    code_lines = cscan.strip_comments(text).split("\n")
    locked = locked_lines(text, macros)
    kill: list[tuple[int, int]] = []
    refused = []
    i = 0
    while i < len(code_lines):
        m = _IF_RE.match(code_lines[i])
        if m and m.group(1) == "if" and m.group(2).strip() == "0":
            depth, j, has_else = 1, i + 1, False
            while j < len(code_lines) and depth:
                mj = _IF_RE.match(code_lines[j])
                if mj:
                    if mj.group(1) in ("if", "ifdef", "ifndef"):
                        depth += 1
                    elif mj.group(1) == "endif":
                        depth -= 1
                    elif depth == 1:
                        has_else = True
                j += 1
            if depth == 0 and not has_else:
                kill.append((_comment_above(text, i), j))
            else:
                refused.append(("if0", "line %d" % (i + 1)))
            i = j
            continue
        i += 1
    out = list(lines)
    dropped = 0
    for a, b in reversed(kill):
        if any((n + 1) in locked for n in range(a, b)):
            out[a:b] = [""] * (b - a)
        else:
            del out[a:b]
            dropped += b - a
            if 0 < a < len(out) and not out[a - 1].strip() and not out[a].strip():
                del out[a]                                  # the blank line the deletion doubled
                dropped += 1
    return FileResult(rel, text, "\n".join(out), Counter({"if0/block": len(kill)}), refused, dropped)


# --- stale paths and narrative markers: the census -----------------------------------------------------------------

#: The stale-path vocabulary, the narrative markers and the stale judgement are `lib.comments`' (section 6.5 rule 15
#: reads the same data): one copy, re-exported under the names this tool always had.
STALE_MARKERS = _comments.STALE_MARKERS
STALE_WHITELIST = _comments.STALE_WHITELIST
HISTORY_MARKERS = _comments.HISTORY_MARKERS


def comment_text(rel: str, text: str) -> str:
    """The comment text of a file (literals and code blanked for C, `#` comments for configure.py, all of a doc)."""
    k = kind_of(rel)
    if k == "c":
        return cscan.strip(text)[1]
    if k == "py":
        keep = [" "] * len(text)
        for g in py_groups(text):
            for i in range(g.start, g.end):
                keep[i] = text[i]
        return "".join(keep)
    return text


def census(root: str, scope: str, markers, whitelist_paths: bool) -> dict:
    files = tracked(root)
    sel = c_files(files) + ([CONFIGURE] if CONFIGURE in files else [])
    if scope == "all":
        sel += [f for f in files if f.endswith(".md") and f.startswith(MD_ROOTS + ("CLAUDE.md",))
                and not f.startswith(MD_GENERATED)] + [f for f in files if f in ("CLAUDE.md", "README.md")]
    totals, per_file = Counter(), {}

    def exists(path: str) -> bool:
        return os.path.exists(os.path.join(root, *path.split("/")))

    for rel in sel:
        if rel in STALE_WHITELIST:
            continue
        text = _read(os.path.join(root, *rel.split("/")))
        if text is None:
            continue
        com = comment_text(rel, text)
        hits = (_comments.stale_hits(com, exists, markers) if whitelist_paths
                else _comments.marker_hits(com, markers))
        for name, n in Counter(h.marker for h in hits).items():
            totals[name] += n
            per_file.setdefault(name, Counter())[rel] = n
    return {"scope": scope, "totals": dict(totals),
            "files": {k: dict(v.most_common()) for k, v in per_file.items()}}


# --- the driver ---------------------------------------------------------------------------------------------------

def _read(path: str) -> str | None:
    try:
        with open(path, "rb") as fh:
            return fh.read().decode("utf-8", "surrogateescape")
    except OSError:
        return None


def select(root: str, op: str, only=()) -> list[str]:
    files = tracked(root)
    if op == "paths":
        sel = c_files(files) + ([CONFIGURE] if CONFIGURE in files else []) + md_files(files)
    else:
        sel = c_files(files)
    if only:
        want = {o.replace("\\", "/") for o in only}
        sel = [f for f in sel if f in want]
    return sel


def run(root: str, op: str, apply: bool = False, only=()) -> dict:
    ctx = Context(root)
    files = tracked(root)
    macros = line_macros(_read(os.path.join(root, *f.split("/"))) or "" for f in c_files(files))
    results: list[FileResult] = []
    refusals: list[str] = []
    if op in ("paths", "history"):
        for rel in select(root, op, only):
            text = _read(os.path.join(root, *rel.split("/")))
            if text is None:
                continue
            r = sweep_text(ctx, rel, text, op, macros)
            if r.new != r.old or r.refused:
                results.append(r)
    elif op == "fixes":
        results, refusals = plan_fixes(ctx)
    elif op == "if0":
        for rel in select(root, "if0", only):
            text = _read(os.path.join(root, *rel.split("/")))
            if text and re.search(r"^[ \t]*#[ \t]*if[ \t]+0\b", text, re.M):
                r = if0_text(rel, text, macros)
                if r.new != r.old:
                    results.append(r)
    changed = [r for r in results if r.new != r.old]
    if apply:
        for r in changed:
            _text.atomic_write(os.path.join(root, *r.rel.split("/")), r.new.encode("utf-8", "surrogateescape"))
    rules = Counter()
    for r in results:
        rules.update(r.rules)
    rewritten = 0
    for r in changed:
        old_lines, new_lines = r.old.split("\n"), r.new.split("\n")
        rewritten += sum(1 for ln in set(old_lines) - set(new_lines))
    return {"op": op, "applied": apply, "files_changed": len(changed),
            "lines_removed": sum(r.old.count("\n") - r.new.count("\n") for r in changed),
            "lines_rewritten_or_removed": rewritten, "rules": dict(rules),
            "files": [r.rel for r in changed],
            "refused_protected": [(r.rel, rule, txt[:80]) for r in results for rule, txt in r.refused],
            "dangling_include": ctx.dangling, "unmapped_provenance": ctx.unmapped,
            "inherited_kept": ctx.inherited_kept, "inherited_removed": ctx.inherited_removed,
            "refusals": refusals}


def main(args) -> int:
    root = _repo.worktree_root(args.root)
    ops = [o for o in ("paths", "history", "fixes", "if0") if getattr(args, o)]
    if args.list_stale or args.markers:
        markers = STALE_MARKERS if args.list_stale else HISTORY_MARKERS
        res = census(root, args.scope, markers, whitelist_paths=bool(args.list_stale))
        if args.json:
            print(json.dumps(res, indent=1))
        else:
            for name, _rx in markers:
                n = res["totals"].get(name, 0)
                top = list(res["files"].get(name, {}).items())[:args.top]
                print("%-20s %5d in %d file(s)%s" % (name, n, len(res["files"].get(name, {})),
                                                     ("  top: " + ", ".join("%s %d" % kv for kv in top)) if top else ""))
            print("total %d (%s)" % (sum(res["totals"].values()), res["scope"]))
        return 0
    if len(ops) != 1:
        print("sweepcomments: name exactly one of --paths, --history, --fixes, --if0, --list-stale, --markers",
              file=sys.stderr)
        return 2
    res = run(root, ops[0], apply=args.apply, only=args.files)
    if args.json:
        print(json.dumps(res, indent=1))
    else:
        print("sweepcomments --%s: %s %d file(s), %d line(s) removed, %d line(s) rewritten or removed"
              % (res["op"], "changed" if res["applied"] else "would change", res["files_changed"],
                 res["lines_removed"], res["lines_rewritten_or_removed"]))
        for k, v in sorted(res["rules"].items()):
            print("  %-32s %d" % (k, v))
        for key in ("refused_protected", "dangling_include", "unmapped_provenance", "inherited_kept", "refusals"):
            if res[key]:
                print("%s: %d" % (key, len(res[key])))
                for item in res[key][:args.top]:
                    print("    %s" % (item,))
    return 1 if res["refusals"] else 0


def build_parser():
    ap = TOOL.parser()
    for op, what in (("paths", "rewrite stale paths (include/, docs/matching.md N, provenance, phase-4 pointers)"),
                     ("history", "drop narrative boilerplate (phase 4, round/pilot/lane parentheticals, dates, "
                                 "inherited headers whose facts survive)"),
                     ("fixes", "correct the header ranges that contradict splits.txt"),
                     ("if0", "delete `#if 0` blocks")):
        ap.add_argument("--" + op, action="store_true", help=what)
    ap.add_argument("--list-stale", action="store_true", help="count the stale-path markers that remain")
    ap.add_argument("--markers", action="store_true", help="count the narrative markers that remain (advisory)")
    ap.add_argument("--scope", choices=("src", "all"), default="src", help="census scope (default: src + configure.py)")
    ap.add_argument("--top", type=int, default=20, help="how many files/items to name per line")
    ap.add_argument("--apply", action="store_true", help="write the changes (default: report only)")
    ap.add_argument("files", nargs="*", help="limit the pass to these tracked files")
    return ap


if __name__ == "__main__":
    raise SystemExit(TOOL.run(main, parser=build_parser()))
