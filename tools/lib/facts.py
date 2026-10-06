"""Fact tokens and where they survive: the facts-preserved judgement factscheck and the comment sweep share.
Spec: docs/tools/spec/lib-facts.md. CLI: none (library)."""
from __future__ import annotations

import os
import re
from dataclasses import dataclass, field

from tools.lib import repo as _repo
from tools.lib.git import Git
from tools.lib.project import symbols as _symbols

# --- what a fact is --------------------------------------------------------------------------------------------

#: A path-qualified name: two or more components joined by `/` (`src/ef/ef_util.cpp`, `include/ef.h`, `auto/...`).
PATH_RE = re.compile(r"[\w.+\-]+(?:/[\w.+*\-]*)+")
#: A date (`2026-09-25`, `2026-09`): inert - the campaign's own clock, never a fact about the game.
DATE_RE = re.compile(r"\b20\d\d-\d\d(?:-\d\d)?\b")
#: The fact tokens: a hex value of four or more digits, an identifier, a decimal number of three or more digits.
FACT_RE = re.compile(r"(?<![\w])0x[0-9A-Fa-f]{4,}(?![\w])|(?<![\w])\w*[A-Za-z_]\w*|(?<![\w.])\d{3,}(?![\w])")
#: How the corpora are indexed: every identifier, every hex literal, every 8-hex-digit run (the address inside a
#: generated stem such as `fn_802D44F4`), every decimal number.
_WORD_RE = re.compile(r"(?<![\w])\w*[A-Za-z_]\w*")
_HEX_RE = re.compile(r"0x([0-9A-Fa-f]+)")
_HEX8_RE = re.compile(r"(?<![0-9A-Fa-f])([0-9A-Fa-f]{8})(?![0-9A-Fa-f])")
_NUM_RE = re.compile(r"(?<![\w.])(\d+)(?![\w])")
#: A camel-humped identifier (`drawTextRuns`): a lower-case letter followed by an upper-case one.
_CAMEL_RE = re.compile(r"[a-z][A-Z]")
#: A generated stem whose address is the fact (`fn_802D44F4`, `lbl_805D4150`, `zz_02d0dcc_`): derivable when the
#: address is anywhere in the corpora.
_STEM_RE = re.compile(r"^(?:(?:[A-Za-z]+_)+([0-9A-Fa-f]{8})(?:_.*)?|([0-9A-Fa-f]{8})_\w+)$")
#: The runtime dump's stem (`zz_02d0dcc_`): seven hex digits, the address less 0x80000000.
_ZZ_RE = re.compile(r"^zz_([0-9A-Fa-f]{7})_$")
#: A glob over generated names (`fn_8004Bxxx`, `fn_XXXXXXXX`): a pattern, not a fact.
_GLOB_RE = re.compile(r"(?:xx|XXXX)")
#: Path prefixes of the campaign's dropped provenance (the scaffolding buckets, the proposal names, the phase-4
#: window notes): their components are the name of something that no longer exists, inert by ruling.
INERT_PATH_RE = re.compile(r"^(?:src/)?(?:auto/|proposal/)|^docs/splits/phase4(?:$|[/,])")
#: Inert identifier words (the narrative vocabulary the sweep drops): listed, never guessed.
INERT_WORDS = frozenset({"PHASE", "phase", "proposal", "phase4", "pre_phase"})
#: What the corpora are (the order is the order `--explain` names the first source that holds a token).
CORPUS_FILES = ("configure.py", "config/RMHE08/splits.txt")
SYMBOLS_REL = "config/RMHE08/symbols.txt"
DOCS_REL = "docs"
#: The suffixes of one unit's files: a source and its own header share a stem.
UNIT_SUFFIXES = (".c", ".cp", ".cpp", ".h", ".hpp")


def is_fact_word(tok: str) -> bool:
    """An identifier counts when it carries an underscore, or camel humps and at least six characters."""
    if tok.strip("_") == "":
        return False
    if "_" in tok.strip("_"):
        return True
    return len(tok) >= 6 and bool(_CAMEL_RE.search(tok))


@dataclass
class Index:
    """The tokens of one corpus: identifiers, hex values (ints) and decimal numbers (ints)."""
    words: set = field(default_factory=set)
    hexes: set = field(default_factory=set)
    nums: set = field(default_factory=set)

    def add_text(self, text: str) -> "Index":
        self.words.update(_WORD_RE.findall(text))
        self.hexes.update(int(h, 16) for h in _HEX_RE.findall(text))
        self.hexes.update(int(h, 16) for h in _HEX8_RE.findall(text))
        self.nums.update(int(n) for n in _NUM_RE.findall(text))
        return self

    def holds(self, kind: str, value) -> bool:
        if kind == "word":
            return value in self.words
        if kind == "hex":
            return value in self.hexes
        return value in self.nums


def classify(tok: str) -> tuple[str, object]:
    """`(kind, value)` for a fact token: `hex` (int), `num` (int) or `word` (str)."""
    if re.fullmatch(r"0[xX][0-9A-Fa-f]+", tok):
        return "hex", int(tok, 16)
    if tok.isdigit():
        return "num", int(tok)
    return "word", tok


# --- the tree a check reads --------------------------------------------------------------------------------------

class Tree:
    """The new side of a diff: the working tree (`ref=None`) or a commit. Reads files and knows which paths exist."""

    def __init__(self, root: str, ref: str | None = None) -> None:
        self.root = root
        self.ref = ref
        self.git = Git(root)
        if ref is None:
            out = self.git.out("ls-files", "-z", "--cached", "--others", "--exclude-standard", check=False)
        else:
            out = self.git.out("ls-tree", "-r", "-z", "--name-only", ref, check=False)
        self.files = sorted({f for f in out.split("\0") if f})
        self.paths = set(self.files)
        for f in self.files:
            parts = f.split("/")
            for i in range(1, len(parts)):
                self.paths.add("/".join(parts[:i]))
        self._cache: dict[str, str | None] = {}

    def read_many(self, rels) -> dict[str, str | None]:
        rels = [r for r in rels if r not in self._cache]
        if rels:
            if self.ref is None:
                for r in rels:
                    try:
                        with open(os.path.join(self.root, r), "rb") as fh:
                            self._cache[r] = fh.read().decode("utf-8", "replace")
                    except OSError:
                        self._cache[r] = None
            else:
                for r, data in self.git.show_many(self.ref, rels).items():
                    self._cache[r] = None if data is None else data.decode("utf-8", "replace")
        return self._cache

    def read(self, rel: str) -> str | None:
        return self.read_many([rel]).get(rel)

    def unit_text(self, rel: str) -> str:
        """The new text of `rel` and of its same-stem siblings (`x.cpp` and `x.h`): a unit's facts live in its source
        and its own header, so a fact moved from one to the other has not been lost."""
        stem, ext = os.path.splitext(rel)
        rels = [rel]
        if ext in UNIT_SUFFIXES:
            rels += [stem + s for s in UNIT_SUFFIXES if s != ext and (stem + s) in self.paths]
        texts = self.read_many(rels)
        return "\n".join(texts.get(r) or "" for r in rels)

    def exists(self, path: str) -> bool:
        """Whether a path-qualified name names something in the tree: as written, below `src/` (an include
        spelling), or through the retired `include/` root (`include/X` is `src/X`, plus movehdr's exceptions)."""
        p = path.strip("`'\".,;:()").rstrip("/")
        if not p:
            return False
        for cand in self._candidates(p):
            if cand in self.paths:
                return True
        return False

    @staticmethod
    def _candidates(p: str) -> list[str]:
        out = [p, "src/" + p]
        if p.startswith(_repo.LEGACY_HEADER_ROOT + "/"):
            out.append(_repo.moved_header(p))
        return out


class Corpora:
    """Every corpus a removed token may survive in, indexed once: configure.py, splits.txt, symbols.txt, docs/**."""

    def __init__(self, tree: Tree) -> None:
        self.tree = tree
        self.named: list[tuple[str, Index]] = []
        texts = tree.read_many(CORPUS_FILES)
        for rel in CORPUS_FILES:
            self.named.append((rel, Index().add_text(texts.get(rel) or "")))
        self.named.append((SYMBOLS_REL, self._symbols_index()))
        docs = [f for f in tree.files if f.startswith(DOCS_REL + "/") and not f.lower().endswith((".png", ".jpg"))]
        texts = tree.read_many(docs)
        idx = Index()
        for rel in docs:
            idx.add_text(texts.get(rel) or "")
        self.named.append((DOCS_REL + "/**", idx))
        self.changed: list[tuple[str, Index]] = []

    def index_changed(self, rels) -> int:
        """Index the new text of every file the diff changed, one corpus per file, so a token moved verbatim to
        another file of the same diff survives there (`judge` names it `moved to <file>`). The fixed corpora are
        skipped (already indexed); a deleted file has no new text. Returns how many files were indexed."""
        fixed = set(CORPUS_FILES) | {SYMBOLS_REL}
        rels = sorted({r for r in rels if r not in fixed and not r.startswith(DOCS_REL + "/")})
        texts = self.tree.read_many(rels)
        self.changed = [(rel, Index().add_text(texts[rel])) for rel in rels if texts.get(rel)]
        return len(self.changed)

    def _symbols_index(self) -> Index:
        """The map's names, addresses and sizes, read row by row through `lib.project.symbols` (never printed)."""
        idx = Index()
        if self.tree.ref is None:
            path = os.path.join(self.tree.root, SYMBOLS_REL)
            if not os.path.isfile(path):
                return idx
            rows = _symbols.read(path).rows()
        else:
            text = self.tree.read(SYMBOLS_REL) or ""
            rows = (r for r in (_symbols.parse_line(line) for line in text.splitlines()) if r)
        for r in rows:
            idx.words.add(r.name)
            idx.add_text(r.name)
            idx.hexes.add(r.address)
            idx.hexes.add(r.size)
        return idx


# --- the verdict for one removed span -------------------------------------------------------------------------

def fact_tokens(text: str, tree: Tree | None = None) -> list[tuple[str, str]]:
    """`[(token, inert reason or "")]` for every fact token of `text` in order: a token inside an existing path, an
    inert provenance path, a glob or a date carries its reason; every other fact token carries `""`."""
    covered: list[tuple[int, int, str]] = []
    for m in DATE_RE.finditer(text):
        covered.append((m.start(), m.end(), "inert: date"))
    for m in PATH_RE.finditer(text):
        p = m.group().strip("`'\".,;:()")
        if "*" in p or "{" in p:
            covered.append((m.start(), m.end(), "inert: path glob"))
        elif INERT_PATH_RE.search(p):
            covered.append((m.start(), m.end(), "inert: provenance path"))
        elif tree is not None and tree.exists(p):
            covered.append((m.start(), m.end(), "path exists"))
    out = []
    for m in FACT_RE.finditer(text):
        tok = m.group()
        if re.fullmatch(r"0[xX][0-9A-Fa-f]{0,3}", tok):
            continue                                        # a short hex literal is not a fact
        kind, _value = classify(tok)
        if kind == "word" and not is_fact_word(tok):
            continue
        reason = next((why for a, b, why in covered if a <= m.start() and m.end() <= b), "")
        if not reason and kind == "word":
            if tok in INERT_WORDS:
                reason = "inert: narrative word"
            elif _GLOB_RE.search(tok) and re.search(r"[0-9A-Fa-f]_?[0-9A-Fa-f]*(?:xx|XXXX)", tok + "_"):
                reason = "inert: name glob"
        out.append((tok, reason))
    return out


def judge(text: str, new_text: str, corpora: Corpora, own: str | None = None) -> list[tuple[str, str]]:
    """`[(token, where)]` for every fact token of a removed span: the corpus that holds it, the inert reason, or
    `""` when it survives nowhere (the failure). After the unit's own new text and the fixed corpora, the new text of
    every other file the diff changed (`Corpora.index_changed`; the span's own file `own` and its same-stem
    siblings are skipped - they are the unit's text) is a corpus too: a token moved there reads `moved to <file>`."""
    new_idx = Index().add_text(new_text or "")
    stem = os.path.splitext(own)[0] if own else None
    moved = [("moved to " + rel, idx) for rel, idx in corpora.changed if os.path.splitext(rel)[0] != stem]
    named = [("new text of the unit", new_idx)] + corpora.named + moved
    out = []
    for tok, reason in fact_tokens(text, corpora.tree):
        if reason:
            out.append((tok, reason))
            continue
        kind, value = classify(tok)
        where = next((name for name, idx in named if idx.holds(kind, value)), "")
        if not where and kind == "word":
            m = _STEM_RE.match(tok)
            z = _ZZ_RE.match(tok)
            if m or z:
                addr = int(m.group(1) or m.group(2), 16) if m else 0x80000000 | int(z.group(1), 16)
                where = next(("derivable: address 0x%08X in %s" % (addr, name)
                              for name, idx in named if addr in idx.hexes), "")
        out.append((tok, where))
    return out


def unmatched(text: str, new_text: str, corpora: Corpora) -> list[str]:
    """The fact tokens of `text` that survive nowhere, in order, each once."""
    seen, out = set(), []
    for tok, where in judge(text, new_text, corpora):
        if not where and tok not in seen:
            seen.add(tok)
            out.append(tok)
    return out


# --- the diff ---------------------------------------------------------------------------------------------------

@dataclass
class Span:
    """A run of consecutive removed lines of one hunk: the old file's `line` (1-based) and its text."""
    file: str
    line: int
    text: str


_HUNK_RE = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


def parse_added(diff: str) -> dict[str, list[tuple[int, str]]]:
    """`{file: [(new line number, text)]}` for every added line of a `git diff -U0` text (a deleted file has none)."""
    out: dict[str, list[tuple[int, str]]] = {}
    cur, new_line = None, 0
    for line in diff.split("\n"):
        if line.startswith("diff --git "):
            cur = None
            continue
        if line.startswith("--- "):
            continue
        if line.startswith("+++ "):
            path = line[4:].strip()
            cur = path[2:] if path.startswith("b/") else None
            continue
        m = _HUNK_RE.match(line)
        if m:
            new_line = int(m.group(3))
            continue
        if cur is None:
            continue
        if line.startswith("+"):
            out.setdefault(cur, []).append((new_line, line[1:].rstrip("\r")))
            new_line += 1
        elif not line.startswith("-") and not line.startswith("\\"):
            new_line += 1
    return out


def parse_diff(diff: str) -> list[Span]:
    """Every removed span of a `git diff -U0` text."""
    spans: list[Span] = []
    cur_file, old_line, buf, start = None, 0, [], 0

    def flush():
        nonlocal buf
        if buf and cur_file:
            spans.append(Span(cur_file, start, "\n".join(buf)))
        buf = []

    for line in diff.split("\n"):
        if line.startswith("diff --git "):
            flush()
            cur_file = None
            continue
        if line.startswith("+++ "):
            continue
        if line.startswith("--- "):
            path = line[4:].strip()
            cur_file = path[2:] if path.startswith("a/") else None
            continue
        m = _HUNK_RE.match(line)
        if m:
            flush()
            old_line = int(m.group(1))
            continue
        if cur_file is None:
            continue
        if line.startswith("-"):
            if not buf:
                start = old_line
            buf.append(line[1:].rstrip("\r"))
            old_line += 1
        else:
            flush()
    flush()
    return spans
