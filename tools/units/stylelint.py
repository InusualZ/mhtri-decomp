#!/usr/bin/env python3
"""Lint `src/` against the type and naming discipline of docs/plan.md section 6.5 (roadmap 7.21).

A rule enforced by remembering is not a rule: eight conformance rules were agreed with the owner and the
only thing keeping them is the reviewer's attention. This tool turns the mechanically checkable ones into
`file:line` findings, and `land.py verify` calls it with `--diff <base>` so a batch that **adds** a
violation is refused before it is committed.

    python tools/units/stylelint.py --budget          # the per-unit backlog over src/
    python tools/units/stylelint.py --diff <ref>      # exit 0 = the batch adds no violation
    python tools/units/stylelint.py --json            # machine-readable findings + budget
    python tools/units/stylelint.py --selftest

Rules checked (each finding is `file:line`):

| # | rule | how it is decided |
| --- | --- | --- |
| 3 | a reconstructed `struct`/`class` states its size | a `size: 0xNN` comment within four lines of the definition (or two lines after its closing brace) |
| 4 | every field carries its offset | an offset comment on the field's own line(s); `/* +0x1C */` is the canonical form and the `/* 0x1C */` variant the existing units use is accepted |
| 5 | no field is left named `unk*` | a field name matching `unk`, `unkNN`; `pad_0xNN` / `unused_0xNN` are the exception |
| 6 | no pointer arithmetic reaches a field | a `(T*)base + 0xNN` / `(T*)(base + 0xNN)` cast-plus-literal-offset expression, except an offset passed straight to `memset`/`memcpy`/`memmove` (the rule's own byte-range exception) |
| 7 | no auto-generated name survives | `fn_XXXXXXXX` anywhere, and `unk*` used for anything that is not a struct field (a field is rule 5's); **exempt per file** - see below |
| 8 | `goto` is forbidden | the `goto` keyword |

Rules 1 (a shared type lives in one header) and 2 (an extern lives with the TU that owns it) need
cross-file analysis and are **not checked** - they are reported as such so nobody assumes coverage.

Comments and string/char literals are stripped before matching, and the two are stripped separately: the
size/offset annotations of rules 3-4 live *in comments*, while every other rule must not fire on text
inside one. The stripped strings keep the file's exact length and newlines, so a reported line is the
original line. `Pl/pl_act.cpp`'s header comment naming the banned `goto` dispatch is the incident this
exists for.

The backlog is a burn-down, not a gate: `--diff` fails only when a (rule, file) count rises, so touching a
unit with 300 `unk*` fields is allowed as long as the touch adds none. A **new** file starts from zero, so
its violations are all additions - new work is held to the rules from its first commit.

**Rule 7 is keyed on the file, not the directory.** Three exemption keys, narrowest last (`rule7_state`):

1. under `src/auto/` - a **temporary grandfather** for the legacy scaffolding units that already have
   bodies. The `auto/` bucket is retired and its units are being named and moved to their final homes,
   so this entry goes with the migration (`EXEMPT` below).
2. **no bodies yet** - a stub is a file-header comment and forward declarations; it has nothing to name,
   so rule 7 cannot apply. This is exactly `brief.text_has_bodies` (a brace outside the comments), so the
   pool and the gate never disagree about what a stub is.
3. **`rule 7 deferred: <reason>`** in a comment - the durable key, and the one that covers a unit that
   already has bodies at its final `src/<module>/<name>` home: a worker's landing commit registers the
   stub and writes the bodies together, so keys 1 and 2 cannot. It is per-unit and reviewable -
   `grep -rn "rule 7 deferred" src/` is the complete list - and key 3 defers the `fn_` half only: a bare
   `unk*` local is nameable from its context, so it still reports.

A key is checkable from the file alone, and a **finished** unit cannot hide behind keys 2 or 3 silently:
key 2 needs the absence of every body, and key 3 is one greppable line whose reason must name the evidence.
**Rules 1-6 and 8 still apply under every key** (sized types, fields with offsets and context names, no
pointer arithmetic, no `goto`).
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys

# `brief.text_has_bodies` is the one definition of "a file with no bodies yet" (rule 7 key 2, `rule7_state`).
# Reuse it rather than keep a second brace scanner. Imported as a package module under land.py/promote.py,
# as a sibling script for `python tools/units/stylelint.py`.
try:
    from units import brief as _brief  # noqa: E402
except ImportError:  # `python tools/units/stylelint.py ...`
    import brief as _brief  # type: ignore  # noqa: E402

SRC = "src"
SUFFIXES = (".c", ".cpp", ".cp", ".cc", ".h", ".hpp", ".hh")

# Rule 7's path-keyed exemption table. This is now only a **temporary grandfather** for the legacy
# `src/auto/` units that already have bodies; the auto/ migration is naming and moving them, and this
# entry is removed with it. Do not add entries: the durable keys are per-file (`rule7_state`).
# One entry per (rule, path prefix); every other rule still applies under the prefix.
EXEMPT = [(7, "src/auto/", "temporary grandfather: legacy scaffolding with bodies, until the auto/ migration lands")]

# The per-file rule-7 keys, reported alongside `EXEMPT` because they are conditions, not path prefixes.
# `rule7_state` is the authority; these strings only keep the human/JSON output honest.
RULE7_NOTES = [
    ("a file with no bodies yet", "a stub has nothing to name"),
    ("a file declaring `rule 7 deferred: <reason>` in a comment", "per-unit and greppable; defers `fn_` only"),
]

RULE_NAMES = {
    3: "struct/class states its size (/* size: 0xNN */)",
    4: "field carries its offset (/* +0xNN */)",
    5: "no field left named unkNN (pad_0xNN / unused_0xNN are the exception)",
    6: "no pointer arithmetic to reach a field",
    7: "no fn_XXXXXXXX / bare unkNN identifier",
    8: "goto is forbidden",
}
UNCHECKED = [
    (1, "a shared type lives in one header"),
    (2, "an extern lives with the TU that owns it"),
]


# --------------------------------------------------------------------------------------------------
# stripping: comments and literals blanked out, positions and newlines preserved
# --------------------------------------------------------------------------------------------------
def strip(text: str) -> tuple[str, str]:
    """Return `(code, comments)`, each the same length as `text` with newlines in place.

    `code` has every comment and string/char literal replaced by spaces; `comments` has the comment
    bodies preserved and everything else blanked. Both keep one character per input character, so an
    offset into either maps to the original line/column exactly.
    """
    n = len(text)
    code = list(text)
    comments = list(text)
    i = 0
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            comments[i] = "/"
            comments[i + 1] = "*"
            code[i] = code[i + 1] = " "
            i += 2
            while i < n:
                comments[i] = text[i]
                if text[i] != "\n":
                    code[i] = " "
                if text[i] == "*" and i + 1 < n and text[i + 1] == "/":
                    comments[i + 1] = "/"
                    code[i + 1] = " "
                    i += 2
                    break
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                comments[i] = text[i]
                code[i] = " "
                i += 1
            continue
        if c in "\"'":
            quote = c
            code[i] = comments[i] = " "
            i += 1
            while i < n:
                if text[i] == "\\" and i + 1 < n:
                    code[i] = code[i + 1] = comments[i] = comments[i + 1] = " "
                    i += 2
                    continue
                if text[i] == "\n":
                    break  # unterminated literal / line continuation: do not swallow the newline
                code[i] = comments[i] = " "
                if text[i] == quote:
                    i += 1
                    break
                i += 1
            continue
        i += 1
    return "".join(code), "".join(comments)


class Source:
    """One file's text plus its two stripped views and a line index."""

    def __init__(self, path: str, rel: str, text: str):
        self.path = path
        self.rel = rel
        self.text = text
        self.code, self.comments = strip(text)
        self._starts = [0]
        for i, ch in enumerate(text):
            if ch == "\n":
                self._starts.append(i + 1)

    def line_of(self, pos: int) -> int:
        lo, hi = 0, len(self._starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if self._starts[mid] <= pos:
                lo = mid
            else:
                hi = mid - 1
        return lo + 1

    def line_text(self, line: int) -> str:
        start = self._starts[line - 1]
        end = self.text.find("\n", start)
        return self.text[start:] if end < 0 else self.text[start:end]

    def span_lines(self, start: int, end: int) -> tuple[int, int]:
        return self.line_of(start), self.line_of(max(start, end - 1))


# --------------------------------------------------------------------------------------------------
# struct/class definitions and their fields
# --------------------------------------------------------------------------------------------------
STRUCT_RE = re.compile(r"\b(?:struct|class)\s*([A-Za-z_]\w*)?\s*\{")
SIZE_RE = re.compile(r"size\s*:\s*0x[0-9A-Fa-f]+")
OFFSET_RE = re.compile(r"/\*\s*\+?0x[0-9A-Fa-f]+")
UNK_FIELD_RE = re.compile(r"^unk\w*$")


def match_brace(code: str, open_pos: int) -> int:
    """Index of the `}` matching the `{` at `open_pos`, or -1."""
    depth = 0
    for i in range(open_pos, len(code)):
        if code[i] == "{":
            depth += 1
        elif code[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def struct_defs(src: Source) -> list[dict]:
    """Every `struct`/`class` definition in the file, with its name, line range and body range."""
    out = []
    for m in STRUCT_RE.finditer(src.code):
        open_pos = m.end() - 1
        close_pos = match_brace(src.code, open_pos)
        if close_pos < 0:
            continue
        name = m.group(1)
        if not name:  # `typedef struct { ... } Name;`
            tail = re.match(r"\s*([A-Za-z_]\w*)", src.code[close_pos + 1:])
            name = tail.group(1) if tail else "<anonymous>"
        out.append({
            "name": name,
            "start": m.start(),
            "open": open_pos,
            "close": close_pos,
            "line": src.line_of(m.start()),
            "end_line": src.line_of(close_pos),
        })
    return out


def iter_fields(code: str, open_pos: int, close_pos: int) -> list[tuple[int, int]]:
    """`(start, end)` of every field declaration chunk at the body's top level."""
    out = []
    depth = 0
    start = open_pos + 1
    for i in range(open_pos + 1, close_pos):
        c = code[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        elif c == ";" and depth == 0:
            out.append((start, i))
            start = i + 1
    return out


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


def struct_has_size(src: Source, defs: list[dict]) -> set[int]:
    """Indices into `defs` that carry a `size: 0xNN` comment near their definition.

    Each annotation is assigned to its nearest definition (the following one wins a tie) so one comment
    cannot certify two adjacent types.
    """
    anns = [src.line_of(m.start()) for m in SIZE_RE.finditer(src.comments)]
    ok = set()
    for a in anns:
        best, best_key = None, None
        for idx, d in enumerate(defs):
            if d["line"] <= a <= d["end_line"]:
                dist = 0
            else:
                dist = min(abs(a - d["line"]), abs(a - d["end_line"]))
            key = (dist, abs(a - d["line"]))
            if best_key is None or key < best_key:
                best, best_key = idx, key
        if best is not None and best_key[0] <= 4:
            ok.add(best)
    return ok


# --------------------------------------------------------------------------------------------------
# rules
# --------------------------------------------------------------------------------------------------
_PTR_TYPE = (r"(?:const\s+)?(?:unsigned\s+|signed\s+)?"
             r"(?:u8|u16|u32|u64|s8|s16|s32|s64|char|short|int|long|float|double|f32|f64|void|BOOL)")
RULE6_RE = re.compile(
    r"\(\s*" + _PTR_TYPE + r"\s*\*+\s*\)"      # a cast to a pointer
    r"\s*\(?\s*"                               # optional opening paren
    r"[A-Za-z_0-9\.\->\[\]\*&\s]*?"            # the base expression (no operators/separators)
    r"[+\-]\s*(?:0x[0-9A-Fa-f]+|\d+)"          # plus/minus a literal byte offset
)
RULE7_FN_RE = re.compile(r"\bfn_[0-9A-Fa-f]{8}\b")
RULE7_UNK_RE = re.compile(r"\bunk\w*\b")
# Rule 7 key 3: the per-unit `rule 7 deferred: <reason>` declaration, matched only against the comment
# view (see `rule7_deferral`). `[ \t]*` rather than `\s*` keeps the declaration and its non-empty reason
# on one line, so `rule 7 deferred:` at the end of a comment cannot borrow the next line's first token,
# and a normal comment's closing `*/` cannot count as the reason.
RULE7_DEFER_RE = re.compile(r"rule[ \t]*7[ \t]+deferred[ \t]*:[ \t]*\S")
_COMMENT_DELIM_RE = re.compile(r"/\*|\*/|//")
RULE8_RE = re.compile(r"\bgoto\b")

# rule 6's own exception: a raw byte offset is allowed where no field is being named - a `memset`/`memcpy`
# range is the common case, so a cast-plus-offset that is an argument to one of these is not reported.
MEM_FUNCS = {"memset", "memcpy", "memmove", "memcmp", "bzero", "__memcpy", "__fill_mem"}


def enclosing_call(code: str, pos: int) -> str | None:
    """The identifier of the call whose argument list contains `pos`, or None."""
    depth = 0
    i = pos - 1
    while i >= 0:
        c = code[i]
        if c == ")":
            depth += 1
        elif c == "(":
            if depth == 0:
                m = re.search(r"([A-Za-z_]\w*)\s*$", code[:i])
                return m.group(1) if m else None
            depth -= 1
        elif c in ";{}":
            return None
        i -= 1
    return None


def _finding(src: Source, rule: int, line: int, detail: str) -> dict:
    return {"rule": rule, "file": src.rel, "line": line,
            "text": src.line_text(line).strip()[:160], "detail": detail}


def lint_source(src: Source) -> list[dict]:
    """All section 6.5 findings for one file, in rule then line order."""
    out: list[dict] = []
    defs = struct_defs(src)
    sized = struct_has_size(src, defs)

    for idx, d in enumerate(defs):
        if idx not in sized:
            out.append(_finding(src, 3, d["line"], "type `%s` has no `/* size: 0xNN */`" % d["name"]))

    field_spans: list[tuple[int, int]] = []
    for d in defs:
        for start, end in iter_fields(src.code, d["open"], d["close"]):
            chunk = src.code[start:end]
            name = field_name(chunk)
            if name is None:
                continue
            name_pos = src.code.find(name, start, end)
            line = src.line_of(name_pos if name_pos >= 0 else start)
            first = start + (len(chunk) - len(chunk.lstrip()))
            first_line = src.line_of(first)
            if name_pos >= 0:
                field_spans.append((name_pos, name_pos + len(name)))
            has_offset = any(
                OFFSET_RE.search(src.comments[src._starts[l - 1]: (src._starts[l] if l < len(src._starts) else len(src.comments))])
                for l in range(first_line, max(first_line, line) + 1)
            )
            if not has_offset:
                out.append(_finding(src, 4, line, "field `%s` has no offset annotation" % name))
            if UNK_FIELD_RE.match(name):
                out.append(_finding(src, 5, line, "field `%s` needs a context name or pad_0xNN" % name))

    for m in RULE6_RE.finditer(src.code):
        if enclosing_call(src.code, m.start()) in MEM_FUNCS:
            continue
        out.append(_finding(src, 6, src.line_of(m.start()), "pointer arithmetic: `%s`" % m.group(0).strip()))

    fn7_enforced, unk7_enforced = rule7_state(src)
    if fn7_enforced or unk7_enforced:
        def in_field(pos: int) -> bool:
            return any(a <= pos < b for a, b in field_spans)

        if fn7_enforced:
            for m in RULE7_FN_RE.finditer(src.code):
                out.append(_finding(src, 7, src.line_of(m.start()), "auto-generated name `%s`" % m.group(0)))
        if unk7_enforced:
            for m in RULE7_UNK_RE.finditer(src.code):
                if not in_field(m.start()):
                    out.append(_finding(src, 7, src.line_of(m.start()), "bare `%s` identifier" % m.group(0)))

    for m in RULE8_RE.finditer(src.code):
        out.append(_finding(src, 8, src.line_of(m.start()), "goto statement"))

    out.sort(key=lambda f: (f["rule"], f["line"]))
    return out


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


def lint_tree(root: str, paths: list[str] | None = None) -> list[dict]:
    out = []
    for path in (paths if paths is not None else source_files(root)):
        out.extend(lint_source(Source(path, rel_of(root, path), read_text(path))))
    return out


def budget(findings: list[dict]) -> dict:
    """Per-unit rule counts plus the totals, both keyed by unit (path relative to the repo root)."""
    per: dict[str, dict] = {}
    for f in findings:
        row = per.setdefault(f["file"], {"file": f["file"], "total": 0,
                                         "rules": {str(r): 0 for r in RULE_NAMES}})
        row["rules"][str(f["rule"])] += 1
        row["total"] += 1
    totals = {str(r): sum(row["rules"][str(r)] for row in per.values()) for r in RULE_NAMES}
    return {"units": [per[k] for k in sorted(per)], "totals": totals,
            "total": sum(totals.values()), "findings": len(findings), "unique": unique_names(findings)}


def unique_names(findings: list[dict]) -> dict:
    """Distinct names behind the name-based rules - the burn-down number a rename closes in one edit."""
    def names(rule: int, prefix: str) -> set:
        return {m.group(1) for f in findings if f["rule"] == rule and f["detail"].startswith(prefix)
                for m in [re.search(r"`([^`]+)`", f["detail"])] if m}
    return {"fn_names": len(names(7, "auto")), "unk_identifiers": len(names(7, "bare")),
            "unk_fields": len(names(5, "")), "types": len(names(3, ""))}


def rule7_deferral(comments: str) -> bool:
    """Whether a file's comment view carries a `rule 7 deferred: <reason>` declaration.

    `comments` is `Source.comments`, which keeps comment bodies and blanks string/char literals, so a
    declaration inside a literal cannot match. Comment delimiters are blanked before the search: an empty
    `rule 7 deferred:` followed by a `*/` must not borrow the `*` as its reason.
    """
    return bool(RULE7_DEFER_RE.search(_COMMENT_DELIM_RE.sub(" ", comments)))


def rule7_state(src: "Source") -> tuple[bool, bool]:
    """`(fn_enforced, unk_enforced)` for rule 7 in `src`, per the three keys.

    1. under `src/auto/` (temporary grandfather, `EXEMPT`): the whole rule is off;
    2. no bodies yet (`brief.text_has_bodies`): the whole rule is off;
    3. a `rule 7 deferred: <reason>` comment declaration: the `fn_` half is off, the `unk` half stays.
    """
    rel = src.rel.replace("\\", "/")
    if any(r == 7 and rel.startswith(prefix) for r, prefix, _why in EXEMPT):
        return (False, False)
    if not _brief.text_has_bodies(src.text):
        return (False, False)
    if rule7_deferral(src.comments):
        return (False, True)
    return (True, True)


def rule_enforced(rule: int, rel: str, src: "Source | None" = None) -> bool:
    """Whether `rule` is enforced at all for `rel` (rule 7 may be half-deferred, see `rule7_state`).

    With `src` this consults all three rule-7 keys; without it only the path-keyed `EXEMPT` table can be
    consulted. Rules other than 7 are enforced everywhere.
    """
    if rule == 7 and src is not None:
        return any(rule7_state(src))
    norm = rel.replace("\\", "/")
    return not any(rule == r and norm.startswith(prefix) for r, prefix, _why in EXEMPT)


def exemptions() -> list[dict]:
    """The `EXEMPT` table plus the per-file rule-7 keys, in the shape the JSON output reports it."""
    out = [{"rule": r, "prefix": p, "why": w} for r, p, w in EXEMPT]
    out += [{"rule": 7, "prefix": None, "condition": cond, "why": why} for cond, why in RULE7_NOTES]
    return out


def rule_counts(findings: list[dict]) -> dict[tuple[int, str], int]:
    out: dict[tuple[int, str], int] = {}
    for f in findings:
        out[(f["rule"], f["file"])] = out.get((f["rule"], f["file"]), 0) + 1
    return out


def diff_deltas(before: dict[tuple[int, str], int], after: dict[tuple[int, str], int]) -> list[dict]:
    """The (rule, file) pairs whose count rose, with the delta. Pure function - the `--diff` core."""
    added = []
    for key in sorted(set(before) | set(after)):
        delta = after.get(key, 0) - before.get(key, 0)
        if delta > 0:
            added.append({"rule": key[0], "file": key[1], "added": delta,
                          "before": before.get(key, 0), "after": after.get(key, 0)})
    return added


def git(root: str, *args: str) -> str:
    p = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), (p.stderr or "").strip()))
    return p.stdout


def git_bytes(root: str, *args: str) -> bytes:
    p = subprocess.run(["git", *args], cwd=root, capture_output=True)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), p.stderr.decode("utf-8", "replace").strip()))
    return p.stdout


def changed_src_files(root: str, ref: str) -> list[tuple[str | None, str]]:
    """`(path_at_ref, path_now)` for every `src/` file the tree changed against `ref`.

    A rename is one entry carrying both names, so the file's violations are compared against its old
    copy rather than counting as new.
    """
    out: list[tuple[str | None, str]] = []
    for line in git(root, "diff", "--name-status", "-M", "--diff-filter=d", ref, "--", SRC).splitlines():
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        status, paths = parts[0], parts[1:]
        after = paths[-1]
        if status.startswith("R"):
            before = paths[0]
        elif status.startswith("A"):
            before = None  # added by this batch: every violation in it is an addition
        else:
            before = after
        if after.endswith(SUFFIXES):
            out.append((before, after))
    return out


def findings_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]]) -> dict[tuple[int, str], int]:
    """Rule counts for the ref's copy of each file, keyed by the file's path *now*.

    A file absent at the ref (`before is None`) counts as zero: it is new, so every violation in it is an
    addition. Keying a rename by its new path keeps the two sides comparable.
    """
    findings = []
    for before, after in pairs:
        if before is None:
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, before)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        findings.extend(lint_source(Source(before, after, text)))
    return rule_counts(findings)


# --------------------------------------------------------------------------------------------------
# reporting
# --------------------------------------------------------------------------------------------------
def print_budget(findings: list[dict]) -> None:
    b = budget(findings)
    width = max([len(row["file"]) for row in b["units"]] + [len("TOTAL")])
    head = "".join("  r%d" % r for r in RULE_NAMES)
    print("%-*s  %4s%s" % (width, "unit", "tot", head))
    for row in b["units"]:
        print("%-*s  %4d%s" % (width, row["file"], row["total"],
                               "".join("  %2d" % row["rules"][str(r)] for r in RULE_NAMES)))
    print("%-*s  %4d%s" % (width, "TOTAL", b["total"],
                           "".join("  %2d" % b["totals"][str(r)] for r in RULE_NAMES)))
    print("")
    u = unique_names(findings)
    print("distinct names: rule 3 %d type(s), rule 5 %d field(s), rule 7 %d fn_* + %d unk identifier(s)"
          % (u["types"], u["unk_fields"], u["fn_names"], u["unk_identifiers"]))
    print("%d finding(s) over %d unit(s), %d file(s) with findings"
          % (b["findings"], len(b["units"]), len(source_files_of(findings))))
    for num, what in UNCHECKED:
        print("not checked (cross-file): rule %d - %s" % (num, what))
    for rule, prefix, why in EXEMPT:
        print("not enforced: rule %d under %s (%s)" % (rule, prefix, why))
    for cond, why in RULE7_NOTES:
        print("not enforced: rule 7 for %s (%s)" % (cond, why))


def source_files_of(findings: list[dict]) -> set[str]:
    return {f["file"] for f in findings}


def print_findings(findings: list[dict]) -> None:
    for f in findings:
        print("%s:%d: rule %d: %s [%s]" % (f["file"], f["line"], f["rule"], f["detail"], f["text"]))
    for num, what in UNCHECKED:
        print("not checked (cross-file): rule %d - %s" % (num, what))
    for rule, prefix, why in EXEMPT:
        print("not enforced: rule %d under %s (%s)" % (rule, prefix, why))
    for cond, why in RULE7_NOTES:
        print("not enforced: rule 7 for %s (%s)" % (cond, why))


# --------------------------------------------------------------------------------------------------
# selftest
# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def rules_of(text: str, rel: str = "x.c") -> list[tuple[int, int]]:
        return [(f["rule"], f["line"]) for f in lint_source(Source("x.c", rel, text))]

    def lines_of(text: str, rule: int, rel: str = "x.c") -> list[int]:
        return [f["line"] for f in lint_source(Source("x.c", rel, text)) if f["rule"] == rule]

    # --- stripping --------------------------------------------------------------------------------
    code, comm = strip("a /* goto */ b\nc // goto\nd \"goto\" 'x'\n")
    check("strip: block comment blanked", "goto" in code, False)
    check("strip: line comment blanked", code.count("goto"), 0)
    check("strip: string blanked", code.count("goto"), 0)
    check("strip: comment text kept", comm.count("goto"), 2)
    check("strip: length preserved", len(code), len(comm))
    check("strip: newlines preserved", code.count("\n"), 3)
    multi = "/* a\ngoto\nb */\nx\ngoto done;\n"
    check("strip: multi-line comment keeps line numbers", [f["line"] for f in lint_source(Source("x.c", "x.c", multi)) if f["rule"] == 8], [5])
    esc = 'char* s = "a \\" goto b";\ngoto x;\n'
    check("strip: escaped quote in a literal", lines_of(esc, 8), [2])
    check("strip: unterminated line literal stops at newline",
          lines_of('char c = \'a;\ngoto x;\n', 8), [2])
    check("strip: comment body has the annotation", bool(SIZE_RE.search(strip("/* size: 0x10 */\n")[1])), True)
    check("strip: annotation inside a string is not seen", bool(SIZE_RE.search(strip('"size: 0x10"\n')[1])), False)

    # --- rule 3: size -----------------------------------------------------------------------------
    check("rule3: annotated before the definition", lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 3), [])
    check("rule3: annotated after the closing brace", lines_of("struct A {\n    u32 x;\n};\n/* size: 0x8 */\n", 3), [])
    check("rule3: no annotation is a violation", lines_of("struct A {\n    u32 x;\n};\n", 3), [1])
    check("rule3: anonymous typedef gets the name after the brace",
          lines_of("typedef struct {\n    u32 x;\n} A;\n", 3), [1])
    check("rule3: annotation too far away", lines_of("/* size: 0x8 */\n\n\n\n\n\nstruct A {\n    u32 x;\n};\n", 3), [7])
    check("rule3: a forward declaration is not a definition", lines_of("struct A;\n", 3), [])
    check("rule3: a use in a parameter list is not a definition", lines_of("void f(struct A* a);\n", 3), [])
    check("rule3: two types, one annotation - only the near one is certified",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\nstruct B {\n    u32 y;\n};\n", 3), [5])
    check("rule3: class counts too", lines_of("class A {\n    u32 x;\n};\n", 3), [1])
    check("rule3: annotation in a string does not certify",
          lines_of('const char* s = "size: 0x8";\nstruct A {\n    u32 x;\n};\n', 3), [2])

    # --- rule 4/5: fields -------------------------------------------------------------------------
    check("rule4: canonical +0x annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 x;\n};\n", 4), [])
    check("rule4: the existing 0x annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* 0x00 */ u32 x;\n};\n", 4), [])
    check("rule4: trailing annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x; /* 0x00 - counter */\n};\n", 4), [])
    check("rule4: missing offset is a violation",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 4), [3])
    check("rule4: two fields, one annotated",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 x;\n    u32 y;\n};\n", 4), [4])
    check("rule4: function pointer field is a field",
          lines_of("/* size: 0x8 */\nstruct A {\n    void (*cb)(void);\n};\n", 4), [3])
    check("rule5: unk field is a violation",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00;\n};\n", 5), [3])
    check("rule5: pad_/unused_ pass",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u8 pad_0x00[4];\n    /* +0x04 */ u8 unused_0x04[4];\n};\n", 5), [])
    check("rule5: a named field passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 item_id;\n};\n", 5), [])
    check("rule5: unk in a comment does not count",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 item_id; /* was unk00 */\n};\n", 5), [])
    check("rule5: an array field name is found",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u8 unk00[4];\n};\n", 5), [3])
    check("rule5: a bitfield name is found",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00 : 3;\n};\n", 5), [3])
    check("rule4: only the top-level body counts, not a nested type",
          lines_of("/* size: 0x8 */\nstruct A {\n    struct B { /* +0x00 */ u32 z; } b;\n    /* +0x00 */ u32 x;\n};\n", 4), [])

    # --- rule 6: pointer arithmetic ---------------------------------------------------------------
    check("rule6: the classic field poke",
          lines_of("void f(u8* self) {\n    *(u32*)((u8*)self + 0x1C) = 1;\n}\n", 6), [2])
    check("rule6: (char*)obj + 0x10",
          lines_of("void f(void* obj) {\n    u32 v = *(u32*)((char*)obj + 0x10);\n}\n", 6), [2])
    check("rule6: decimal offset",
          lines_of("void f(void* obj) {\n    u32 v = *(u32*)((u8*)obj + 4);\n}\n", 6), [2])
    check("rule6: a member access is clean", lines_of("void f(A* self) {\n    self->field = 1;\n}\n", 6), [])
    check("rule6: memset is clean", lines_of("void f(void* p) {\n    memset(p, 0, 0x10);\n}\n", 6), [])
    check("rule6: offsetof is clean", lines_of("u32 n = offsetof(A, x);\n", 6), [])
    check("rule6: a cast with no offset is clean", lines_of("void f(void* p) {\n    u32 v = *(u32*)p;\n}\n", 6), [])
    check("rule6: memset's allowed byte range is clean",
          lines_of("void f(u8* p) {\n    memset((u8*)p + 4, 0, 8);\n}\n", 6), [])
    check("rule6: memcpy's allowed byte range is clean",
          lines_of("void f(u8* p, u8* q) {\n    memcpy(p, (u8*)q + 8, 4);\n}\n", 6), [])
    check("rule6: a cast offset passed to another callee is not allowed",
          lines_of("void f(u8* p) {\n    foo((u8*)p + 4);\n}\n", 6), [2])
    check("rule6: the memset exception does not cover a dereference",
          lines_of("void f(u8* p) {\n    memset(p, 0, *(u32*)((u8*)p + 4));\n}\n", 6), [2])
    check("rule6: inside a comment it is clean",
          lines_of("/* written as *(s16*)((u8*)self + 0x1C) */\nvoid f(void) {}\n", 6), [])
    check("rule6: two sites on one line are two findings",
          lines_of("void f(u8* p) {\n    a = *(u32*)((u8*)p + 4); b = *(u32*)((u8*)p + 8);\n}\n", 6), [2, 2])

    # --- rule 7: names ----------------------------------------------------------------------------
    check("rule7: fn_ name is a violation", lines_of("void fn_80040598(void) {}\n", 7), [1])
    check("rule7: a named function is clean", lines_of("void Pl_Skill_ck(void) {}\n", 7), [])
    check("rule7: bare unk local is a violation", lines_of("void f(void) {\n    u32 unk4 = 0;\n}\n", 7), [2])
    check("rule7: a field's unk is rule 5's, not rule 7's",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00;\n};\n", 7), [])
    check("rule7: unk in a comment does not count", lines_of("/* unk4 fn_80040598 */\nvoid f(void) {}\n", 7), [])
    check("rule7: unk in a string does not count", lines_of('const char* s = "unk4";\n', 7), [])
    check("rule7: a name that merely contains unk is clean", lines_of("void f(void) {\n    u32 junk = 0;\n}\n", 7), [])
    check("rule7: fn_ with the wrong digit count is clean", lines_of("void fn_1234(void) {}\n", 7), [])

    # --- rule 7 exemption under src/auto/ (docs/plan.md, "The breadth blocker") --------------------
    auto = "src/auto/802B2978_fn_802B2978.c"
    check("rule7 exempt: an auto file's own fn_ name is clean",
          lines_of("void fn_802B2978(void) {}\n", 7, auto), [])
    check("rule7 exempt: an auto body calling fn_ is clean",
          lines_of("void fn_802B2978(void) {\n    fn_80040598();\n}\n", 7, auto), [])
    check("rule7 exempt: a bare unk local is clean under src/auto/ too",
          lines_of("void fn_802B2978(void) {\n    u32 unk4 = 0;\n}\n", 7, auto), [])
    check("rule7 exempt: a subdirectory of src/auto/ is covered",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/auto/deep/x.c"), [])
    check("rule7 exempt: a path that merely contains auto is not covered",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/auto_tools/x.c"), [1])
    check("rule7 exempt: a sibling prefix is not covered",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/automaton/x.c"), [1])
    check("rule7 exempt: the same code under src/Pl/ is still a violation",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/Pl/pl_act.cpp"), [1])
    check("rule7 exempt: a src/Pl/ body calling fn_ is still a violation",
          lines_of("void Pl_x(void) {\n    fn_80040598();\n}\n", 7, "src/Pl/pl_act.cpp"), [2])
    check("rule7 exempt: only rule 7 is exempt - rule 4 still fires under src/auto/",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 4, auto), [3])
    check("rule7 exempt: rule 6 still fires under src/auto/",
          lines_of("void fn_802B2978(u8* p) {\n    *(u32*)((u8*)p + 4) = 1;\n}\n", 6, auto), [2])
    check("rule7 exempt: rule 8 still fires under src/auto/",
          lines_of("void fn_802B2978(void) {\n    goto out;\nout:\n    return;\n}\n", 8, auto), [2])
    check("rule7 keys: rule_enforced consults all three keys when given the file",
          [rule_enforced(7, "src/auto/x.c"),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c", "/* stub: only a header comment */\n")),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c", "void fn_80040598(void) {}\n")),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c",
                         "/* rule 7 deferred: the map has no name */\nvoid fn_80040598(void) {}\n")),
           rule_enforced(6, "src/auto/x.c"), rule_enforced(7, "src/auto\\x.c")],
          [False, False, True, True, True, False])
    check("rule7 keys: the temporary src/auto/ entry is still in the table",
          [(r, p) for r, p, _w in EXEMPT], [(7, "src/auto/")])

    # --- rule 7 key 2: no bodies yet (the re-key - the first landing at a final path) -------------
    pl = "src/Pl/pl_act.cpp"
    check("rule7 bodyless: a src/Pl fn_ prototype is clean",
          lines_of("void fn_802B2978(void);\n", 7, pl), [])
    check("rule7 bodyless: a src/Pl fn_ call with no body is clean",
          lines_of("fn_80040598();\n", 7, pl), [])
    check("rule7 bodyless: a src/Pl bare unk local is clean",
          lines_of("u32 unk4;\n", 7, pl), [])
    check("rule7 bodyless: adding a body and no declaration removes the exemption",
          lines_of("void fn_802B2978(void) {}\n", 7, pl), [1])
    check("rule7 bodyless: a bodyfull unit at another final path is a violation too",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/enemy/em_act.c"), [1])
    check("rule7 bodyless: rule 6 still fires without a body",
          lines_of("u32 v = *(u32*)((u8*)p + 4);\n", 6, pl), [1])
    check("rule7 bodyless: rule 8 still fires without a body",
          lines_of("goto out;\n", 8, pl), [1])
    check("rule7 unk: a bare unk local in a bodyfull, undeclared src/Pl file still violates",
          lines_of("void f(void) {\n    u32 unk4 = 0;\n}\n", 7, pl), [2])

    # --- rule 7 key 3: the per-unit `rule 7 deferred: <reason>` declaration ------------------------
    defer = "/* rule 7 deferred: the map has only fn_XXXXXXXX for this range */\n"
    check("rule7 deferred: a declared bodyfull src/Pl fn_ definition is clean",
          lines_of(defer + "void fn_802B2978(void) {}\n", 7, pl), [])
    check("rule7 deferred: a declared fn_ call inside a body is clean",
          lines_of(defer + "void f(void) {\n    fn_80040598();\n}\n", 7, pl), [])
    check("rule7 deferred: an empty reason does not defer",
          lines_of("/* rule 7 deferred: */\nvoid fn_802B2978(void) {}\n", 7, pl), [2])
    check("rule7 deferred: a declaration inside a string does not defer",
          lines_of('const char* s = "rule 7 deferred: x";\nvoid fn_802B2978(void) {}\n', 7, pl), [2])
    check("rule7 deferred: a declaration in a line comment defers",
          lines_of("// rule 7 deferred: the map has no name\nvoid fn_802B2978(void) {}\n", 7, pl), [])
    check("rule7 deferred: the declaration does not defer a bare unk local",
          lines_of(defer + "void f(void) {\n    u32 unk4 = 0;\n}\n", 7, pl), [3])
    check("rule7 deferred: rules 4/5/6/8 still fire in a deferred file",
          [r for r, _l in rules_of(
              defer
              + "/* size: 0x8 */\nstruct A {\n    u32 x;\n    /* +0x04 */ u32 unk04;\n};\n"
              + "void fn_802B2978(u8* p) {\n    *(u32*)((u8*)p + 4) = 1;\n    goto out;\nout:\n    return;\n}\n",
              pl)],
          [4, 5, 6, 8])

    # --- rule 8: goto -----------------------------------------------------------------------------
    check("rule8: goto is a violation", lines_of("void f(void) {\n    goto out;\nout:\n    return;\n}\n", 8), [2])
    check("rule8: goto in a comment is clean",
          lines_of("/* the old shape was a goto dispatch */\nvoid f(void) {}\n", 8), [])
    check("rule8: goto in a string is clean", lines_of('const char* s = "goto";\n', 8), [])
    check("rule8: a plain label is not reported by this check", lines_of("void f(void) {\nout:\n    return;\n}\n", 8), [])
    check("rule8: a name containing goto is clean", lines_of("void f(void) {\n    u32 gotot = 1;\n}\n", 8), [])

    # --- budget aggregation -----------------------------------------------------------------------
    text = ("/* size: 0x8 */\nstruct A {\n    u32 unk00;\n};\n"
            "void f(u8* p) {\n    *(u32*)((u8*)p + 0x4) = 1;\n}\n")
    f1 = lint_source(Source("a.c", "src/A/a.c", text))
    f2 = lint_source(Source("b.c", "src/B/b.c", "void fn_80040598(void) {}\n"))
    b = budget(f1 + f2)
    check("budget: two units", [row["file"] for row in b["units"]], ["src/A/a.c", "src/B/b.c"])
    check("budget: unit row total", b["units"][0]["total"], 3)
    check("budget: rule 5 counted once", b["totals"]["5"], 1)
    check("budget: rule 7 counted once", b["totals"]["7"], 1)
    check("budget: empty rule is zero", b["totals"]["8"], 0)
    check("budget: overall total", b["total"], 4)
    check("budget: units are sorted", b["units"] == sorted(b["units"], key=lambda r: r["file"]), True)
    check("budget: findings count", b["findings"], 4)
    check("budget: unique fn names", b["unique"]["fn_names"], 1)
    check("budget: unique unk fields", b["unique"]["unk_fields"], 1)
    check("budget: unique types", b["unique"]["types"], 0)
    check("unique: repeated sites collapse", unique_names([
        {"rule": 5, "detail": "field `unk1`"}, {"rule": 5, "detail": "field `unk1`"},
        {"rule": 7, "detail": "bare `unk1` identifier"},
    ])["unk_fields"], 1)

    # --- diff comparison --------------------------------------------------------------------------
    before = rule_counts(f1)
    after = rule_counts(f1 + f2)
    d = diff_deltas(before, after)
    check("diff: one rising pair", [(x["rule"], x["file"], x["added"]) for x in d], [(7, "src/B/b.c", 1)])
    check("diff: an unchanged tree is clean", diff_deltas(before, before), [])
    check("diff: a falling count is not an addition",
          diff_deltas({(5, "x"): 3}, {(5, "x"): 1}), [])
    check("diff: a fixed rule still fails for a different one",
          [(x["rule"]) for x in diff_deltas({(5, "x"): 1, (8, "x"): 0}, {(5, "x"): 1, (8, "x"): 1})], [8])

    # --- end-to-end over the fixtures -------------------------------------------------------------
    check("e2e: rule list is complete", sorted(RULE_NAMES), [3, 4, 5, 6, 7, 8])
    check("e2e: rules 1 and 2 are declared unchecked", [n for n, _ in UNCHECKED], [1, 2])
    check("e2e: findings sort by rule then line",
          rules_of(text), sorted(rules_of(text)))

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


# --------------------------------------------------------------------------------------------------
# cli
# --------------------------------------------------------------------------------------------------
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Lint src/ against docs/plan.md 6.5 (roadmap 7.21).")
    ap.add_argument("--diff", metavar="REF",
                    help="fail only if the working tree adds a violation relative to REF")
    ap.add_argument("--budget", action="store_true", help="report the backlog per unit over src/")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()

    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True)
    root = root.stdout.strip() if root.returncode == 0 else os.getcwd()

    if args.diff is not None:
        try:
            pairs = changed_src_files(root, args.diff)
            rels = [after for _before, after in pairs]
            before = findings_at_ref(root, args.diff, pairs)
        except RuntimeError as exc:
            print("stylelint: %s" % exc, file=sys.stderr)
            return 2
        after = rule_counts(lint_tree(root, [os.path.join(root, a) for _b, a in pairs]))
        added = diff_deltas(before, after)
        if args.json:
            print(json.dumps({"ref": args.diff, "added": added, "changed": rels,
                              "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                              "exempt": exemptions()}, indent=2))
        elif added:
            print("stylelint: the batch adds %d section 6.5 violation(s) over %d changed file(s):"
                  % (sum(a["added"] for a in added), len(rels)))
            for a in added:
                print("  +%d rule %d  %s  (%d -> %d)" % (a["added"], a["rule"], a["file"], a["before"], a["after"]))
            for num, what in UNCHECKED:
                print("  not checked (cross-file): rule %d - %s" % (num, what))
            for rule, prefix, why in EXEMPT:
                print("  not enforced: rule %d under %s (%s)" % (rule, prefix, why))
            for cond, why in RULE7_NOTES:
                print("  not enforced: rule 7 for %s (%s)" % (cond, why))
        else:
            print("stylelint: no new section 6.5 violation over %d changed file(s) "
                  "(rules 1-2 not checked: cross-file; rule 7 exempt per rule7_state)"
                  % len(rels))
        return 1 if added else 0

    findings = lint_tree(root)
    if args.json:
        print(json.dumps({"budget": budget(findings),
                          "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                          "exempt": exemptions()}, indent=2))
    elif args.budget:
        print_budget(findings)
    else:
        print_findings(findings)
    return 0


if __name__ == "__main__":
    sys.exit(main())
