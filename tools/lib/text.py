"""Bytes, line endings, byte-exact replace, atomic writes, anchors and transactions on shared files.
Spec: docs/tools/spec/lib-text.md. CLI: none (library)."""
from __future__ import annotations

import os
import re
import stat
import tempfile
from pathlib import Path
from typing import Callable, Iterable

#: Every `Transaction` write stages to `<name>` + this suffix beside its target and is renamed in.
TMP_SUFFIX = ".sharedfiles-tmp"


class AnchorError(Exception):
    """An anchor a write depends on is not in the file - refuse before touching a byte."""


class MatchCountError(ValueError):
    """`replace_bytes` found 0 or more than the asserted number of matches; nothing was replaced."""

    def __init__(self, expected: int, lines: list[int]) -> None:
        self.expected, self.lines = expected, lines
        super().__init__("expected %d match(es), found %d%s" % (
            expected, len(lines), " at line(s) " + ", ".join(map(str, lines)) if lines else ""))


# --- endings ------------------------------------------------------------------------------------------------

def ending_counts(data: bytes) -> dict[str, int]:
    """`{"crlf", "lf", "lone_cr"}`: `lf` is a bare `\\n` (not part of a CRLF), `lone_cr` a `\\r` with no `\\n`."""
    crlf = data.count(b"\r\n")
    return {"crlf": crlf, "lf": data.count(b"\n") - crlf, "lone_cr": data.count(b"\r") - crlf}


def endings(data: bytes, lone_cr: bool = True) -> str:
    """'lf', 'crlf', 'cr', 'mixed' or 'none' (no line break).

    With `lone_cr=False` a bare CR is not a line break (the `edit.py` reading: a file is classed by its
    `\\r\\n` against its bare `\\n` only).
    """
    counts = ending_counts(data)
    kinds = [k for k, key in (("crlf", "crlf"), ("lf", "lf"), ("cr", "lone_cr"))
             if counts[key] and (lone_cr or key != "lone_cr")]
    return "none" if not kinds else kinds[0] if len(kinds) == 1 else "mixed"


def dominant(data: bytes) -> bytes:
    """The ending most lines use: CRLF when `\\r\\n` outnumbers bare `\\n`, else LF."""
    crlf = data.count(b"\r\n")
    return b"\r\n" if crlf > data.count(b"\n") - crlf else b"\n"


def to_lf(data: bytes) -> bytes:
    return data.replace(b"\r\n", b"\n")


def to_ending(data: bytes, ending: bytes) -> bytes:
    """`data` with every `\\r\\n`/`\\n` normalised to `ending`."""
    data = to_lf(data)
    return data if ending == b"\n" else data.replace(b"\n", ending)


def line_ending(text: str) -> str:
    """The ending a text already uses: CRLF if any line has one, else LF."""
    return "\r\n" if "\r\n" in text else "\n"


def with_ending(text: str, nl: str) -> str:
    """`text` with every line ending normalised to `nl`."""
    return text.replace("\r\n", "\n").replace("\n", nl)


def read_text(path: str | os.PathLike) -> str:
    """A file's text with its line endings intact (`newline=""`)."""
    with open(path, encoding="utf-8", newline="") as fh:
        return fh.read()


# --- byte-exact replace -------------------------------------------------------------------------------------

def find_matches(data: bytes, old: bytes) -> list[tuple[int, int, int]]:
    """`[(start, end, 1-based line)]` of the non-overlapping matches of `old`, across `\\n` or `\\r\\n`."""
    if not old:
        raise ValueError("the old text is empty")
    pat = re.compile(b"\r?\n".join(re.escape(x) for x in to_lf(old).split(b"\n")))
    return [(m.start(), m.end(), data.count(b"\n", 0, m.start()) + 1) for m in pat.finditer(data)]


def replace_bytes(data: bytes, old: bytes, new: bytes, count: int = 1) -> tuple[bytes, list[int]]:
    """(new bytes, match line numbers): each match takes `new` in the ending of the span it replaces.

    A span with no line break takes the data's dominant ending. 0 or more than `count` matches raise
    `MatchCountError` (with the match lines) and nothing is replaced.
    """
    hits = find_matches(data, old)
    lines = [h[2] for h in hits]
    if len(hits) != count:
        raise MatchCountError(count, lines)
    fallback = dominant(data)
    out, pos = [], 0
    for start, end, _line in hits:
        span = data[start:end]
        ending = b"\r\n" if b"\r\n" in span else b"\n" if b"\n" in span else fallback
        out += [data[pos:start], to_ending(new, ending)]
        pos = end
    out.append(data[pos:])
    return b"".join(out), lines


# --- writes -------------------------------------------------------------------------------------------------

def _mode_for(path: str) -> int:
    """The permission bits a rewrite keeps: the file's own, else what `open(path, "w")` would give."""
    try:
        return stat.S_IMODE(os.stat(path).st_mode)
    except OSError:
        umask = os.umask(0)
        os.umask(umask)
        return 0o666 & ~umask


def atomic_write(path: str | os.PathLike, data: bytes | str, append: bool = False) -> None:
    """Write `data` byte-exactly (str is UTF-8, never newline-translated): a unique temp file beside the
    target, then `os.replace`, so a reader sees the old bytes or the new ones and two writers never share a
    temp file. `append` appends in place to an existing file (a missing one is created atomically).
    Parent directories are created; the temp file never outlives a failure.
    """
    path = os.path.abspath(os.fspath(path))
    raw = data.encode("utf-8") if isinstance(data, str) else data
    parent = os.path.dirname(path)
    os.makedirs(parent, exist_ok=True)
    if append and os.path.exists(path):
        with open(path, "ab") as fh:
            fh.write(raw)
        return
    fd, tmp = tempfile.mkstemp(dir=parent, prefix="." + os.path.basename(path) + ".", suffix=".tmp")
    try:
        with os.fdopen(fd, "wb") as fh:
            fh.write(raw)
        os.chmod(tmp, _mode_for(path))
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)


class Transaction:
    """All-or-nothing writes: each target is staged to `<name>.sharedfiles-tmp` and renamed in.

    `rollback()` puts every renamed target back to its previous bytes (deleting the ones that did not
    exist and the directories the transaction created), newest first; `cleanup()` removes temp files.
    `rename` is the fault-injection seam; recovery never goes through it.
    """

    def __init__(self, rename: Callable | None = None) -> None:
        self._rename = rename or os.replace
        self.prev: dict[Path, bytes | None] = {}
        self.order: list[Path] = []
        self.dirs: list[Path] = []
        self.temps: list[Path] = []

    def write(self, path: str | os.PathLike, text: str | bytes) -> None:
        path = Path(path)
        if not path.parent.exists():
            made, d = [], path.parent
            while not d.exists() and d != d.parent:
                made.append(d)
                d = d.parent
            path.parent.mkdir(parents=True, exist_ok=True)
            self.dirs.extend(made)
        if path not in self.prev:
            self.prev[path] = path.read_bytes() if path.exists() else None
        tmp = path.with_name(path.name + TMP_SUFFIX)
        self.temps.append(tmp)
        tmp.write_bytes(text.encode("utf-8") if isinstance(text, str) else text)
        self._rename(tmp, path)
        self.order.append(path)

    def cleanup(self) -> None:
        for tmp in self.temps:
            try:
                tmp.unlink()
            except FileNotFoundError:
                pass

    def rollback(self) -> None:
        self.cleanup()
        for path in reversed(self.order):
            prev = self.prev[path]
            try:
                if prev is None:
                    path.unlink(missing_ok=True)
                else:
                    tmp = path.with_name(path.name + TMP_SUFFIX)
                    tmp.write_bytes(prev)
                    os.replace(tmp, path)
            except OSError:
                pass
        self.cleanup()
        for d in self.dirs:
            try:
                d.rmdir()
            except OSError:
                pass


# --- anchors ------------------------------------------------------------------------------------------------

def missing_anchors(text: str, anchors: Iterable[str]) -> list[str]:
    """The anchors not present in `text`."""
    return [a for a in anchors if a not in text]


def insert_after_anchor(text: str, anchor: str, insertion: str, present: str | None = None) -> tuple[str, bool]:
    """`(new text, inserted)`: `insertion` after the line `anchor`, once, in the text's own ending.

    Raises `AnchorError` when the anchor line is absent; `present` (a marker already in `text`) makes it
    a no-op.
    """
    nl = line_ending(text)
    marker = anchor + nl
    if marker not in text:
        raise AnchorError("%r not found (line ending %r?)" % (anchor, nl))
    if present is not None and present in text:
        return text, False
    return text.replace(marker, marker + nl + with_ending(insertion, nl), 1), True


def append_blocks(text: str, blocks: Iterable[tuple[str, str]]) -> tuple[str, int]:
    """`(new text, appended)`: the `(key, block)` pairs whose key is absent, once, in the text's ending."""
    fresh = [(k, b) for k, b in blocks if k not in text]
    if not fresh:
        return text, 0
    nl = line_ending(text)
    if not text.endswith(nl):
        text += nl
    return text + nl + "".join(with_ending(b, nl) for _, b in fresh), len(fresh)


# --- C string-literal escapes -------------------------------------------------------------------------------

_SHORT = {"\\": "\\\\", '"': '\\"', "\n": "\\n", "\t": "\\t", "\r": "\\r",
          "\a": "\\a", "\b": "\\b", "\f": "\\f", "\v": "\\v"}
_UNSHORT = {"n": b"\n", "t": b"\t", "r": b"\r", "a": b"\a", "b": b"\b", "f": b"\f", "v": b"\v",
            "\\": b"\\", '"': b'"', "'": b"'", "0": b"\0"}
_OCTAL = "01234567"


def c_escape(text: str) -> str:
    """`raw text` -> a C string literal body (no quotes); a character outside printable ASCII becomes
    `\\xHH` of its code point, everything `c_unescape` reads back byte for byte."""
    out = []
    for ch in text:
        if ch in _SHORT:
            out.append(_SHORT[ch])
        elif 0x20 <= ord(ch) < 0x7F:
            out.append(ch)
        else:
            out.append("\\x%02x" % ord(ch))
    return "".join(out)


def c_unescape(text: str) -> bytes:
    """A C string literal body -> the exact bytes (`\\n`, `\\t`, `\\xHH`, `\\NNN` all understood).

    An unknown escape is kept as the two literal characters it is - never silently dropped - so a typo
    surfaces in the output instead of vanishing.
    """
    out = bytearray()
    i, n = 0, len(text)
    while i < n:
        ch = text[i]
        if ch != "\\" or i + 1 >= n:
            out += ch.encode("utf-8")
            i += 1
            continue
        nxt = text[i + 1]
        if nxt in _UNSHORT:
            out += _UNSHORT[nxt]
            i += 2
        elif nxt == "x":
            j = i + 2
            while j < n and j < i + 4 and text[j] in "0123456789abcdefABCDEF":
                j += 1
            if j == i + 2:
                out += b"\\x"          # a bare `\x`: keep it, do not eat the following character
                i += 2
            else:
                out.append(int(text[i + 2:j], 16) & 0xFF)
                i = j
        elif nxt in _OCTAL:
            j = i + 1
            while j < n and j < i + 4 and text[j] in _OCTAL:
                j += 1
            out.append(int(text[i + 1:j], 8) & 0xFF)
            i = j
        else:
            out += ("\\" + nxt).encode("utf-8")
            i += 2
    return bytes(out)
