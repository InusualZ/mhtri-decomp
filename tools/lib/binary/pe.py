"""PE32 images (the Metrowerks compilers and linkers): sections, RVA mapping, data directories, imports, resources, CodeView.
Spec: docs/tools/spec/lib-binary.md. CLI: none (library)."""
from __future__ import annotations

import os
import struct
from dataclasses import dataclass
from pathlib import Path

#: The data-directory indices this reader walks.
DIR_IMPORT, DIR_RESOURCE, DIR_DEBUG = 1, 2, 6
#: `IMAGE_FILE_MACHINE_I386`, the machine of every Metrowerks host binary in the tree.
MACHINE_I386 = 0x014C
#: `RT_STRING`: the resource type that holds a Windows string table (16 strings per block).
RT_STRING = 6


@dataclass(frozen=True)
class Section:
    """One PE section header; `index` is 1-based, as the CodeView records number sections."""
    name: str
    va: int
    vsize: int
    raw_off: int
    raw_size: int
    flags: int
    index: int = 0

    def __repr__(self) -> str:
        return f"<{self.name} va={self.va:#x} vs={self.vsize:#x}>"


class Pe:
    """A parsed PE32 image. Refuses (ValueError) a file with no `MZ`, no `PE\\0\\0` signature or a non-PE32
    optional header; everything past the headers is read lazily and leniently."""

    def __init__(self, path: str | os.PathLike) -> None:
        self.path = Path(path)
        self.data = self.path.read_bytes()
        d = self.data
        if d[:2] != b"MZ":
            raise ValueError(f"{self.path} is not a PE image (no MZ)")
        e = struct.unpack_from("<I", d, 0x3C)[0]
        if d[e:e + 4] != b"PE\0\0":
            raise ValueError(f"{self.path} has no PE signature")
        self.pe_off = e
        self.machine = struct.unpack_from("<H", d, e + 4)[0]
        nsec = struct.unpack_from("<H", d, e + 6)[0]
        optsz = struct.unpack_from("<H", d, e + 20)[0]
        opt = e + 24
        magic = struct.unpack_from("<H", d, opt)[0]
        if magic != 0x10B:
            raise ValueError(f"{self.path} is not PE32 (magic {magic:#x})")
        self.image_base = struct.unpack_from("<I", d, opt + 28)[0]
        self.entry = struct.unpack_from("<I", d, opt + 16)[0]
        self.size_image = struct.unpack_from("<I", d, opt + 56)[0]
        self.dll_characteristics = struct.unpack_from("<H", d, opt + 70)[0]
        self.opt_off = opt
        self.optsz = optsz
        sections = []
        so = opt + optsz
        for i in range(nsec):
            s = so + 40 * i
            name = d[s:s + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, raw_size, raw_off = struct.unpack_from("<IIII", d, s + 8)
            flags = struct.unpack_from("<I", d, s + 36)[0]
            sections.append(Section(name, va, vsize, raw_off, raw_size, flags, i + 1))
        self.sections: list[Section] = sections
        self._codeview: list[tuple[int, str, str]] | None = None

    # ---- addressing ------------------------------------------------------------------------------------------
    def section(self, name: str) -> Section | None:
        """The first section called `name`, or None."""
        for s in self.sections:
            if s.name == name:
                return s
        return None

    def rva2off(self, rva: int) -> int | None:
        """The file offset of `rva`, or None when no section backs it with file data (`.bss`-like tails)."""
        for s in self.sections:
            if s.va <= rva < s.va + max(s.vsize, s.raw_size):
                if rva - s.va >= s.raw_size:
                    return None
                return s.raw_off + (rva - s.va)
        return None

    def read(self, rva: int, n: int) -> bytes | None:
        """`n` bytes at `rva`, or None when `rva` is not backed by file data."""
        o = self.rva2off(rva)
        return None if o is None else self.data[o:o + n]

    def read_rva(self, rva: int, n: int) -> bytes:
        """`n` bytes at `rva`; ValueError when `rva` is not backed by file data."""
        o = self.rva2off(rva)
        if o is None:
            raise ValueError(f"RVA {rva:#x} is not backed by file data")
        return self.data[o:o + n]

    def cstring(self, rva: int) -> str | None:
        """The NUL-terminated latin-1 string at `rva`, or None when `rva` is not backed."""
        o = self.rva2off(rva)
        if o is None:
            return None
        end = self.data.find(b"\0", o)
        return self.data[o:end].decode("latin-1")

    # ---- data directories ------------------------------------------------------------------------------------
    def data_dir(self, index: int) -> tuple[int, int]:
        """`(rva, size)` of optional-header data directory `index`."""
        return struct.unpack_from("<II", self.data, self.opt_off + 96 + 8 * index)

    def debug_entries(self) -> list[dict]:
        """The debug directory's entries (`type`, `size`, `addr`, `ptr`, `char`); empty for every mwldeppc.exe."""
        rva, size = self.data_dir(DIR_DEBUG)
        if not rva or not size:
            return []
        o = self.rva2off(rva)
        if o is None:
            return []
        out = []
        for i in range(size // 28):
            ch, _ts, _maj, _mnr, typ, dsize, daddr, ptr = struct.unpack_from("<IIHHIIII", self.data, o + 28 * i)
            out.append({"type": typ, "size": dsize, "addr": daddr, "ptr": ptr, "char": ch})
        return out

    def debug_blob(self) -> bytes | None:
        """The bytes the first debug-directory entry points at (the CodeView blob), or None."""
        rva, _size = self.data_dir(DIR_DEBUG)
        if rva == 0:
            return None
        o = self.rva2off(rva)
        if o is None:
            return None
        _c, _t, _maj, _min, _typ, dsize, _a, ptr = struct.unpack_from("<IIHHIIII", self.data, o)
        return self.data[ptr:ptr + dsize]

    def codeview_symbols(self) -> list[tuple[int, str, str]]:
        """`[(rva, name, section name)]` sorted, from the CodeView `NB11` blob several mwcceppc.exe builds ship;
        `[]` when the image has none (every mwldeppc.exe)."""
        if self._codeview is not None:
            return self._codeview
        blob = self.debug_blob()
        recs = []
        if blob and blob[:4] == b"NB11":
            i, n = 0, len(blob)
            while i < n - 12:
                ln = struct.unpack_from("<H", blob, i)[0]
                if 8 <= ln <= 0x400 and i + 2 + ln <= n:
                    typ = struct.unpack_from("<H", blob, i + 2)[0]
                    zero = struct.unpack_from("<I", blob, i + 4)[0]
                    if zero == 0 and 0x1000 <= typ <= 0x10FF:
                        off = struct.unpack_from("<I", blob, i + 8)[0]
                        sec = struct.unpack_from("<H", blob, i + 12)[0]
                        nlen = blob[i + 14]
                        name = blob[i + 15:i + 15 + nlen]
                        if (1 <= sec <= len(self.sections) and 3 <= nlen <= 200
                                and all(32 <= c < 127 for c in name) and 15 + nlen <= ln + 2):
                            owner = self.sections[sec - 1]
                            recs.append((owner.va + off, name.decode(), owner.name))
                            i += 2 + ln
                            continue
                i += 1
            recs.sort()
        self._codeview = recs
        return recs

    def symbol_map(self) -> dict[int, str]:
        """`{rva: name}` from `codeview_symbols()`."""
        return {rva: name for rva, name, _sec in self.codeview_symbols()}

    # ---- imports ---------------------------------------------------------------------------------------------
    def _import_descriptors(self):
        """`(dll, [(name-or-ordinal, IAT slot RVA), ...])` per import descriptor."""
        rva, size = self.data_dir(DIR_IMPORT)
        if not rva or not size:
            return
        d = self.data
        o = self.rva2off(rva)
        i = 0
        while o is not None:
            oft, _ts, _fc, namerva, fta = struct.unpack_from("<IIIII", d, o + 20 * i)
            if namerva == 0 and fta == 0 and oft == 0:
                break
            no = self.rva2off(namerva)
            dll = d[no:d.find(b"\0", no)].decode("latin-1")
            to = self.rva2off(oft or fta)
            funcs = []
            j = 0
            while to is not None:
                v = struct.unpack_from("<I", d, to + 4 * j)[0]
                if v == 0:
                    break
                if v & 0x80000000:
                    funcs.append((f"ord#{v & 0xFFFF}", fta + 4 * j))
                else:
                    ho = self.rva2off(v)
                    funcs.append((d[ho + 2:d.find(b"\0", ho + 2)].decode("latin-1"), fta + 4 * j))
                j += 1
            yield dll, funcs
            i += 1

    def imports(self) -> list[tuple[str, list[str]]]:
        """`[(dll, [name-or-ordinal, ...]), ...]` in directory order (an ordinal reads `ord#N`)."""
        return [(dll, [name for name, _slot in funcs]) for dll, funcs in self._import_descriptors()]

    def iat_slots(self) -> dict[str, int]:
        """`{imported function name: VA of its IAT slot}` (by-name imports; the first slot of a repeated name)."""
        out: dict[str, int] = {}
        for _dll, funcs in self._import_descriptors():
            for name, slot in funcs:
                if not name.startswith("ord#"):
                    out.setdefault(name, self.image_base + slot)
        return out

    # ---- resources -------------------------------------------------------------------------------------------
    def resources(self) -> list[tuple]:
        """`[(type_id, name_id, lang_id, data_rva, size, codepage), ...]`; a named (non-integer) id reads -1."""
        rva, size = self.data_dir(DIR_RESOURCE)
        if not rva or not size:
            return []
        base = self.rva2off(rva)
        if base is None:
            return []
        blob = self.data

        def entries(off):
            _chars, _ts, _maj, _mnr, nnamed, nid = struct.unpack_from("<IIHHHH", blob, base + off)
            rows = []
            for i in range(nnamed + nid):
                name, offv = struct.unpack_from("<II", blob, base + off + 16 + 8 * i)
                rows.append((name, offv & 0x7FFFFFFF, bool(offv & 0x80000000)))
            return rows

        out = []

        def walk(off, path, depth=0):
            if depth > 3:
                return
            for name, sub, isdir in entries(off):
                nid = name if name < 0x10000 else -1
                if isdir:
                    walk(sub, path + [nid], depth + 1)
                else:
                    doff, dsize, cp, _res = struct.unpack_from("<IIII", blob, base + sub)
                    out.append(tuple(path + [nid]) + (doff, dsize, cp))

        walk(0, [])
        return out

    def string_blocks(self) -> list[tuple[int, int, int]]:
        """The `RT_STRING` blocks: `[(block_id, data_rva, size), ...]`."""
        out = []
        for res in self.resources():
            type_id, name_id = res[0], res[1]
            data_rva, size = res[-3], res[-2]
            if type_id == RT_STRING and name_id >= 0:
                out.append((name_id, data_rva, size))
        return out
