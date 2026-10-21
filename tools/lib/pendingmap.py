"""Pending `symbols.txt` edits (renames, row splits) applied to a split target object, so a unit is scored as the map
will read once the edits land. Spec: docs/tools/spec/lib-pendingmap.md. CLI: none (library)."""
from __future__ import annotations

import os
import struct
from dataclasses import dataclass, field

from tools.lib.binary.elf import SHN_UNDEF, STB_GLOBAL, STB_LOCAL, STB_WEAK, Elf

#: `--scope` words of a `split` line -> the ELF binding of the new symbol (None: the row's own binding)
SCOPES = {"global": STB_GLOBAL, "local": STB_LOCAL, "weak": STB_WEAK}
_SYM = struct.Struct(">IIIBBH")


@dataclass(frozen=True)
class Op:
    """One pending edit: `rename` (`old` -> `new`) or `split` (`old` shrinks to `offset` bytes, `new` takes the rest)."""
    kind: str
    old: str
    new: str
    offset: int = 0
    scope: str | None = None
    line: int = 0


@dataclass
class Outcome:
    """What `apply` did: `renamed` `(old, new, entries)`, `split` `(row, new, offset)`, `absent` the names this object
    neither defines nor references (normal for another unit's rename), `refused` `(line, reason)`."""
    renamed: list = field(default_factory=list)
    split: list = field(default_factory=list)
    absent: list = field(default_factory=list)
    refused: list = field(default_factory=list)

    def summary(self) -> str:
        return ("%d rename(s) applied, %d split(s), %d name(s) not in the object, %d refused"
                % (len(self.renamed), len(self.split), len(self.absent), len(self.refused)))

    def to_json(self) -> dict:
        return {"renamed": [list(r) for r in self.renamed], "split": [list(s) for s in self.split],
                "absent": list(self.absent), "refused": [list(r) for r in self.refused]}


def parse(text: str) -> list[Op]:
    """The ops of a pending-map file, in file order. A line is `old new` (the `symedit.py rename-batch` shape),
    `rename old new`, or `split <row> <offset> <new-name> [--scope S]` (`symedit.py split`'s arguments); `#` starts
    a comment and blank lines are skipped. Anything else raises `ValueError` naming the line."""
    ops: list[Op] = []
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        words = line.split()
        if words[0] == "rename":
            words = words[1:]
        if words[0] == "split":
            rest, scope = words[1:], None
            if "--scope" in rest:
                i = rest.index("--scope")
                if i + 1 >= len(rest) or rest[i + 1] not in SCOPES:
                    raise ValueError("line %d: --scope wants one of %s" % (number, ", ".join(sorted(SCOPES))))
                scope = rest[i + 1]
                rest = rest[:i] + rest[i + 2:]
            if len(rest) != 3:
                raise ValueError("line %d: want `split <row> <offset> <new-name> [--scope S]`: %r" % (number, raw))
            try:
                offset = int(rest[1], 0)
            except ValueError:
                raise ValueError("line %d: offset %r is not a number" % (number, rest[1])) from None
            ops.append(Op("split", rest[0], rest[2], offset, scope, number))
        elif len(words) == 2:
            ops.append(Op("rename", words[0], words[1], line=number))
        else:
            raise ValueError("line %d: want `old new`, `rename old new` or `split ...`: %r" % (number, raw))
    return ops


def load(path: str) -> list[Op]:
    with open(path, encoding="utf-8-sig") as fh:
        return parse(fh.read())


@dataclass
class _Sym:
    name: str
    value: int
    size: int
    info: int
    other: int
    shndx: int
    changed: bool = False


def apply(raw: bytes, ops: list[Op]) -> tuple[bytes, Outcome]:
    """`(object bytes, Outcome)`: `raw` with the ops applied to its symbol table, in order.

    A rename renames every `.symtab` entry called `old` (defined or undefined, so relocations follow); it is refused
    when `old` and `new` are both defined here. A split shrinks the one defined `old` to `offset` bytes and appends
    `new` at `old + offset` with the remainder, in the same section and of the same type (binding: `--scope`, else
    the row's). Section contents and relocations are never touched: the grown `.strtab`/`.symtab` are appended at
    the end of the file and their section headers re-pointed, so no other offset moves."""
    elf = Elf.read(raw).require_be32()
    st = elf.symtab
    if st is None or not (0 <= st.link < len(elf.sections)):
        raise ValueError("the object has no .symtab with a string table")
    syms = [_Sym(s.name, s.value, s.size, s.info, s.other, s.shndx) for s in elf.symbols]
    first_new = len(syms)
    out = Outcome()
    for op in ops:
        named = [s for s in syms[1:] if s.name == op.old]
        defined = [s for s in named if s.shndx != SHN_UNDEF]
        if op.kind == "rename":
            if not named:
                out.absent.append(op.old)
                continue
            if defined and any(s.name == op.new and s.shndx != SHN_UNDEF for s in syms[1:]):
                out.refused.append((op.line, "%s -> %s: both are defined in this object" % (op.old, op.new)))
                continue
            for s in named:
                s.name, s.changed = op.new, True
            out.renamed.append((op.old, op.new, len(named)))
            continue
        if not defined:
            out.absent.append(op.old)
            continue
        if len(defined) > 1:
            out.refused.append((op.line, "split %s: defined %d times here" % (op.old, len(defined))))
            continue
        row = defined[0]
        if not 0 < op.offset < row.size:
            out.refused.append((op.line, "split %s: offset %#x is not inside its %#x bytes" % (op.old, op.offset, row.size)))
            continue
        if any(s.name == op.new and s.shndx != SHN_UNDEF for s in syms[1:]):
            out.refused.append((op.line, "split %s: %s is already defined here" % (op.old, op.new)))
            continue
        bind = SCOPES[op.scope] if op.scope else row.info >> 4
        syms.append(_Sym(op.new, row.value + op.offset, row.size - op.offset, (bind << 4) | (row.info & 0xF),
                         row.other, row.shndx, True))
        row.size, row.changed = op.offset, True
        out.split.append((op.old, op.new, op.offset))
    if not any(s.changed for s in syms):
        return bytes(raw), out
    strtab = elf.sections[st.link]
    names = bytearray(strtab.data)
    offsets = {}
    for s in elf.symbols:
        offsets[s.index] = struct.unpack_from(">I", raw, s.header)[0]
    table = bytearray()
    for i, s in enumerate(syms):
        if i >= first_new or s.changed:
            name_off = len(names)
            names += s.name.encode("latin-1") + b"\0"
        else:
            name_off = offsets[i]
        table += _SYM.pack(name_off, s.value, s.size, s.info, s.other, s.shndx)
    data = bytearray(raw)
    data += b"\0" * (-len(data) % 4)
    str_off = len(data)
    data += names
    data += b"\0" * (-len(data) % 4)
    sym_off = len(data)
    data += table
    struct.pack_into(">II", data, strtab.header + 16, str_off, len(names))
    struct.pack_into(">II", data, st.header + 16, sym_off, len(table))
    return bytes(data), out


def apply_file(src: str, dst: str, ops: list[Op]) -> Outcome:
    """`apply` from the object at `src` to `dst` (directories created); `dst` must not be `src`."""
    if os.path.abspath(src) == os.path.abspath(dst):
        raise ValueError("refusing to rewrite the split object in place: %s" % src)
    with open(src, "rb") as fh:
        blob, out = apply(fh.read(), ops)
    os.makedirs(os.path.dirname(os.path.abspath(dst)), exist_ok=True)
    with open(dst, "wb") as fh:
        fh.write(blob)
    return out
