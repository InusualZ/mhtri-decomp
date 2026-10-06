"""The runtime dump's Dolphin symbol map (`DumpSymbols.zip`): parse it and index it by address.
Spec: docs/tools/spec/lib-dumpsyms.md. CLI: none (library; `tools/symbols/dumpmap.py` is its command line)."""
from __future__ import annotations

import collections
import os
import re
import zipfile

DEFAULT_DUMP = os.environ.get("MHTRI_DUMP_SYMBOLS", "D:/WiiExperiment/DumpSymbols.zip")

# `NAME ADDR FLAG`, the flag glued to the next entry's name when the dumper lost a newline.
ENTRY_RE = re.compile(r"(?P<name>\S.*?)\s+(?P<addr>[0-9a-fA-F]{8})\s+(?P<flag>[fl])")
ZZ_RE = re.compile(r"^zz_[0-9a-fA-F]{6,8}_?$")
FUN_RE = re.compile(r"^FUN_[0-9a-fA-F]{6,8}$")
# Ghidra's auto-names: `<kind>_<address>`.  The address tail is checked separately as well, because
# `s_<text>_<addr>` and `u_MonsterHunter3_8056f4a0` carry more than the address.
GHIDRA_RE = re.compile(r"^(?:word|byte|dword|qword|u|s|FLOAT|DOUBLE|BOOL|BYTE|switchdataD|switchdata"
                       r"|DAT|PTR|LAB|UNK|off|field|zz|FUN)_[0-9a-fA-F]{6,8}_?$")


def base_name(name: str) -> str:
    """The name without the demangled argument list and without MWCC's `__F...` mangling suffix."""
    n = name.split("(", 1)[0]
    i = n.find("__F")
    if i > 0:
        n = n[:i]
    return n


def is_placeholder(name: str, address: int) -> bool:
    """True when the dump's name is not a real name: `zz_`, `FUN_`, or a Ghidra auto-name.

    Ghidra labels unnamed data `s_<text>_<addr>`, `FLOAT_<addr>`, `u_<addr>`, `switchdataD_<addr>`
    and unnamed code `FUN_<addr>`.  The `<prefix>_<8hex>` shape covers most of them; the extra
    address-tail test catches `s_<text>_<addr>` and the dumper's `zz_005b988_` spelling, and is
    what tells a real `_savefpr_14` apart from a generated name.  A `word_<addr>` that does not
    match its own address is still a Ghidra name (the dumper attached it to the wrong address),
    so the prefix list is checked unconditionally.
    """
    if ZZ_RE.match(name) or FUN_RE.match(name) or GHIDRA_RE.match(name):
        return True
    tail = name.rsplit("_", 1)[-1]
    if len(tail) >= 6 and re.fullmatch(r"[0-9a-fA-F]{6,8}", tail):
        try:
            return (int(tail, 16) & 0xFFFFFF) == (address & 0xFFFFFF)
        except ValueError:
            return False
    return False


def parse_map_text(text: str) -> list[dict]:
    """Parse the Dolphin `.map` member into entries (a line may hold one or two of them)."""
    out = []
    for lineno, line in enumerate(text.splitlines(), 1):
        i = 0
        while i < len(line):
            m = ENTRY_RE.match(line, i)
            if not m:
                break
            name = m.group("name").strip()
            address = int(m.group("addr"), 16)
            head = name.split("(", 1)[0]
            out.append({
                "name": name,
                "address": address,
                "flag": m.group("flag"),
                "lineno": lineno,
                "mangled": "__F" in name,
                "placeholder": is_placeholder(name, address),
                "clean": base_name(name),
                "signature": name[len(head):] or "",
            })
            i = m.end()
    return out


def index_by_address(entries: list[dict]) -> dict[int, list[dict]]:
    index: dict[int, list[dict]] = collections.defaultdict(list)
    for e in entries:
        index[e["address"]].append(e)
    return index


def load_dump(path: str = DEFAULT_DUMP, member: str | None = None) -> tuple[dict[int, list[dict]], str]:
    """Read the dump's `.map` member into an address index.  Raises SystemExit on a bad path."""
    if not os.path.exists(path):
        raise SystemExit("dump not found: %s (see docs/memory-dump.md; override with --dump)" % path)
    with zipfile.ZipFile(path) as z:
        members = z.namelist()
        if member is None:
            maps = [n for n in members if n.lower().endswith(".map")]
            if not maps:
                raise SystemExit("no .map member in %s: %s" % (path, ", ".join(members)))
            member = maps[0]
        if member not in members:
            raise SystemExit("member %r not in %s: %s" % (member, path, ", ".join(members)))
        text = z.read(member).decode("latin-1")
    return index_by_address(parse_map_text(text)), member
