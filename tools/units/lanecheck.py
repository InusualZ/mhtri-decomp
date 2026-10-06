#!/usr/bin/env python3
"""The mechanical pre-review of a lane's branch: owners by address, wrong callees, empty stubs, flip blockers, GUESS names.
Spec: docs/tools/spec/lanecheck.md. CLI: lanecheck.py [--branch B] [--base REF] [--no-flipcheck] [--no-dump] [--strict]
[--json] [--root TREE]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import json
import os
import re
import time
from dataclasses import dataclass

from tools.lib import cli
from tools.lib import comments as _comments
from tools.lib import cscan
from tools.lib import dumpsyms
from tools.lib import facts as _facts
from tools.lib import findings as _findings
from tools.lib import names as _names
from tools.lib import objcompare
from tools.lib import proc as _proc
from tools.lib import repo as _repo
from tools.lib.git import Git
from tools.lib.project import configure as _configure
from tools.lib.project.ownership import SPLITS_REL, SYMBOLS_REL, Ownership

TOOL = cli.Tool("lanecheck", "docs/tools/spec/lanecheck.md", tests="tools/tests/units/test_lanecheck.py",
                description=(__doc__ or "").splitlines()[0], common=("json", "root"))

#: The finding classes, in report order.
CLASSES = ("owner-by-address", "stale-path", "unowned-claim", "wrong-callee", "empty-stub", "flip-blocker", "guess")
SOURCE_SUFFIXES = (".c", ".cp", ".cpp")
HEADER_SUFFIXES = (".h", ".hpp")
#: A unit path cited in comment text: at least one `/`, a C/C++ source or header suffix, `src/` optional.
PATH_RE = re.compile(r"(?<![\w/.\-])(?:src/)?[A-Za-z_][\w+\-]*(?:/[\w+\-.]+)*/[\w+\-]+\.(?:cpp|cp|c|hpp|h)\b")
#: The symbols a comment cites: a generated stem, or any backticked identifier the map knows.
STEM_RE = re.compile(r"\b(?:fn|lbl)_[0-9A-Fa-f]{8}\b")
TICKED_RE = re.compile(r"`([A-Za-z_][\w$@]*)`")
ADDRESS_RE = re.compile(r"\b0x(8[0-9A-Fa-f]{7})\b")
#: An ownership claim's verb. Passive (`<symbol> ... owned by <path>`): the subject is the last symbol before it, the
#: owner the first path after it. Active (`<path> owns <symbol>`): the other way round. Nothing else pairs a symbol
#: with a path - `its own view`, `declared in` and a bare mention are not ownership claims.
PASSIVE_RE = re.compile(r"\b(?:owned by|owner(?: is)?:?|defined (?:by|in)|lives in|belongs to)\s", re.I)
ACTIVE_RE = re.compile(r"\b(?:owns|defines)\s", re.I)
#: The end of a clause: a claim's subject and verb (and an "unowned" phrase and its token) never straddle one.
CLAUSE_BREAK_RE = re.compile(r"[;:]|\.\s")
#: A comment line that claims nobody owns something.
UNOWNED_RE = re.compile(r"\b(?:unowned|unclaimed|no registered (?:owner|unit)|nobody owns|has no owner)\b", re.I)
#: The header label lines of the unit template (`docs/plan.md` section 6.5).
LABEL_RE = re.compile(r"^\s*\*?\s*([A-Z][A-Z0-9 ]{2,})\.\s", re.M)
#: Words that say a residual function is unwritten (an empty body is then what the header promises).
UNWRITTEN_RE = re.compile(r"\b(?:unwritten|not written|stub|empty|placeholder|0 ?%|zero|no body|body-less)\b", re.I)
#: A retail function this small is `blr` (or `li r3,0; blr`): its empty body is the real body.
TINY_FUNCTION = 8
#: The flipcheck problem classes this check reads (byte differences of a partial unit are not blockers to name).
FLIP_UNDEFINED_RE = re.compile(r"defined by nothing a flip can use - (.+?) - our object")
FLIP_ROW36_RE = re.compile(r"^row 36: \d+ function\(s\) force-active in retail \.comment, not in ours: (.+?) - ")
FLIP_NOT_EMITTED_RE = re.compile(r"^splits\.txt claims (\S+) \(0x[0-9A-F]+\) but the object emits no such section")
FLIP_SIZE_RE = re.compile(r"^(\S+): object is 0x[0-9A-F]+, splits\.txt claims 0x[0-9A-F]+")
FLIP_UNCLAIMED_RE = re.compile(r"^(\S+) \(0x[0-9A-F]+\) is in the object but not claimed by splits\.txt")
#: Size gaps in these sections follow the code residuals the header already names function by function.
CODE_SECTIONS = (".text", "extab", "extabindex")
FLIPCHECK = os.path.join(os.path.dirname(os.path.abspath(__file__)), "flipcheck.py")


@dataclass
class Item:
    """One finding: `file:line | class | what | hint`."""
    file: str
    line: int
    cls: str
    what: str
    hint: str
    token: str | None = None

    def finding(self) -> _findings.Finding:
        return _findings.Finding(self.cls, self.file, self.line, self.token, self.what, self.hint)

    def render(self) -> str:
        return "%s:%d | %s | %s | %s" % (self.file, self.line, self.cls, self.what, self.hint)


# --- the unit header ------------------------------------------------------------------------------------------------

def header_of(text: str) -> tuple[str, int]:
    """`(the file's leading block comment, its first line)`: the unit header, `("", 0)` when the file opens with code."""
    m = re.match(r"\s*", text)
    pos = m.end() if m else 0
    if not text.startswith("/*", pos):
        return "", 0
    end = text.find("*/", pos)
    end = len(text) if end < 0 else end + 2
    return text[pos:end], text.count("\n", 0, pos) + 1


def header_section(header: str, label: str) -> str:
    """The text of one header label (`RESIDUALS`, `NAMES`, ...) up to the next label; `""` when absent."""
    marks = [(m.start(), m.group(1).strip()) for m in LABEL_RE.finditer(header)]
    for i, (at, name) in enumerate(marks):
        if name == label:
            stop = marks[i + 1][0] if i + 1 < len(marks) else len(header)
            return header[at:stop]
    return ""


def residual_text(header: str) -> str:
    """Where a header names what still differs: its RESIDUALS section, else the whole header minus the RANGE line
    (an older header spells residuals in free prose; RANGE names every section and would answer everything)."""
    text = header_section(header, "RESIDUALS")
    if text:
        return text
    return re.sub(r"(?m)^.*\bRANGE\..*$", "", header)


def plain_names(name: str) -> set[str]:
    """The spellings a header may use for a symbol: itself, its owner stem (`setTevKColor__6MHchar`), its identifier
    (`setTevKColor`) and, for a qualified dump spelling, the last component."""
    out = {name, _names.owner_stem(name), name.split("::")[-1]}
    if "__" in name.strip("_"):
        head = name[:name.find("__", 1)] if not name.startswith("__") else name
        out.add(head)
    return {n for n in out if n}


def mentions(text: str, names) -> bool:
    """Whether any of `names` occurs in `text` as a whole word."""
    return any(re.search(r"(?<![\w$@])%s(?![\w$@])" % re.escape(n), text) for n in names if n)


# --- the tree a check reads -----------------------------------------------------------------------------------------

class Context:
    """The branch (or the working tree) against its base: the files, the head map and splits, the registered units."""

    def __init__(self, root: str, base: str = "main", branch: str | None = None) -> None:
        self.root = root
        self.branch = branch
        self.git = Git(root)
        self.base = self.git.merge_base(base, branch or "HEAD") or base
        self.tree = _facts.Tree(root, branch)
        texts = self.tree.read_many([SYMBOLS_REL, SPLITS_REL, "configure.py"])
        self.own = Ownership.from_texts(texts.get(SYMBOLS_REL) or "", texts.get(SPLITS_REL) or "")
        try:
            objects = _configure.Configure.parse(texts.get("configure.py") or "").objects()
        except Exception:                                       # noqa: BLE001 - an unparsable registry names no unit
            objects = []
        self.units = {os.path.splitext(o.path.replace("\\", "/"))[0]: "src/" + o.path.replace("\\", "/")
                      for o in objects}
        args = ["diff", "--no-renames", "--no-color", "--no-ext-diff", "-U0", self.base] + ([branch] if branch else [])
        p = self.git.run(*args, "--", "src")
        if p.returncode != 0:
            raise RuntimeError("git diff failed: %s" % (p.stderr or "").strip())
        self.added = _facts.parse_added(p.stdout or "")
        if branch is None:                                      # untracked new files are added text too
            for rel in self.git.out("ls-files", "-z", "--others", "--exclude-standard", "--", "src",
                                    check=False).split("\0"):
                if rel and rel not in self.added:
                    body = self.tree.read(rel) or ""
                    self.added[rel] = list(enumerate(body.split("\n"), 1))

    def text(self, rel: str) -> str:
        return self.tree.read(rel) or ""

    def touched_units(self) -> dict[str, str]:
        """`{unit stem: its source path}` for every registered unit whose source or own header the diff changed."""
        out = {}
        for rel in self.added:
            if not rel.startswith("src/"):
                continue
            stem = os.path.splitext(rel[4:])[0]
            if stem in self.units:
                out[stem] = self.units[stem]
        return dict(sorted(out.items()))

    def object_paths(self, stem: str) -> tuple[str, str]:
        b = os.path.join(self.root, "build", _repo.VERSION)
        return (os.path.join(b, "src", *(stem + ".o").split("/")), os.path.join(b, "obj", *(stem + ".o").split("/")))


def owner_at(own: Ownership, address: int, section: str | None = None) -> tuple[str, str] | None:
    """`(unit, section)` of the registered range covering `address` (every section when `section` is None)."""
    for sec in ([section] if section else list(own.ranges)):
        hit = own.covering(sec, address)
        if hit is not None:
            return hit[2], sec
    return None


def address_of(own: Ownership, name: str) -> tuple[str | None, int] | None:
    """`(section, address)` of a map name, or of a generated stem's own address (section unknown)."""
    entries = own.symbols.get(name)
    if entries and len(entries) == 1:
        return entries[0][0], entries[0][1]
    m = re.fullmatch(r"(?:fn|lbl)_([0-9A-Fa-f]{8})", name)
    return (None, int(m.group(1), 16)) if m else None


def unit_stem_of(path: str) -> str:
    p = path[4:] if path.startswith("src/") else path
    return os.path.splitext(p)[0]


# --- (a) owner by address, stale paths; (f) "unowned" claims ---------------------------------------------------------

def comment_lines(text: str, added: list[tuple[int, str]]) -> list[tuple[int, str]]:
    """The added lines' comment text (code and literals blanked), skipping lines with none."""
    com = _comments.comment_only(text).split("\n")
    out = []
    for n, _line in added:
        if 0 < n <= len(com) and com[n - 1].strip():
            out.append((n, com[n - 1]))
    return out


def check_citations(rel: str, lines: list[tuple[int, str]], own: Ownership, exists, units: dict) -> list[Item]:
    """(a) A cited unit path that no longer exists; a symbol or address a line says some unit owns, resolved by ADDRESS
    against `splits.txt`, when the cited unit is a registered one that does not own it."""
    out = []
    for n, line in lines:
        paths = [(m.start(), m.group()) for m in PATH_RE.finditer(line)]
        for path in dict.fromkeys(p for _a, p in paths):
            if not exists(path):
                out.append(Item(rel, n, "stale-path", "`%s` names no file in the tree" % path,
                                "name the live path (the unit's current home), or drop the reference", path))
        if not paths:
            continue
        cites = {}
        for at, tok in ([(m.start(), m.group()) for m in STEM_RE.finditer(line)]
                        + [(m.start(1), m.group(1)) for m in TICKED_RE.finditer(line) if m.group(1) in own.symbols]
                        + [(m.start(), m.group()) for m in ADDRESS_RE.finditer(line)]):
            cites.setdefault(at, tok)
        tokens = sorted([(a, "cite", t) for a, t in cites.items()] + [(a, "path", p) for a, p in paths])
        claims = []
        for rx, subject, obj in ((PASSIVE_RE, "cite", "path"), (ACTIVE_RE, "path", "cite")):
            for m in rx.finditer(line):
                before = [t for t in tokens if t[0] < m.start()]
                after = [t for t in tokens if t[0] >= m.end() - 1]
                if not before or not after or before[-1][1] != subject or after[0][1] != obj:
                    continue
                # the object follows the verb at once (a backtick at most) and the subject sits in the same clause
                if after[0][0] - m.end() > 1 or CLAUSE_BREAK_RE.search(line, before[-1][0], m.start()):
                    continue
                pair = (before[-1][2], after[0][2])
                claims.append(pair if subject == "cite" else pair[::-1])
        for tok, path in dict.fromkeys(claims):
            cited = unit_stem_of(path)
            if cited not in units:
                continue                                       # a hub header or a retired path: not an owner claim
            loc = (None, int(tok, 16)) if tok.startswith("0x") else address_of(own, tok)
            if loc is None:
                continue
            hit = owner_at(own, loc[1], loc[0])
            if hit is None or unit_stem_of(hit[0]) == cited:
                continue
            out.append(Item(rel, n, "owner-by-address",
                            "`%s` (0x%08X) is owned by `src/%s` (splits.txt), not `%s`" % (tok, loc[1], hit[0], path),
                            "cite the owner by address (`symedit.py at`), and include that unit's header (rule 2)",
                            tok))
    return out


def check_unowned_claims(rel: str, lines: list[tuple[int, str]], own: Ownership) -> list[Item]:
    """(f) A line saying a symbol or address has no registered owner, when `splits.txt` gives it one. Only a token in
    the same clause as the phrase is judged (`0x8058D6C0-0x8058D7E0.  Both edges are the unclaimed run's` names no
    unowned address)."""
    out = []
    for n, line in lines:
        phrases = [m.span() for m in UNOWNED_RE.finditer(line)]
        if not phrases:
            continue
        found = [(m.start(), m.group()) for m in ADDRESS_RE.finditer(line)]
        found += [(m.start(), m.group()) for m in STEM_RE.finditer(line)]
        found += [(m.start(1), m.group(1)) for m in TICKED_RE.finditer(line) if m.group(1) in own.symbols]

        def near(at: int) -> bool:
            return any(not CLAUSE_BREAK_RE.search(line, min(at, a), max(at, b)) for a, b in phrases)
        toks = [tok for at, tok in sorted(found) if near(at)]
        for tok in dict.fromkeys(toks):
            loc = (None, int(tok, 16)) if tok.startswith("0x") else address_of(own, tok)
            hit = owner_at(own, loc[1], loc[0]) if loc else None
            if hit is not None:
                out.append(Item(rel, n, "unowned-claim",
                                "the comment says `%s` has no registered owner, but `src/%s` owns 0x%08X (splits.txt)"
                                % (tok, hit[0], loc[1]),
                                "include the owner's header (rule 2) and fix the comment", tok))
    return out


# --- (b) wrong callees ------------------------------------------------------------------------------------------------

def check_callees(rel: str, header: str, header_line: int, diffs: list[dict]) -> list[Item]:
    """(b) The symbol-name relocation differences (`objcompare.callee_diffs`) the header's residuals do not name, one
    item per function (a difference is named when either spelling, or its owner stem or identifier, is in the text)."""
    text = residual_text(header)
    out = []
    for fn in diffs:
        left = []
        for d in fn["diffs"]:
            names = set()
            for side in (d["ours"], d["target"]):
                if side:
                    names |= plain_names(side)
            if not mentions(text, names):
                left.append(d)
        if not left:
            continue
        shown = "; ".join("+0x%x %s: ours `%s`, retail `%s`" % (d["offset"], d["kind"], d["ours"] or "-",
                                                               d["target"] or "-") for d in left[:4])
        more = "" if len(left) <= 4 else " (+%d more)" % (len(left) - 4)
        out.append(Item(rel, header_line, "wrong-callee",
                        "%s: %d relocation symbol(s) differ and are not in the header's RESIDUALS - %s%s"
                        % (fn["function"], len(left), shown, more),
                        "call what retail calls, or record it in RESIDUALS (`relocdiff --callees %s`)"
                        % unit_stem_of(rel), fn["function"]))
    return out


# --- (c) empty stubs; (e) GUESS names --------------------------------------------------------------------------------

def definitions(text: str) -> list[tuple[str, str | None, int, bool]]:
    """`[(name, class qualifier or None, line, body is empty)]` for every function definition; an empty body is one
    with no statement but `(void)x;` casts."""
    t = cscan.Text(text)
    out = []
    for d in cscan.function_declarations(t):
        if d.body is None:
            continue
        qual = re.search(r"([A-Za-z_]\w*)\s*::\s*~?\s*$", t.code[max(0, d.pos - 200):d.pos])
        body = t.code[d.body[0] + 1:d.body[1]]
        body = re.sub(r"\(\s*void\s*\)\s*[A-Za-z_]\w*\s*;", "", body)
        out.append((d.name, qual.group(1) if qual else None, d.line, not body.strip()))
    return out


def map_row(own: Ownership, name: str, qual: str | None) -> tuple[str, str, int] | None:
    """`(map name, section, address)` of a definition: its exact name, else (a member) the one row spelled
    `name__<len><Class>...`."""
    if not qual:
        e = own.symbols.get(name)
        return (name, e[0][0], e[0][1]) if e and len(e) == 1 else None
    prefix = "%s__%d%s" % (name, len(qual), qual)
    hits = [(n, e[0][0], e[0][1]) for n, e in own.symbols.items() if n.startswith(prefix) and len(e) == 1]
    return hits[0] if len(hits) == 1 else None


def function_size(own: Ownership, address: int) -> int | None:
    rows = own.functions.get(address) or []
    return rows[0][1] if rows else None


def check_stubs(rel: str, text: str, header: str, own: Ownership, lines: set | None = None) -> list[Item]:
    """(c) A function whose body is empty while retail's is more than `blr`: the header must call it unwritten, and
    must not list it as a partial residual. `lines` limits it to the definitions the diff added (None: every one)."""
    res = residual_text(header)
    out = []
    for name, qual, line, empty in definitions(text):
        if not empty or (lines is not None and line not in lines):
            continue
        row = map_row(own, name, qual)
        size = function_size(own, row[2]) if row else None
        if size is not None and size <= TINY_FUNCTION:
            continue
        spelled = {name} | (plain_names(row[0]) if row else set())
        said = [ln for ln in res.split("\n") if mentions(ln, spelled)]
        if said and any(UNWRITTEN_RE.search(ln) for ln in said):
            continue
        what = ("`%s` has an empty body%s but the header lists it as a residual, not as unwritten"
                % (name, " (retail 0x%X bytes)" % size if size else "") if said else
                "`%s` has an empty body%s and the header does not record it as unwritten"
                % (name, " (retail 0x%X bytes)" % size if size else ""))
        out.append(Item(rel, line, "empty-stub", what,
                        "write the body, or say `unwritten` for it in RESIDUALS (never `partial`)", name))
    return out


def check_guesses(rel: str, text: str, header: str, own: Ownership, dump: dict | None,
                  lines: set | None = None) -> list[Item]:
    """(e) A name this unit defines that the runtime dump does not carry at its ADDRESS and the header's NAMES line
    does not mark a GUESS. `lines` limits it to the definitions the diff added (None: every one)."""
    if dump is None:
        return []
    names_text = header_section(header, "NAMES") or header
    out = []
    for name, qual, line, _empty in definitions(text):
        if lines is not None and line not in lines:
            continue
        if _names.is_generated(name):
            continue                                            # rule 7's finding, not a naming guess
        row = map_row(own, name, qual)
        if row is None:
            continue
        entries = [e for e in dump.get(row[2], []) if not e["placeholder"]]
        real = {e["clean"].split("::")[-1] for e in entries}
        if name in real or (row[0] in {e["name"] for e in entries}):
            continue
        marked = [ln for ln in names_text.split("\n") if "GUESS" in ln and mentions(ln, {name, row[0]})]
        if marked:
            continue
        dump_says = ", ".join(sorted({e["clean"] for e in entries})) or "a placeholder"
        out.append(Item(rel, line, "guess",
                        "`%s` (0x%08X) is not the dump's name there (%s) and the NAMES line does not mark it a GUESS"
                        % (name, row[2], dump_says),
                        "use the dump's name, or add `NAMES. %s is a GUESS (<the context>)`" % name, name))
    return out


# --- (d) flipcheck blockers ------------------------------------------------------------------------------------------

def blocker_names(problem: str) -> tuple[str, list[str]] | None:
    """`(class, [names or sections the header should mention])` for a flipcheck problem this check reads, else None."""
    m = FLIP_UNDEFINED_RE.search(problem)
    if m:
        return "undefined reference", [n.strip() for n in m.group(1).split(",")]
    m = FLIP_ROW36_RE.search(problem)
    if m:
        return "row 36", [n.strip() for n in m.group(1).split(",") if n.strip() and "..." not in n]
    for rx, cls in ((FLIP_NOT_EMITTED_RE, "claimed, not emitted"), (FLIP_SIZE_RE, "size gap"),
                    (FLIP_UNCLAIMED_RE, "emitted, not claimed")):
        m = rx.search(problem)
        if m:
            sec = m.group(1)
            return (None if sec in CODE_SECTIONS else (cls, [sec]))
    return None


def check_blockers(rel: str, header: str, header_line: int, problems: list[str]) -> list[Item]:
    """(d) Every flipcheck blocker of these classes that the header's residuals do not mention by name or section."""
    text = residual_text(header)
    out = []
    for p in problems:
        got = blocker_names(p)
        if got is None:
            continue
        cls, names = got
        if names and mentions(text, names):
            continue
        out.append(Item(rel, header_line, "flip-blocker", "%s not in the header's RESIDUALS: %s" % (cls, p[:240]),
                        "record it in RESIDUALS (what blocks the flip), or fix it", names[0] if names else None))
    return out


def flip_problems(root: str, stems: list[str]) -> dict[str, list[str]] | None:
    """flipcheck's problems per unit, from one `--json` run over all of them; None when it could not run."""
    if not stems:
        return {}
    p = _proc.run([sys.executable, FLIPCHECK, "--json", "--root", root, *stems], cwd=root)
    try:
        data = json.loads(p.stdout or "")
    except ValueError:
        return None
    return {u: v.get("problems", []) for u, v in (data.get("units") or {}).items()}


# --- the run ----------------------------------------------------------------------------------------------------------

def run(root: str, base: str = "main", branch: str | None = None, flipcheck: bool = True,
        dump_path: str | None = dumpsyms.DEFAULT_DUMP, dump: dict | None = None) -> dict:
    """Every check over the units the diff touches: `{base, head, units, items, skipped, seconds}`."""
    t0 = time.time()
    ctx = Context(root, base, branch)
    items: list[Item] = []
    skipped: list[str] = []
    if dump is None and dump_path:
        try:
            dump = dumpsyms.load_dump(dump_path)[0]
        except SystemExit as exc:
            skipped.append("guess: %s" % exc)
    elif dump is None:
        skipped.append("guess: no dump (--no-dump)")
    for rel, added in sorted(ctx.added.items()):
        if not rel.endswith(SOURCE_SUFFIXES + HEADER_SUFFIXES):
            continue
        lines = comment_lines(ctx.text(rel), added)
        items += check_citations(rel, lines, ctx.own, ctx.tree.exists, ctx.units)
        items += check_unowned_claims(rel, lines, ctx.own)
    units = ctx.touched_units()
    problems = flip_problems(root, list(units)) if flipcheck else None
    if not flipcheck:
        skipped.append("flip-blocker: --no-flipcheck")
    elif problems is None:
        skipped.append("flip-blocker: flipcheck could not run")
    for stem, src in units.items():
        text = ctx.text(src)
        header, hline = header_of(text)
        hline = hline or 1
        ours, target = ctx.object_paths(stem)
        if os.path.exists(ours) and os.path.exists(target):
            try:
                items += check_callees(src, header, hline, objcompare.callee_diffs(target, ours))
            except (OSError, ValueError) as exc:
                skipped.append("wrong-callee %s: %s" % (stem, exc))
        else:
            skipped.append("wrong-callee %s: no compiled or split object in this tree's build/" % stem)
        new = {n for n, _t in ctx.added.get(src, [])}
        items += check_stubs(src, text, header, ctx.own, new)
        items += check_guesses(src, text, header, ctx.own, dump, new)
        if problems:
            items += check_blockers(src, header, hline, problems.get(stem, []))
    order = {c: i for i, c in enumerate(CLASSES)}
    items.sort(key=lambda i: (order.get(i.cls, 99), i.file, i.line))
    return {"base": ctx.base, "head": branch or "(working tree)", "units": list(units), "items": items,
            "skipped": skipped, "seconds": round(time.time() - t0, 1)}


def main(args) -> object:
    root = _repo.worktree_root(args.root)
    try:
        res = run(root, args.base, args.branch, flipcheck=not args.no_flipcheck,
                  dump_path=None if args.no_dump else (args.dump or dumpsyms.DEFAULT_DUMP))
    except RuntimeError as exc:
        print("lanecheck: %s" % exc, file=sys.stderr)
        return _findings.EXIT_ERROR
    findings = [i.finding() for i in res["items"]]
    failed = bool(findings) or (args.strict and bool(res["skipped"]))
    if args.json:
        payload = {"tool": TOOL.name, "ok": not failed, "base": res["base"], "head": res["head"],
                   "units": res["units"], "skipped": res["skipped"], "seconds": res["seconds"],
                   "rows": [dict(f.to_dict(), hint=i.hint) for f, i in zip(findings, res["items"])],
                   "summary": "%d finding(s) over %d unit(s)" % (len(findings), len(res["units"]))}
        print(json.dumps(payload, indent=1))
        return 1 if failed else 0
    for i in res["items"]:
        print(i.render())
    for s in res["skipped"]:
        print("not checked: %s" % s)
    counts = {c: sum(1 for i in res["items"] if i.cls == c) for c in CLASSES}
    print("lanecheck: %s..%s: %d unit(s), %d finding(s) (%s) in %.1f s"
          % (res["base"][:12], res["head"], len(res["units"]), len(findings),
             ", ".join("%s %d" % (c, n) for c, n in counts.items() if n) or "none", res["seconds"]))
    return 1 if failed else 0


def build_parser():
    ap = TOOL.parser()
    ap.add_argument("--branch", default=None, help="judge this branch's committed tree (default: the working tree)")
    ap.add_argument("--base", default="main", help="the base it is judged against (its merge base; default: main)")
    ap.add_argument("--no-flipcheck", action="store_true", help="skip the flipcheck-blocker check (it builds nothing "
                                                                "but reads every touched unit's objects)")
    ap.add_argument("--no-dump", action="store_true", help="skip the GUESS check (it reads the runtime dump)")
    ap.add_argument("--dump", default=None, help="DumpSymbols.zip (default: $MHTRI_DUMP_SYMBOLS or lib.dumpsyms')")
    ap.add_argument("--strict", action="store_true", help="also exit 1 when a check could not run (`not checked`)")
    return ap


if __name__ == "__main__":
    raise SystemExit(TOOL.run(main, parser=build_parser()))
