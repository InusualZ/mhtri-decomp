"""One module owns every write to a shared file (docs/plan.md 7.12).

A shared file - `config/RMHE08/splits.txt`, `configure.py` - is read and rewritten by several tools, and
the write itself is the dangerous part. Two incidents say so: a CRLF file written back with LF silently
voided two edits (the anchor stopped matching), and an undeclared progress category in `configure.py`
failed the *next* `ninja`, not the edit. So every write goes through here:

* the file's line ending is preserved exactly - `read_text` reads with `newline=""`, `line_ending` picks
  the ending the file already has, and every insertion is normalised to it;
* every anchor a caller depends on is asserted *before* a byte is written - `insert_after_anchor` raises
  `AnchorError`, and `missing_anchors` lets a validator report instead of raising;
* a block already present is not appended twice - `append_blocks` is idempotent, and
  `insert_after_anchor(present=...)` is too;
* a range that overlaps an existing claim is refused before the write - `find_overlap` /
  `check_no_overlap`, the same collision `symbolpreflight` reports (its kinds 1/4/5/8);
* the edit is a temp-file + `os.replace` transaction - `Transaction` restores the previous bytes exactly
  on any failure and removes the temp files on both paths.

The next writers that should move here: `tools/symbols/symedit.py`
(writes `symbols.txt` with its own `.tmp` + `os.replace` and no anchor/overlap gate) and
`tools/units/dataqueue.py` (`write_queue` is a second temp+rename implementation). See
`.pi/notes/sharedfiles.md`.

    python tools/units/sharedfiles.py --selftest
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path

# Every write stages to `<name>.sharedfiles-tmp` beside its target and is renamed in. The suffix lives
# here so a selftest can sweep for leftovers.
TMP_SUFFIX = ".sharedfiles-tmp"

SPLIT_RANGE = re.compile(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)")


class AnchorError(Exception):
    """An anchor a write depends on is not in the file - refuse before touching a byte."""


class OverlapError(Exception):
    """An edit's range collides with a range the file already claims."""


@dataclass(frozen=True)
class Layout:
    """Where a batch writes: the two shared files and the root of the generated sources."""

    splits: Path
    configure: Path
    src: Path


def read_text(path: Path) -> str:
    """The file's text with its line endings intact - what goes back in on a rollback."""
    return open(path, encoding="utf-8", newline="").read()


def line_ending(text: str) -> str:
    """The ending the file already uses: CRLF if any line has one, else LF."""
    return "\r\n" if "\r\n" in text else "\n"


def with_ending(text: str, nl: str) -> str:
    """`text` with every line ending normalised to `nl` (LF text in, `nl` text out)."""
    return text.replace("\r\n", "\n").replace("\n", nl)


def missing_anchors(text: str, anchors) -> list[str]:
    """The anchors not present in `text` - for a validator that reports instead of raising."""
    return [a for a in anchors if a not in text]


def insert_after_anchor(text: str, anchor: str, insertion: str, present: str | None = None):
    """Insert `insertion` after the line `anchor`, once, in the file's own line ending.

    Returns `(new_text, inserted)`. Raises `AnchorError` when the anchor line is absent - a silently
    skipped edit is a unit that exists in `splits.txt` and nowhere else. `present` is a marker that
    makes the insert idempotent: when it is already in `text`, nothing changes.
    """
    nl = line_ending(text)
    marker = anchor + nl
    if marker not in text:
        raise AnchorError("%r not found (line ending %r?)" % (anchor, nl))
    if present is not None and present in text:
        return text, False
    return text.replace(marker, marker + nl + with_ending(insertion, nl), 1), True


def append_blocks(text: str, blocks):
    """Append the `(key, block)` pairs whose key is not already in `text`, once, in order.

    Returns `(new_text, appended)`. `new_text` is the input when nothing is appended, so re-applying a
    batch is a no-op. The file's line ending is preserved and every block is normalised to it.
    """
    fresh = [(k, b) for k, b in blocks if k not in text]
    if not fresh:
        return text, 0
    nl = line_ending(text)
    if not text.endswith(nl):
        text += nl
    body = "".join(with_ending(b, nl) for _, b in fresh)
    return text + nl + body, len(fresh)


def parse_ranges(text: str) -> list[tuple[str, str, int, int]]:
    """Every `start:`/`end:` range in a `splits.txt` text, as (unit, section, start, end)."""
    out, cur = [], None
    for line in text.splitlines():
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":"):
            cur = line.strip()[:-1]
            continue
        m = SPLIT_RANGE.match(line)
        if m and cur is not None:
            out.append((cur, m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
    return out


def overlaps(a: tuple[int, int], b: tuple[int, int]) -> bool:
    return a[0] < b[1] and b[0] < a[1]


def find_overlap(claimed, start: int, end: int, section: str | None = None,
                 unit: str | None = None):
    """The first claim in `claimed` overlapping `[start, end)`, or None.

    `claimed` is `parse_ranges`' shape. A claim by the same `unit`, in `.text`, with the identical
    range is the idempotent re-apply of the same block, not a collision. `section=None` checks every
    section. This is the collision `symbolpreflight` reports and the one a range must pass before it
    lands.
    """
    for u, sec, s, e in claimed:
        if section is not None and sec != section:
            continue
        if unit is not None and u == unit and sec == ".text" and (s, e) == (start, end):
            continue
        if s < end and start < e:
            return (u, sec, s, e)
    return None


def check_no_overlap(claimed, start: int, end: int, section: str | None = None,
                     unit: str | None = None) -> None:
    """Raise `OverlapError` when `[start, end)` collides with a claim - the pre-write gate."""
    hit = find_overlap(claimed, start, end, section, unit)
    if hit is not None:
        raise OverlapError("0x%08X..0x%08X overlaps %s's %s range 0x%08X..0x%08X"
                           % (start, end, hit[0], hit[1], hit[2], hit[3]))


class Transaction:
    """All-or-nothing writes over a fixed set of paths.

    Every target is staged to `<name>.sharedfiles-tmp` beside it and renamed in with `os.replace`, so a
    reader never sees a half-written file. If a write fails, `rollback()` puts every already-renamed
    target back to the bytes it had before - and deletes the ones that did not exist - newest first;
    `cleanup()` removes the temp files on both paths. `rename` is the injection point the selftest uses
    to fail a write on purpose; recovery deliberately does not go through it, because recovery must not
    be breakable by the fault it is recovering from.
    """

    def __init__(self, rename=None):
        self._rename = rename or os.replace
        self.prev: dict[Path, bytes | None] = {}
        self.order: list[Path] = []
        self.dirs: list[Path] = []
        self.temps: list[Path] = []

    def write(self, path: Path, text: str) -> None:
        if not path.parent.exists():
            made, d = [], path.parent
            while not d.exists() and d != d.parent:
                made.append(d)
                d = d.parent
            path.parent.mkdir(parents=True, exist_ok=True)
            self.dirs.extend(made)          # deepest first: rollback removes children before parents
        if path not in self.prev:
            self.prev[path] = path.read_bytes() if path.exists() else None
        tmp = path.with_name(path.name + TMP_SUFFIX)
        self.temps.append(tmp)
        tmp.write_bytes(text.encode("utf-8"))
        self._rename(tmp, path)
        self.order.append(path)

    def cleanup(self) -> None:
        for tmp in self.temps:
            try:
                tmp.unlink()
            except FileNotFoundError:
                pass

    def rollback(self) -> None:
        """Undo every rename already made, newest first, back to the exact previous bytes.

        The temp files go first: the write that failed left one in the directory this is about to
        remove, and a directory with a file in it cannot be removed. The restore then re-creates a temp
        per file, so a second `cleanup()` follows. Recovery uses `os.replace` directly - an injected
        fault must not be able to break the recovery from itself.
        """
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
        for d in self.dirs:                     # deepest first, so a child is gone before its parent
            try:
                d.rmdir()                       # only if empty: never destroy a pre-existing tree
            except OSError:
                pass


# --------------------------------------------------------------------------------------------------
# selftest
# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    import tempfile

    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def temps(root):
        return sorted(str(p) for p in Path(root).rglob("*" + TMP_SUFFIX))

    # --- line-ending round trip ---------------------------------------------------------------------
    for nl, name in (("\n", "LF"), ("\r\n", "CRLF")):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "splits.txt"
            base = "main/foo.c:%s\t.text       start:0x1000 end:0x1100%s" % (nl, nl)
            block = "auto/a.c:%s\t.text       start:0x2000 end:0x2100%s" % (nl, nl)
            path.write_bytes(base.encode("utf-8"))
            new, added = append_blocks(read_text(path), [("auto/a.c", block)])
            check("append %s: one block added" % name, added, 1)
            check("append %s: the old block survives" % name, "main/foo.c:" in new, True)
            check("append %s: the new block landed" % name, "auto/a.c:" in new, True)
            if nl == "\n":
                check("append %s: no CR appeared" % name, "\r" not in new, True)
            else:
                check("append %s: no bare LF appeared" % name, "\n" not in new.replace("\r\n", ""), True)
            path.write_bytes(new.encode("utf-8"))       # round-trip through the disk
            check("append %s: round-trip keeps the text" % name, read_text(path), new)
            check("append %s: round-trip keeps the ending" % name, line_ending(read_text(path)), nl)

    # --- a missing anchor refuses before any write --------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "configure.py"
        path.write_bytes(b"config.libs = [\r\n]\r\n")
        before = path.read_bytes()
        check("anchor: missing_anchors names what is absent",
              missing_anchors(read_text(path), ["config.libs = [", "config.progress_categories = ["]),
              ["config.progress_categories = ["])
        raised = False
        try:
            insert_after_anchor(read_text(path), "config.progress_categories = [", "    x,\r\n")
        except AnchorError:
            raised = True
        check("anchor: a missing anchor raises", raised, True)
        check("anchor: the file is untouched", path.read_bytes(), before)
        check("anchor: no temp file", temps(tmp), [])

    # --- idempotency --------------------------------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "splits.txt"
        base = "main/foo.c:\n\t.text       start:0x1000 end:0x1100\n"
        block = "auto/a.c:\n\t.text       start:0x2000 end:0x2100\n"
        path.write_bytes(base.encode("utf-8"))
        new, added = append_blocks(read_text(path), [("auto/a.c", block)])
        new2, added2 = append_blocks(new, [("auto/a.c", block)])
        check("append: re-applying adds nothing", added2, 0)
        check("append: the key appears once", new2.count("auto/a.c:"), 1)
        check("append: the text is unchanged", new2, new)
        conf = "config.libs = [\n]\n"
        conf2, ins = insert_after_anchor(conf, "config.libs = [", "    lib,\n", present="lib,")
        conf3, ins2 = insert_after_anchor(conf2, "config.libs = [", "    lib,\n", present="lib,")
        check("anchor: the first insert lands", ins, True)
        check("anchor: the second is a no-op", ins2, False)
        check("anchor: the marker appears once", conf3.count("lib,"), 1)
        check("anchor: the text is unchanged", conf3, conf2)

    with tempfile.TemporaryDirectory() as tmp:
        conf = "config.libs = [\r\n]\r\n"
        new, ins = insert_after_anchor(conf, "config.libs = [", "    lib,\r\n", present="lib,")
        check("anchor CRLF: the insert lands", ins, True)
        check("anchor CRLF: the lib line is there", "    lib,\r\n" in new, True)
        check("anchor CRLF: no bare LF appeared", "\n" not in new.replace("\r\n", ""), True)

    # --- overlap refusal ----------------------------------------------------------------------------
    ranges = parse_ranges("main/foo.c:\n\t.text       start:0x1000 end:0x1100\n")
    check("overlap: a range inside a claim is found", find_overlap(ranges, 0x1050, 0x1080),
          ("main/foo.c", ".text", 0x1000, 0x1100))
    check("overlap: a range clear of every claim passes", find_overlap(ranges, 0x2000, 0x2100), None)
    check("overlap: an exact re-apply of the same unit is not a collision",
          find_overlap(ranges, 0x1000, 0x1100, unit="main/foo.c"), None)
    check("overlap: a different section is filtered out",
          find_overlap(ranges, 0x1050, 0x1080, section=".sdata"), None)
    raised = False
    try:
        check_no_overlap(ranges, 0x1080, 0x1200)
    except OverlapError:
        raised = True
    check("overlap: check_no_overlap raises", raised, True)

    # --- rollback restores the previous bytes exactly -----------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        a, b = root / "a.txt", root / "b.txt"
        a.write_bytes(b"one\r\n")

        def flaky(src, dst):
            if Path(dst).name == "b.txt":
                raise OSError("injected failure")
            os.replace(src, dst)

        tx = Transaction(rename=flaky)
        try:
            tx.write(a, "one\r\ntwo\r\n")
            tx.write(b, "b\r\n")
        except OSError:
            tx.rollback()
        finally:
            tx.cleanup()
        check("rollback: the first file is restored byte for byte", a.read_bytes(), b"one\r\n")
        check("rollback: the second file was never created", b.exists(), False)
        check("rollback: no temp files left", temps(tmp), [])

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    return ap


def main() -> int:
    args = build_parser().parse_args()
    if args.selftest:
        return selftest()
    build_parser().print_help()
    return 2


if __name__ == "__main__":
    sys.exit(main())
