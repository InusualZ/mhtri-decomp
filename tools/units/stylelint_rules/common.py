"""The shared vocabulary of the lint: paths, rule names, a scanned `Source`, the finding shape and the tree walks.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import os
import re
from dataclasses import dataclass

from tools.lib import cscan
from tools.lib import findings as _findings
from tools.lib.project.ownership import BAND_ROOT as _BAND_ROOT, HEADER_SUFFIXES as _HEADER_SUFFIXES
from tools.lib.project.ownership import is_band_header as _is_band_header
from tools.lib.repo import LEGACY_HEADER_ROOT as _LEGACY_HEADER_ROOT


SRC = "src"
# The unsplit band (`src/unsplit/<module>.h`; `include/unsplit/` before the 2026-10-05 move) is the legitimate home for a symbol with no registered
# owner. Rule 2 reads it inverted: an owned symbol declared here collides with the owner's typed definition (MWCC
# `(10197) illegal function overloading`) in every translation unit that includes the band. Every body rule applies
# to it as to any header (2026-10-05). The path is `lib.project.ownership.BAND_ROOT`, the one spelling - the owner's
# ruling moved the band beside the sources, and the classifier follows that constant.
UNSPLIT = _BAND_ROOT
# The pseudo-module rule 2 reports when the registered bands bracketing an unsplit address name different
# modules (a `sound` unit inside the `ef` band): no `<module>.h` is sound, so the finding names the band
# directory instead. It is not a path component, so it cannot collide with a real module name.
UNSPLIT_UNRESOLVED = "<band unresolved>"
# The header tree before the owner's 2026-10-05 move (`lib.repo.LEGACY_HEADER_ROOT`). It is a *walk root*, never a
# classifier: a header is a file with a `HEADER_SUFFIXES` suffix wherever it lives (`is_header`), and since the move
# every header sits beside its source under `src/`. It stays in `LINT_ROOTS` because a ref older than the move (the
# gate's `--diff` back side) keeps its headers there; in a moved tree it is simply absent.
HEADERS = _LEGACY_HEADER_ROOT
LINT_ROOTS = (SRC, HEADERS)
HEADER_SUFFIXES = _HEADER_SUFFIXES
SUFFIXES = (".c", ".cpp", ".cp", ".cc") + HEADER_SUFFIXES

# Rule 7's path-keyed exemption table is **empty and stays empty** (owner's ruling, 2026-09-27): the
# `src/auto/` bucket is retired and no path exempts a rule. The table is kept so `exemptions()` and the
# "not enforced" report lines stay honest - an empty table is a statement, not an omission.
# One entry per (rule, path prefix); every other rule still applies under the prefix.
EXEMPT: list = []

# The per-file rule-7 keys are gone too: a file with no bodies is fired on like any other, and a
# `rule 7 deferred: <reason>` comment is inert. Reported alongside `EXEMPT` because both are conditions
# rather than path prefixes; empty now, and the JSON output says so.
RULE7_NOTES: list = []

RULE_NAMES = {
    1: "a shared type is defined once (in the owner's header)",
    2: "an extern lives with the TU that owns it (or a header under src/unsplit/)",
    3: "struct/class states its size (/* size: 0xNN */)",
    4: "field carries its offset (/* +0xNN */)",
    5: "no field left named unkNN (pad_0xNN / unused_0xNN are the exception)",
    6: "no pointer arithmetic to reach a field",
    7: "no generated name survives (a `fn_`/`lbl_`/`loc_`/`dtor_`/`zz_` + 8-hex stem or a DOL address anywhere in an "
       "identifier or a file/directory name, bare `unkNN`)",
    8: "goto is forbidden",
    9: "no mangled spelling used as a callable identifier (call/declare the owner)",
    10: "a vtable we own is compiler output (vtableaudit's violations; `--budget` reads them, the lint has none)",
    11: "no `void *` parameter or return type (mark the declaration `/* untyped: <reason> */` if genuinely untyped)",
    12: "data no registered range claims is the unit's to claim and match (an `extern` for it is the finding)",
    13: "a method is a member (`<Type>_<name>(<Type>* self, ...)` is `<Type>::<name>`; mark a genuine C function "
        "`/* free: <reason> */`)",
    14: "a codegen pragma lives in the TU that needs it, not in a shared header",
    15: "comment hygiene: no stale path, a function comment's `0xADDR (0xSIZE)` prefix is the function's (both "
        "refuse); narrative markers, dates, percentages and self-names are advisory (counted, never refused)",
}
#: Rules the lint itself never reports: plan 6.5 rule 10 (vtable ownership) is `tools/units/vtableaudit.py`'s, so
#: `--budget` fills its column from the audit (`report.rule10_counts`) and the lint's own sets carry no rule-10 row.
AUDIT_RULES = {10: "tools/units/vtableaudit.py"}
# No rule is unchecked any more. Rule 2's remaining gap is dynamic (an unsplit address whose bracketing
# registered units name different modules), so it is reported from `Ownership.gaps`, not from here.
UNCHECKED: list[tuple[int, str]] = []


# --------------------------------------------------------------------------------------------------
# stripping: comments and literals blanked out, positions and newlines preserved
# --------------------------------------------------------------------------------------------------
def strip(text: str) -> tuple[str, str]:
    """`(code, comments)` - `lib.cscan.strip`: comments and literals blanked, positions and newlines preserved."""
    return cscan.strip(text)


class Source(cscan.Text):
    """One file's text plus its two stripped views and a line index (`lib.cscan.Text`), and its paths.

    `rel` is the key a finding is filed under - the path the *working tree* spells, so both sides of a comparison
    line up across a rename. `origin` is the path the text really had (a base copy read at its old path), which is
    what rule 7's file-name findings judge; it defaults to `rel`."""

    def __init__(self, path: str, rel: str, text: str, origin: "str | None" = None):
        self.path = path
        self.rel = rel
        self.origin = (origin or rel).replace("\\", "/")
        super().__init__(text)


# --------------------------------------------------------------------------------------------------
# struct/class definitions and their fields
# --------------------------------------------------------------------------------------------------
STRUCT_RE = cscan.STRUCT_RE


def match_brace(code: str, open_pos: int) -> int:
    """Index of the `}` matching the `{` at `open_pos`, or -1."""
    return cscan.match_brace(code, open_pos)


def struct_defs(src: Source) -> list[dict]:
    """Every `struct`/`class` definition in the file, with its name, line range and body range."""
    return [d.to_dict() for d in cscan.struct_defs(src)]


def iter_fields(code: str, open_pos: int, close_pos: int) -> list[tuple[int, int]]:
    """`(start, end)` of every field declaration chunk at the body's top level."""
    return cscan.fields(code, open_pos, close_pos)


def field_name(chunk: str) -> str | None:
    """The declared name of a field chunk, or None when the chunk is not a plain field."""
    t = chunk.strip().rstrip(";").strip()
    if not t:
        return None
    if "{" in t:  # a nested type definition: its own struct_def entry covers it
        return None
    if re.fullmatch(r"(public|private|protected|typedef)", t):
        return None
    m = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)", t)  # function pointer: void (*name)(void)
    if m:
        return m.group(1)
    if "(" in t:  # a method declaration, not a field
        return None
    t = t.split(":")[0]  # drop a bitfield width
    t = re.sub(r"\[[^\]]*\]", "", t)  # drop array dimensions
    ids = re.findall(r"[A-Za-z_]\w*", t)
    return ids[-1] if ids else None


@dataclass(frozen=True)
class Field:
    """A named field of a struct/class body: the name, the line it is on, the line its declaration starts on, and
    the name's `(start, end)` in the file (None when the name is not found in the chunk's code)."""
    name: str
    line: int
    first_line: int
    span: "tuple[int, int] | None"


def field_walk(src: Source, defs: list[dict]) -> list[Field]:
    """Every named field of every definition in `defs` (`struct_defs`), in definition then body order - the one walk
    rules 4, 5 and 7 read (7 needs the spans: a field named `unk*` is rule 5's, not rule 7's)."""
    out = []
    for d in defs:
        for start, end in iter_fields(src.code, d["open"], d["close"]):
            chunk = src.code[start:end]
            name = field_name(chunk)
            if name is None:
                continue
            name_pos = src.code.find(name, start, end)
            first = start + (len(chunk) - len(chunk.lstrip()))
            out.append(Field(name, src.line_of(name_pos if name_pos >= 0 else start), src.line_of(first),
                             (name_pos, name_pos + len(name)) if name_pos >= 0 else None))
    return out


# --------------------------------------------------------------------------------------------------
# rule 1: a shared type is defined once (cross-file)
# --------------------------------------------------------------------------------------------------
# `union` is included (the rule names struct/class/union), and an anonymous `typedef struct { ... } Foo;`
# takes `Foo` from the tail exactly as `struct_defs` does. A forward declaration (`struct Foo;`) has no
# body and is not a definition, so it never counts.
RULE1_TYPE_RE = re.compile(r"\b(?:struct|class|union)\s+([A-Za-z_]\w*)?\s*(?::[^{;]*)?\{")


def type_defs(src: Source) -> list[tuple[str, int]]:
    """`(name, line)` for every named struct/class/union definition in `src`, first definition per name."""
    out = []
    seen: set[str] = set()
    for m in RULE1_TYPE_RE.finditer(src.code):
        open_pos = m.end() - 1
        name = m.group(1)
        if not name:  # `typedef struct { ... } Foo;`
            close_pos = match_brace(src.code, open_pos)
            if close_pos < 0:
                continue
            tail = re.match(r"\s*([A-Za-z_]\w*)", src.code[close_pos + 1:])
            name = tail.group(1) if tail else None
        if not name or name in seen:
            continue
        seen.add(name)
        out.append((name, src.line_of(m.start())))
    return out


def is_unsplit_header(rel: str) -> bool:
    """Whether `rel` is a declaration-only header in the unsplit band (`lib.project.ownership.is_band_header`)."""
    return _is_band_header(rel)


def is_header(rel: str) -> bool:
    """Whether `rel` is a header: a `HEADER_SUFFIXES` file anywhere - `include/**` and `src/**` alike."""
    return rel.replace("\\", "/").endswith(HEADER_SUFFIXES)


def is_shared_header(rel: str) -> bool:
    """Whether `rel` is a header (the unsplit band included). The old reading was "under `include/`"; a header is
    classified by its suffix now (`is_header`), so a header moved beside its source keeps every rule it had."""
    return is_header(rel)


def _finding(src: Source, rule: int, line: int, detail: str, token: str | None = None) -> dict:
    """One finding (`lib.findings.Finding` as a dict). `token` is the identifier at fault - the
    `fn_XXXXXXXX`/`unkNN` a rule-7 rename closes, the declared symbol rule 2/12 wants moved or claimed, the
    type/field a rule 3/5 wants named; None for a rule whose finding names no single token (rule 8's `goto`).
    """
    return _findings.Finding(rule, src.rel, line, token, detail, text=src.line_text(line).strip()[:160]).to_dict()


def _rule2_finding(src: Source, line: int, name: str, detail: str) -> dict:
    """A rule-2 finding, carrying the declared **symbol** as data and not only inside the message.

    `--diff` judges an added rule-2 finding by the *address* its symbol resolves to, so the name has to
    survive as a field (the `detail` string is prose meant for a human); `owed_rename_completion` reads it.
    """
    return dict(_finding(src, 2, line, detail, token=name), symbol=name)
RULE11_MARKER_RE = re.compile(r"untyped\s*:\s*([^\n]*)")


def match_paren(code: str, open_pos: int) -> int:
    """Index of the `)` matching the `(` at `open_pos`, or -1."""
    return cscan.match_paren(code, open_pos)


_mask_preproc = cscan.mask_preproc


def function_declarations(src: Source) -> list[dict]:
    """Every function declaration/definition header in the file, with its parameter and return text
    (`lib.cscan.function_declarations`; a definition carries `body`). A statement inside a function body is
    never one, which is what keeps a cast out of rule 11."""
    return [d.to_dict() for d in cscan.function_declarations(src)]


def _untyped_marker(src: Source, start_line: int, end_line: int,
                    marker_re: "re.Pattern" = RULE11_MARKER_RE) -> str | None:
    """The reason of a `/* untyped: <reason> */` marker on the declaration or the line above it.

    `marker_re` selects the marker: rule 13 reuses this with its `free:` pattern, so the window and the
    standalone-line rule are one implementation.

    The window is `start_line - 1 .. end_line`, exactly the owner's "on the declaration or the line above
    it". The line above must be a **standalone** marker (its code view is blank): a trailing marker on the
    previous declaration's own line is that declaration's, not the next one's, so one marker can never
    exempt two declarations. Per-file keys are gone (rule 7's removal is the precedent): a file may never
    exempt itself.
    """
    for line in range(max(1, start_line - 1), end_line + 1):
        seg_start = src._starts[line - 1]
        seg_end = src._starts[line] if line < len(src._starts) else len(src.comments)
        m = marker_re.search(src.comments[seg_start:seg_end])
        if not m:
            continue
        if line == start_line - 1 and src.code[seg_start:seg_end].strip():
            continue
        reason = m.group(1)
        if reason.endswith("*/"):
            reason = reason[:-2]
        return reason.strip()
    return None


_split_parameters = cscan.split_params  # `(chunk, absolute_offset)` per top-level comma-separated parameter


# --------------------------------------------------------------------------------------------------
# tree walking / budget
# --------------------------------------------------------------------------------------------------
def source_files(root: str) -> list[str]:
    out = []
    base = os.path.join(root, SRC)
    for dirpath, dirnames, filenames in os.walk(base):
        dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
        for name in sorted(filenames):
            if name.endswith(SUFFIXES):
                out.append(os.path.join(dirpath, name))
    return out


def rel_of(root: str, path: str) -> str:
    return os.path.relpath(path, root).replace(os.sep, "/")


def read_text(path: str) -> str:
    with open(path, "r", encoding="utf-8", errors="replace", newline="") as fh:
        return fh.read()


def all_header_files(root: str) -> list[str]:
    """Every header the lint judges, wherever it lives (`lint_files` filtered by `is_header`), in path order - the
    walk of the whole-tree header readings, so a header moved into `src/` stays in them."""
    return [path for path in lint_files(root) if is_header(path)]


def all_sources(root: str) -> list["Source"]:
    """Every `src/` file as a `Source`, in path order."""
    return [Source(path, rel_of(root, path), read_text(path)) for path in source_files(root)]


def all_lint_sources(root: str) -> list["Source"]:
    """Every file the lint judges (`lint_files`) as a `Source`, in path order."""
    return [Source(path, rel_of(root, path), read_text(path)) for path in lint_files(root)]


def header_files(root: str) -> list[str]:
    """Every header outside `src/` (today the `include/` tree, the unsplit band included), in path order.

    The complement of `source_files`, which already walks every file under `src/` - a header there included - so a
    caller that adds the two never reads one file twice. After the 2026-10-05 move this is empty."""
    out = []
    for top in LINT_ROOTS:
        if top == SRC:
            continue
        for dirpath, dirnames, filenames in os.walk(os.path.join(root, top)):
            dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
            for name in sorted(filenames):
                if name.endswith(HEADER_SUFFIXES):
                    out.append(os.path.join(dirpath, name))
    return out


def lint_files(root: str) -> list[str]:
    """Every file the lint judges - each `.c`/`.cpp`/`.h` under `LINT_ROOTS` - in path order: `source_files` (all of
    `src/`) then `header_files` (the headers outside it)."""
    return source_files(root) + header_files(root)


def unsplit_header_files(root: str) -> list[str]:
    """Every header in the unsplit band (`src/unsplit/`, or `include/unsplit/` before the move), in path order."""
    out = []
    for path in lint_files(root):
        if is_unsplit_header(rel_of(root, path)):
            out.append(path)
    return out


def rule_enforced(rule: int, rel: str, src: "Source | None" = None) -> bool:
    """Whether `rule` is enforced for `rel`. It always is: the rule-7 exemptions and per-file keys are
    gone (owner's ruling, 2026-09-27) and no other rule ever had one. Kept as an API because callers
    ask before they count."""
    return True


def exemptions() -> list[dict]:
    """The `EXEMPT` table plus the per-file rule-7 keys, in the shape the JSON output reports it."""
    out = [{"rule": r, "prefix": p, "why": w} for r, p, w in EXEMPT]
    out += [{"rule": 7, "prefix": None, "condition": cond, "why": why} for cond, why in RULE7_NOTES]
    return out
