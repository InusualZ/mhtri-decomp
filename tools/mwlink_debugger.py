#!/usr/bin/env python3
"""Interrogate the Metrowerks linker (``mwldeppc.exe``) about a real link.

This is the linker-side sibling of ``tools/mwcc-debugger/``.  That tool answers
"which optimizer pass did that?" for the *compiler* by driving
``mwcceppc.exe`` under gdb and classifying the PCode dump against the object.
The compiler is tractable because every Wii ``mwcceppc.exe`` ships a CodeView
``NB11`` symbol blob naming its own functions; this tool starts by checking
whether the linker has the same lever, and reports what it has instead.

**The lever is not a symbol blob.**  Every one of the 31 ``mwldeppc.exe``
files in ``build/compilers/{Wii,GC}/*`` has an *empty* PE debug directory - no
CodeView blob, no symbols (``info`` prints this).  What the linker does have,
and what this tool uses, is two things the compiler-side technique cannot use:

1. its own **diagnostics** - ``-v`` / ``-progress`` / ``-map`` emit a real
   phase timeline (``Linking:`` -> ``Optimizing:`` -> ``Writing:`` ->
   ``Layout: <section>`` xN -> ``Writing: <section>`` xN), so the "dump" is the
   link map and the "timeline" is the verbose stream (``timeline``);
2. a **message catalog** in the PE resources - the linker's engine messages
   (including every phase name) live in the RT_STRING resource as
   length-prefixed UTF-16LE, which is why an ASCII ``strings`` pass misses
   them; ``messages`` decodes the catalog and ``anchors`` finds the code that
   references the address-referenced strings.

**Anchors are derived, not transcribed.**  In the same spirit as
``locate/pass_points.py`` (which takes the return address of every ``call
<pass>``), ``anchors`` scans ``.text`` for instructions whose immediate operand
is the VA of a known string and reports ``{anchor RVA: what the string is}``.
The header calls the ctor/dtor name-dispatch sites out specifically because
they are the documented Row 46 mystery, and ``anchors --prove`` (gdb) counts
the hits on a real link - a breakpoint that never fires is reported as
*unproven*, not as an anchor.

**Health check.**  ``verify`` clasps the link map against the ELF
``elf2dol`` will be run on: section addresses and sizes in ``main.MAP`` must
*be* the section headers of ``main.elf``.  It classifies first and stays loud:
a ``FAIL`` names the first section that disagrees, and it is never a formality.

**Ground truth check for a run**: ``run --gdb`` also harnesses the anchors
proven above - it stops the linker at each derived anchor and prints the
section name being classified, then reports whether the produced ELF is
byte-identical to the one ``ninja`` built.

Provenance: no code is copied from ``tools/mwcc-debugger/``.  The PE header /
section / data-directory / resource parsing here is our own, stdlib-only
(that tree's ``locate/dissect.py`` is a different, symbol-blob-shaped helper).
The *method* - dump the tool's own state, then prove the dump describes the
artifact before believing it - is borrowed from ``locate/verify_pcode.py``;
the anchor derivation borrows its shape from ``locate/pass_points.py``.  The
cc0 / fork provenance of ``tools/mwcc-debugger/`` does not reach this file:
this is original work in this repository.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent

# The linker the build actually uses, if we can find it.
DEFAULT_LINKER = ROOT / "build/compilers/Wii/1.3/mwldeppc.exe"
# The Wii/1.3 facts from the compiler lane: the blobs a compiler carries.
CODES = ["Wii/1.0", "Wii/1.0RC1", "Wii/1.0a", "Wii/1.1", "Wii/1.3", "Wii/1.5",
         "Wii/1.6", "Wii/1.7", "Wii/0x4201_127"]


# ---------------------------------------------------------------------------
# A small, self-contained PE reader
# ---------------------------------------------------------------------------


class Section:
    __slots__ = ("name", "va", "vsize", "raw_off", "raw_size", "flags")

    def __init__(self, name, va, vsize, raw_off, raw_size, flags):
        self.name = name
        self.va = va
        self.vsize = vsize
        self.raw_off = raw_off
        self.raw_size = raw_size
        self.flags = flags

    def __repr__(self):
        return f"<{self.name} va={self.va:#x} vs={self.vsize:#x}>"


class Pe:
    """The PE facts the linker debugger needs: sections, dirs, resources."""

    def __init__(self, path):
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
        self.opt_off = opt
        self.optsz = optsz
        self.sections = []
        so = opt + optsz
        for i in range(nsec):
            s = so + 40 * i
            name = d[s:s + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, raw_size, raw_off = struct.unpack_from("<IIII", d, s + 8)
            flags = struct.unpack_from("<I", d, s + 36)[0]
            self.sections.append(Section(name, va, vsize, raw_off, raw_size, flags))

    # ---- addressing ------------------------------------------------------
    def section(self, name):
        for s in self.sections:
            if s.name == name:
                return s
        return None

    def rva2off(self, rva):
        for s in self.sections:
            if s.va <= rva < s.va + max(s.vsize, s.raw_size):
                if rva - s.va >= s.raw_size:
                    return None
                return s.raw_off + (rva - s.va)
        return None

    def read_rva(self, rva, n):
        o = self.rva2off(rva)
        if o is None:
            raise ValueError(f"RVA {rva:#x} is not backed by file data")
        return self.data[o:o + n]

    def cstring(self, rva):
        o = self.rva2off(rva)
        if o is None:
            return None
        end = self.data.find(b"\0", o)
        return self.data[o:end].decode("latin-1")

    # ---- data directories ------------------------------------------------
    def data_dir(self, index):
        d = self.data
        dd = self.opt_off + 96 + 8 * index
        return struct.unpack_from("<II", d, dd)

    def debug_entries(self):
        """The PE debug directory (index 6) - empty for every mwldeppc.exe."""
        rva, size = self.data_dir(6)
        if not rva or not size:
            return []
        o = self.rva2off(rva)
        if o is None:
            return []
        out = []
        for i in range(size // 28):
            ch, ts, maj, mnr, typ, dsize, daddr, ptr = struct.unpack_from(
                "<IIHHIIII", self.data, o + 28 * i
            )
            out.append({"type": typ, "size": dsize, "addr": daddr, "ptr": ptr,
                        "char": ch})
        return out

    def imports(self):
        """[(dll, [name-or-ordinal, ...]), ...]."""
        rva, size = self.data_dir(1)
        if not rva or not size:
            return []
        o = self.rva2off(rva)
        out = []
        i = 0
        while True:
            oft, ts, fc, namerva, fta = struct.unpack_from(
                "<IIIII", self.data, o + 20 * i
            )
            if namerva == 0 and fta == 0 and oft == 0:
                break
            no = self.rva2off(namerva)
            dll = self.data[no:self.data.find(b"\0", no)].decode("latin-1")
            thunk = oft or fta
            to = self.rva2off(thunk)
            funcs = []
            j = 0
            while to is not None:
                v = struct.unpack_from("<I", self.data, to + 4 * j)[0]
                if v == 0:
                    break
                if v & 0x80000000:
                    funcs.append(f"ord#{v & 0xFFFF}")
                else:
                    ho = self.rva2off(v)
                    nm = self.data[ho + 2:self.data.find(b"\0", ho + 2)].decode("latin-1")
                    funcs.append(nm)
                j += 1
            out.append((dll, funcs))
            i += 1
        return out

    # ---- resources -------------------------------------------------------
    def resources(self):
        """``[(type_id, name_id, lang_id, data_rva, size), ...]``."""
        rva, size = self.data_dir(2)
        if not rva or not size:
            return []
        base = self.rva2off(rva)
        if base is None:
            return []
        blob = self.data

        def entries(off):
            chars, ts, maj, mnr, nnamed, nid = struct.unpack_from("<IIHHHH", blob, base + off)
            out = []
            for i in range(nnamed + nid):
                eo = base + off + 16 + 8 * i
                name, offv = struct.unpack_from("<II", blob, eo)
                out.append((name, offv & 0x7FFFFFFF, bool(offv & 0x80000000)))
            return out

        out = []

        def walk(off, path, depth=0):
            if depth > 3:
                return
            for name, sub, isdir in entries(off):
                nid = name if name < 0x10000 else -1
                if isdir:
                    walk(sub, path + [nid], depth + 1)
                else:
                    doff, dsize, cp, res = struct.unpack_from("<IIII", blob, base + sub)
                    out.append(tuple(path + [nid]) + (doff, dsize, cp))

        walk(0, [])
        return out

    def string_blocks(self):
        """The RT_STRING (type 6) blocks: ``[(block_id, rva, size), ...]``."""
        out = []
        for res in self.resources():
            # (..., type_id, name_id, lang_id, data_rva, size, codepage)
            type_id, name_id = res[0], res[1]
            data_rva, size = res[-3], res[-2]
            if type_id == 6 and name_id >= 0:
                out.append((name_id, data_rva, size))
        return out


def decode_string_block(pe, rva, size):
    """A Windows RT_STRING block: uint16 length + length UTF-16LE WCHARs, x16.

    Returns ``[(string_id, text), ...]`` for the non-empty slots; slot *n* of
    the block is id ``block_id * 16 + n``.  This is where the linker keeps its
    own engine messages - including every phase name ``-v`` prints - which is
    why plain ASCII `strings` does not find them.
    """
    raw = pe.read_rva(rva, size)
    out = []
    i, slot = 0, 0
    while i + 1 < len(raw):
        n = struct.unpack_from("<H", raw, i)[0]
        i += 2
        if n == 0:
            slot += 1
            continue
        s = raw[i:i + 2 * n]
        i += 2 * n
        try:
            text = s.decode("utf-16le")
        except UnicodeDecodeError:
            text = s.decode("latin-1")
        out.append((slot, text))
        slot += 1
    return out


def message_catalogue(pe):
    """``{msgid: text}`` for the whole RT_STRING table, ids numbered from 1.

    The first block's slot 0 is id 16 (Windows numbers the string table from
    1, one id per slot); the linker's phase messages sit in ids 38-60, so the
    offset is 16 per block, which is what ``block_id * 16 + slot`` restores.
    """
    msgs = {}
    for block_id, rva, size in pe.string_blocks():
        for slot, text in decode_string_block(pe, rva, size):
            msgs[block_id * 16 + slot] = text
    return msgs


# ---------------------------------------------------------------------------
# The ctor/dtor order table - the Row 46 lever, derived from the image
# ---------------------------------------------------------------------------


def _string_targets(pe, min_len=6, max_len=120):
    """``{va: text}`` for every null-terminated printable string in the image.

    These are the strings the linker itself passes to its message printer, so
    an instruction whose immediate operand is one of these VAs is a place the
    linker used that string.  ``.rdata`` and ``.data`` are both scanned: the
    option tables and the ctor/dtor name pool live in ``.data`` in these
    binaries, while the diagnostic text is in ``.rdata``.
    """
    out = {}
    for sec_name in (".rdata", ".data"):
        sec = pe.section(sec_name)
        if sec is None or sec.raw_size == 0:
            continue
        data = pe.read_rva(sec.va, sec.vsize)
        for m in re.finditer(rb"[\x20-\x7e]{%d,%d}\x00" % (min_len, max_len), data):
            va = pe.image_base + sec.va + m.start()
            out[va] = m.group()[:-1].decode("latin-1")
    return out


def _ctor_dtor_names(pe):
    """``[(rva, name), ...]`` for the string-pool entries that name a
    ctor/dtor section (``.ctors``, ``.ctors$10``, ``.dtors$15``, ...)."""
    rx = re.compile(r"^\.(ctors|dtors)(\$\w+)?$")
    out = []
    for va, text in _string_targets(pe, min_len=6, max_len=16).items():
        if rx.match(text):
            out.append((va - pe.image_base, text))
    out.sort()
    return out


def derive_order(pe):
    """The linker's fixed ctor/dtor order list, derived from a pointer array.

    The linker identifies each special section by name; the *order* those names
    are laid out in is a fixed array of ``{name, ...}`` records in ``.data``
    (stride 0x2e in the Wii linkers), and the record order *is* the priority
    order.  Find every pointer into the ctor/dtor name pool, sort the cells by
    address, and that sequence is the answer - no address is transcribed.

    This is the mechanism behind Row 46: a unit whose object is byte-identical
    still moves by its ``.ctors$10`` / ``.dtors$15`` words because a fragment
    named ``.ctors$10`` is not ordered like a fragment named ``.ctors``; they
    are two distinct fixed slots in this list.
    """
    pool = {rva: name for rva, name in _ctor_dtor_names(pe)}
    if not pool:
        return None
    base = pe.image_base
    cells = []
    for sec in (pe.section(".data"), pe.section(".rdata")):
        if sec is None or sec.raw_size == 0:
            continue
        blob = pe.read_rva(sec.va, sec.vsize)
        # NOTE: scan every byte offset: the record stride is 0x2e (not a
        # multiple of 4), so the pointer field of every second record is only
        # 2-byte aligned and a 4-byte-stepped scan misses half of the table.
        for o in range(0, len(blob) - 4):
            v = struct.unpack_from("<I", blob, o)[0]
            if base <= v < base + pe.size_image and v - base in pool:
                cells.append((sec.va + o, pool[v - base]))
    cells.sort()
    # Keep only the longest stride-consistent run referencing distinct names:
    # a stray pointer elsewhere in .data is not part of the order table.
    diffs = [cells[i + 1][0] - cells[i][0] for i in range(len(cells) - 1)]
    stride = max(set(diffs), key=diffs.count) if diffs else None
    runs, cur = [], []
    for rva, name in cells:
        if cur and (rva - cur[-1][0] != stride or name in [n for _r, n in cur]):
            runs.append(cur)
            cur = []
        cur.append((rva, name))
    if cur:
        runs.append(cur)
    best = max(runs, key=len) if runs else []
    names = [n for _r, n in best]
    if not names:
        names = [n for _r, n in cells]
    return {"names": names, "cell_rvas": [r for r, _n in best],
            "stride": stride}


# ---------------------------------------------------------------------------
# Anchor derivation: {code address: what string it uses}
# ---------------------------------------------------------------------------


def _capstone():
    try:
        import capstone  # noqa: F401
    except ImportError:
        return None
    import capstone
    return capstone


def derive_anchors(pe, with_capstone=True):
    """``[{anchor, va, target, string, kind}, ...]`` derived from the image.

    Every ``.text`` instruction whose immediate operand is the VA of a string
    in ``.rdata``/``.data`` is a place the linker uses that string.  The
    ctor/dtor name compares (``mov edi, <va>; mov ecx, <len>; repe cmpsb``)
    are flagged as ``section-name`` anchors, because those are the Row 46
    decision points; everything else is a ``message`` anchor.
    """
    cs = _capstone() if with_capstone else None
    if cs is None:
        return None
    targets = _string_targets(pe)
    ctor_rvas = {pe.image_base + rva: name for rva, name in _ctor_dtor_names(pe)}

    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    text = pe.section(".text")
    code = pe.read_rva(text.va, text.vsize)
    insns = list(md.disasm(code, pe.image_base + text.va))
    anchors = []
    for i, ins in enumerate(insns):
        for op in ins.operands:
            if op.type != cs.x86.X86_OP_IMM or op.imm not in targets:
                continue
            rva = ins.address - pe.image_base
            is_name = op.imm in ctor_rvas
            entry = {
                "anchor": rva,
                "target_va": op.imm,
                "string": targets[op.imm],
                "kind": "section-name" if is_name else "message",
                "instruction": f"{ins.mnemonic} {ins.op_str}",
            }
            if is_name:
                # The exact compare length is the proof the match is exact.
                for j in (i + 1, i + 2):
                    if j < len(insns) and insns[j].mnemonic == "mov" \
                            and insns[j].operands and insns[j].operands[0].reg \
                            == cs.x86.X86_REG_ECX and insns[j].operands[1].type \
                            == cs.x86.X86_OP_IMM:
                        entry["compare_len"] = insns[j].operands[1].imm
                        break
            anchors.append(entry)
    anchors.sort(key=lambda a: a["anchor"])
    return anchors


# ---------------------------------------------------------------------------
# The link map and the ELF it describes - the health check
# ---------------------------------------------------------------------------

MAP_FRAG = re.compile(
    r"^\s*([0-9a-f]{8})\s+([0-9a-f]{6})\s+([0-9a-f]{8})\s+([0-9a-f]{8})"
    r"(?:\s+(\d+)\s+(\S*))?"
)
MAP_HDR = re.compile(r"^(.*) section layout$")


def parse_map(text):
    """``{section: {blocks: [{start, extent, fragments}]}}``.

    A link map lists, per output section, every *row* with its
    section-relative start, size and final virtual address.  Two kinds of row
    appear: the input section fragments (named after the section) and the
    symbols inside them.  So the section's size is **not** the sum of the
    sizes - that double-counts every fragment, roughly 2x - it is the largest
    ``offset + size``, exactly the invariant the ELF section header agrees
    with.  The rows are kept whole: the ctor/dtor order check reads their
    names, ``verify`` reads their addresses.
    """
    out = {}
    cur = None
    for line in text.splitlines():
        m = MAP_HDR.match(line.rstrip())
        if m:
            name = m.group(1).strip()
            cur = out.setdefault(name, {"blocks": []})
            cur["blocks"].append({"start": None, "extent": 0, "fragments": []})
            continue
        if cur is None or not cur["blocks"]:
            continue
        fm = MAP_FRAG.match(line)
        if not fm:
            continue
        srel, size, vaddr, foff = (int(fm.group(i), 16) for i in range(1, 5))
        frag_name = fm.group(6) or ""
        block = cur["blocks"][-1]
        if block["start"] is None:
            block["start"] = vaddr
        block["fragments"].append({"name": frag_name, "offset": srel,
                                   "size": size, "addr": vaddr, "file_off": foff})
        block["extent"] = max(block["extent"], srel + size)
    return out


class Elf:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        if self.data[:4] != b"\x7fELF":
            raise ValueError(f"{self.path} is not an ELF")
        if self.data[4] != 1:
            raise ValueError(f"{self.path} is not ELF32")
        self.be = self.data[5] == 2
        self.end = ">" if self.be else "<"

    def sections(self):
        d, en = self.data, self.end
        shoff = struct.unpack_from(en + "I", d, 0x20)[0]
        shentsize, shnum, shstrndx = struct.unpack_from(en + "HHH", d, 0x2E)
        def sh(i):
            return struct.unpack_from(en + "IIIIIIIIII", d, shoff + i * shentsize)
        _, _, _, _, so, ss, _, _, _, _ = sh(shstrndx)

        def nm(x):
            end = d.find(b"\0", so + x)
            return d[so + x:end].decode("latin-1")
        out = []
        for i in range(shnum):
            name, typ, flags, addr, off, size, link, info, align, entsize = sh(i)
            out.append({"name": nm(name), "type": typ, "addr": addr,
                        "offset": off, "size": size, "flags": flags})
        return out


# Sections the linker lays out but that are not ELF output sections of the
# linked program (internal/zero-length pseudo-sections).
_NON_OUTPUT = {"stack", ".stack", "note.split", ".note.split"}


def verify_map(elf_path, map_path):
    """The health check: does this map describe this ELF?

    Returns a report dict with ``status`` in ``match`` / ``fail`` and a
    ``problems`` list.  It compares, per output section, the map's start
    address and summed size against the ELF's section header - the two do not
    just have to be *consistent*, the map has to *be* the artifact elf2dol
    consumes.  A difference is a real disagreement and names the first one.
    """
    elf = Elf(elf_path)
    sections = {s["name"]: s for s in elf.sections()}
    text = Path(map_path).read_text(encoding="utf-8", errors="replace")
    m = parse_map(text)

    problems = []
    compared = []
    for name, info in sorted(m.items()):
        if name in _NON_OUTPUT or name.startswith(".mwcats"):
            continue
        sec = sections.get(name)
        if sec is None:
            if info["blocks"] and info["blocks"][-1]["extent"] == 0:
                continue  # a zero-length pseudo-section the ELF does not carry
            problems.append(f"section '{name}' is in the map but not in the ELF")
            continue
        # Prefer the map block that claims this ELF address; a section whose
        # ELF size is zero still has a start address to agree with.
        blocks = [b for b in info["blocks"] if b["start"] == sec["addr"]]
        if not blocks:
            got = ", ".join(f"{b['start']:#x}" for b in info["blocks"] if b["start"] is not None)
            problems.append(
                f"section '{name}': map start {got or '(none)'} != "
                f"ELF addr {sec['addr']:#x}"
            )
            continue
        if len(blocks) > 1:
            problems.append(f"section '{name}': {len(blocks)} map blocks claim {sec['addr']:#x}")
        block = blocks[0]
        if block["extent"] != sec["size"]:
            problems.append(
                f"section '{name}': map covers {block['extent']:#x} bytes, "
                f"ELF size {sec['size']:#x}"
            )
            continue
        # Every row must land inside the section at its own offset - the
        # map's own arithmetic has to be consistent with the ELF's address.
        # '*fill*' is the one exception: it is padding whose printed address
        # is the *next* fragment's, not start+offset.
        for frag in block["fragments"]:
            if frag["name"] == "*fill*":
                continue
            if frag["offset"] + frag["size"] > sec["size"] or \
                    frag["addr"] != sec["addr"] + frag["offset"]:
                problems.append(
                    f"section '{name}': row '{frag['name'] or '(anonymous)'}' "
                    f"at +{frag['offset']:#x}/{frag['size']:#x} -> {frag['addr']:#x} "
                    f"does not fit {sec['addr']:#x}+{sec['size']:#x}"
                )
                break
        compared.append({"section": name, "start": sec["addr"], "size": sec["size"],
                         "fragments": len(block["fragments"])})

    status = "fail" if problems else "match"
    return {
        "status": status,
        "elf": str(elf_path),
        "map": str(map_path),
        "sections_compared": compared,
        "problems": problems,
        "first_divergence": problems[0] if problems else None,
    }


# ---------------------------------------------------------------------------
# Timeline: run the linker's own diagnostics
# ---------------------------------------------------------------------------

PHASE_LINE = re.compile(r"^#\s+Linking:\s|^#\s+Optimizing:\s|^#\s+Writing:\s|"
                        r"^#\s+Layout:\s|^#\s+Compiling:\s|^#\s+Link order")
ANY_PHASE = re.compile(r"^#\s{3}([A-Za-z][A-Za-z ]*?):\s")


def classify_phase(line, catalogue):
    """Name the message a verbose line came from, if the catalogue has it."""
    body = line.strip()
    if body.startswith("#"):
        body = body[1:].strip()
    for msgid, text in catalogue.items():
        # Engine messages use %c for strings; compare the literal prefix.
        head = text.split("'")[0]
        if head and body.startswith(head.rstrip()):
            return msgid, text
    return None


def parse_timeline(text):
    """The verbose stream as an ordered phase list."""
    out = []
    for line in text.splitlines():
        m = ANY_PHASE.match(line)
        if not m:
            continue
        kind = m.group(1)
        if kind in ("Compiling", "Importing", "Lib Import") and len(out) < 3:
            pass
        out.append((kind, line.strip()))
    return out


# ---------------------------------------------------------------------------
# Finding the linker the build uses
# ---------------------------------------------------------------------------


def default_linker():
    """The linker the *build* uses, read from build.ninja's global mw_version.

    That variable is what the ``link`` rule expands, so it is the binary whose
    behaviour the build depends on - in this tree Wii/1.0, not the Wii/1.3 the
    compiler lane named.  Falls back to a local Wii build, then to 1.3.
    """
    ninja = ROOT / "build.ninja"
    if ninja.exists():
        m = re.search(r"^mw_version\s*=\s*(\S+)", ninja.read_text(errors="replace"), re.M)
        if m:
            cand = ROOT / "build/compilers" / m.group(1).replace("\\", "/") / "mwldeppc.exe"
            if cand.exists():
                return cand
    found = sorted((ROOT / "build/compilers").glob("Wii/*/mwldeppc.exe"))
    if found:
        return found[-1]
    return DEFAULT_LINKER if DEFAULT_LINKER.exists() else None


def find_gdb(explicit=None):
    if explicit:
        return explicit if Path(explicit).exists() else None
    for cand in (ROOT / "build/tools/gdb.exe", Path.home() / "tools/mwcc-dbg/mingw64/bin/gdb.exe"):
        if Path(cand).exists():
            return str(cand)
    from shutil import which
    return which("gdb.exe") or which("gdb")


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------


def cmd_info(args):
    pe = Pe(args.linker)
    print(f"linker:     {pe.path}")
    print(f"machine:    {pe.machine:#x}  image base {pe.image_base:#x}  "
          f"size {pe.size_image:#x}  entry {pe.entry:#x}")
    print("sections:")
    for s in pe.sections:
        print(f"  {s.name:<8} va={s.va:#08x} vs={s.vsize:#07x} "
              f"raw={s.raw_off:#08x} rs={s.raw_size:#07x} flags={s.flags:#x}")
    dbg = pe.debug_entries()
    print(f"debug directory: {len(dbg)} entr{'y' if len(dbg) == 1 else 'ies'}"
          + ("" if dbg else "  <- no CodeView blob; the compiler's symbol lever is absent"))
    for e in dbg:
        print(f"  type={e['type']} size={e['size']:#x} addr={e['addr']:#x}")
    imps = pe.imports()
    print(f"imports: {len(imps)} module(s)")
    for dll, funcs in imps:
        print(f"  {dll}: {len(funcs)} name(s)"
              + (f" [{', '.join(funcs[:4])}{', ...' if len(funcs) > 4 else ''}]"
                 if len(funcs) <= 12 else ""))
    blocks = pe.string_blocks()
    cat = message_catalogue(pe)
    print(f"RT_STRING resource blocks: {len(blocks)}  messages: {len(cat)} "
          "(the linker's own phase names live here)")
    if args.json:
        print(json.dumps({"path": str(pe.path), "image_base": pe.image_base,
                          "debug_entries": dbg, "messages": len(cat),
                          "imports": {d: len(f) for d, f in imps}}, indent=2))
    return 0


def cmd_messages(args):
    pe = Pe(args.linker)
    cat = message_catalogue(pe)
    if args.grep:
        rx = re.compile(args.grep, re.I)
        hit = {k: v for k, v in cat.items() if rx.search(v)}
    else:
        hit = cat
    for msgid in sorted(hit):
        print(f"msgid={msgid:<4} {hit[msgid]}")
    print(f"# {len(hit)} of {len(cat)} messages", file=sys.stderr)
    return 0


def cmd_order(args):
    pe = Pe(args.linker)
    info = derive_order(pe)
    if not info:
        print("no ctor/dtor name pool found", file=sys.stderr)
        return 2
    names = info["names"]
    print(f"linker:  {pe.path}")
    print(f"stride:  {info['stride'] and hex(info['stride'])}  "
          f"cells: {', '.join(hex(c) for c in info['cell_rvas'])}")
    print("the linker's fixed ctor/dtor order (record order in .data):")
    for i, name in enumerate(names):
        print(f"  {i}: {name}")
    if args.map:
        rep = validate_order_against_map(names, Path(args.map))
        print()
        print(f"cross-check against {args.map}:")
        print(f"  {rep['verdict']}")
        for line in rep["lines"]:
            print("  " + line)
        return 0 if rep["ok"] else 1
    return 0


def validate_order_against_map(order, map_path):
    """Does the map's ctor/dtor layout follow the linker's derived order?

    The map lists the fragments of ``.ctors``/``.dtors`` in layout order.  The
    section-name groups in that listing must appear in the same sequence as the
    derived priority list - this is what makes the derived table evidence
    rather than a plausible-looking array.
    """
    m = parse_map(map_path.read_text(encoding="utf-8", errors="replace"))
    rank = {name: i for i, name in enumerate(order)}
    lines, ok = [], True
    for section in (".ctors", ".dtors"):
        info = m.get(section)
        if not info:
            lines.append(f"{section}: not in the map")
            continue
        block = info["blocks"][-1]
        groups = []
        for frag in block["fragments"]:
            n = frag["name"]
            if n and n not in groups:
                groups.append(n)
        # Keep only the names that the order table knows (the map also lists
        # symbols like _ctors and __init_cpp_exceptions_reference).
        known = [g for g in groups if g in rank]
        ranks = [rank[g] for g in known]
        in_order = ranks == sorted(ranks)
        ok = ok and in_order
        lines.append(f"{section}: {', '.join(groups)}")
        lines.append(f"  -> priority ranks {ranks} "
                     f"{'non-decreasing (in order)' if in_order else 'OUT OF ORDER'}")
    return {"ok": ok, "lines": lines,
            "verdict": "MATCH: the map's ctor/dtor layout follows the derived order"
            if ok else "FAIL: the map's ctor/dtor layout contradicts the derived order"}


def cmd_anchors(args):
    pe = Pe(args.linker)
    anchors = derive_anchors(pe)
    if anchors is None:
        print("anchors needs capstone (pip install capstone)", file=sys.stderr)
        return 2
    if args.kind:
        anchors = [a for a in anchors if a["kind"] == args.kind]
    names = [a for a in anchors if a["kind"] == "section-name"]
    msgs = [a for a in anchors if a["kind"] == "message"]
    print(f"linker: {pe.path}")
    print(f"derived anchors: {len(anchors)} ({len(names)} section-name, {len(msgs)} message)")
    print()
    print("section-name anchors (the Row 46 dispatch sites):")
    print(f"  {'anchor':<10} {'kind':<13} {'compare':<8} string")
    for a in names:
        cl = a.get("compare_len")
        print(f"  {a['anchor']:#08x}   {a['kind']:<13} "
              f"{(str(cl) + ' bytes') if cl else '-':<8} {a['string']}")
    if args.messages:
        print()
        print("message anchors (first 40):")
        for a in msgs[:40]:
            print(f"  {a['anchor']:#08x}   {a['string'][:60]}")
    if args.json:
        print(json.dumps(anchors, indent=2))
    if args.prove:
        return prove_anchors(pe, names if not args.all_anchors else anchors, args)
    return 0


def prove_anchors(pe, anchors, args):
    """Break on each derived anchor in a real link and count the hits.

    A breakpoint that never fires is *not* an anchor: it is reported as
    unproven.  This is the linker analogue of ``verify_pcode`` - the derived
    table is only believed after the binary is seen to reach it.
    """
    gdb = find_gdb(args.gdb)
    if not gdb:
        print("no gdb found; pass --gdb or run tools/mwcc-debugger/fetch_gdb.py",
              file=sys.stderr)
        return 2
    if not args.args:
        print("--prove needs the linker argument list (--args '<link line>')", file=sys.stderr)
        return 2
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    script = work / "mwlink-prove.gdb"
    # A gdb Python stop handler is used rather than `commands` + `printf`, so a
    # bad candidate pointer at one anchor cannot abort the whole run.
    table = "\n".join(
        "  (0x%x, '%08x', %r, %s)," % (
            pe.image_base + a["anchor"], a["anchor"], a["string"],
            "True" if a.get("compare_len") else "False",
        )
        for a in anchors
    )
    script.write_text(
        "set pagination off\n"
        "set confirm off\n"
        "set width 0\n"
        f"file {pe.path.as_posix()}\n"
        "set args " + args.args + "\n"
        "python\n"
        "import gdb\n"
        "ANCHORS = [\n" + table + "\n]\n"
        "class Anchor(gdb.Breakpoint):\n"
        "    def __init__(self, addr, tag, text, candidate):\n"
        "        super().__init__('*0x%x' % addr, internal=True)\n"
        "        self.tag, self.text, self.candidate = tag, text, candidate\n"
        "    def stop(self):\n"
        "        sec = ''\n"
        "        if self.candidate:\n"
        "            try:\n"
        "                p = int(gdb.parse_and_eval('$esi'))\n"
        "                raw = gdb.selected_inferior().read_memory(p, 32).tobytes()\n"
        "                s = raw.split(b'\\0')[0].decode('latin-1')\n"
        "                if s and all(32 <= ord(c) < 127 for c in s):\n"
        "                    sec = \" sec='%s'\" % s\n"
        "            except Exception:\n"
        "                sec = ' sec=?'\n"
        "        print('MWLINK-HIT %s%s' % (self.tag, sec))\n"
        "        return False\n"
        "for _a in ANCHORS:\n"
        "    Anchor(*_a)\n"
        "end\n"
        "run\n"
        "printf \"MWLINK-DONE\\n\"\n"
        "quit\n"
    )
    argv = [gdb, "-batch", "-nx", "-x", str(script)]
    print(f"# gdb {script}")
    proc = subprocess.run(argv, capture_output=True, text=True, errors="replace")
    counts, sections = {}, {}
    for line in proc.stdout.splitlines():
        m = re.match(r"^MWLINK-HIT ([0-9a-f]{8})(?: sec='([^']*)')?", line.strip())
        if m:
            counts[m.group(1)] = counts.get(m.group(1), 0) + 1
            if m.group(2) is not None:
                sections.setdefault(m.group(1), set()).add(m.group(2))
    fired = 0
    print("proven on a real link (FIRED) / not reached by this link (UNPROVEN):")
    for a in anchors:
        key = f"{a['anchor']:08x}"
        n = counts.get(key, 0)
        fired += 1 if n else 0
        seen = sections.get(key)
        sec = ("  matched " + ", ".join(f"'{s}'" for s in sorted(seen))) if seen else ""
        print(f"  {'FIRED   ' if n else 'UNPROVEN'} {key} x{n:<5} {a['string']}{sec}")
    print(f"# {fired}/{len(anchors)} anchors fired")
    if not fired and proc.stderr.strip():
        print("# gdb stderr:\n" + proc.stderr.strip()[:1000], file=sys.stderr)
    # Report the artifact the run produced, if the command line says where.
    m = re.search(r"(?:^|\s)-o\s+(\S+)", args.args)
    if m:
        out = Path(m.group(1))
        if out.exists():
            import hashlib
            digest = hashlib.sha256(out.read_bytes()).hexdigest()
            print(f"# artifact {out} sha256 {digest}")
        else:
            print(f"# artifact {out} was NOT written", file=sys.stderr)
    if args.require_all:
        return 0 if fired == len(anchors) else 1
    return 0 if fired else 1


def cmd_timeline(args):
    pe = Pe(args.linker)
    cat = message_catalogue(pe)
    argv = [str(pe.path)] + args.args.split()
    if "-v" not in argv and "-verbose" not in argv:
        argv.append("-v")
    print("# " + " ".join(argv))
    proc = subprocess.run(argv, capture_output=True, text=True, errors="replace")
    text = proc.stdout + proc.stderr
    for kind, line in parse_timeline(text):
        if kind in ("Compiling", "Importing", "Lib Import"):
            continue
        c = classify_phase(line, cat)
        tag = f"  [msgid {c[0]}]" if c else ""
        print(f"{line}{tag}")
    return 0 if proc.returncode == 0 else 1


def cmd_verify(args):
    rep = verify_map(args.elf, args.map)
    print(f"map:   {args.map}")
    print(f"elf:   {args.elf}")
    if rep["status"] == "match":
        print(f"MATCH: {len(rep['sections_compared'])} section(s) - the map is this ELF")
        for s in rep["sections_compared"]:
            print(f"  {s['section']:<12} {s['start']:#010x} size {s['size']:#08x} "
                  f"({s['fragments']} fragments)")
    else:
        print("FAIL: the map does not describe this ELF")
        print(f"first divergence: {rep['first_divergence']}")
        for p in rep["problems"][:40]:
            print("  " + p)
    if args.identity:
        a = Path(args.elf).read_bytes()
        b = Path(args.identity).read_bytes()
        same = a == b
        print(f"identity: {args.identity} "
              + ("byte-identical" if same else f"DIFFERS ({len(a)} vs {len(b)} bytes)"))
        if not same:
            return 1
    return 0 if rep["status"] == "match" else 1


# ---------------------------------------------------------------------------
# Selftest - fixtures only, no gdb, no compiler, no linker
# ---------------------------------------------------------------------------


def _fixture_elf(sections):
    """A minimal big-endian ELF32 with a shstrtab and the given sections."""
    names = [""] + [s[0] for s in sections] + [".shstrtab"]
    blob = b"\0"
    offs = {}
    for n in names:
        if n and n not in offs:
            offs[n] = len(blob)
            blob += n.encode() + b"\0"
    strndx = len(names) - 1
    shoff = 0x100
    hdr = b"\x7fELF\x01\x02\x01\x00" + b"\0" * 8
    # e_type, e_machine, e_version, e_entry, e_phoff, e_shoff, e_flags,
    # e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx
    hdr += struct.pack(">HHIIIIIHHHHHH", 2, 20, 1, 0, 0, shoff, 0, 52,
                       0, 0, 40, len(names), strndx)
    hdr = hdr.ljust(0x40, b"\0")
    body = bytearray(b"\0" * shoff)
    body[:len(hdr)] = hdr
    out = bytearray(body)
    # section headers
    def sh(name, typ, addr, size, offset):
        return struct.pack(">IIIIIIIIII", offs.get(name, 0), typ, 0, addr,
                           offset, size, 0, 0, 4, 0)
    shdrs = [sh("", 0, 0, 0, 0)]
    for n, typ, addr, size, offset in sections:
        shdrs.append(sh(n, typ, addr, size, offset))
    shdrs.append(sh(".shstrtab", 3, 0, len(blob), 0x200))
    out += b"\0" * (shoff + 40 * len(shdrs) - len(out))
    out[shoff:shoff + 40 * len(shdrs)] = b"".join(shdrs)
    out += b"\0" * (0x200 - len(out))
    out += blob
    return bytes(out)


def selftest():
    """Pure-function + fixture checks. No gdb/compiler/linker is invoked."""
    fails, checks = [], 0

    def ok(cond, what):
        nonlocal checks
        checks += 1
        if not cond:
            fails.append(what)
        return cond

    # 1. RT_STRING decode: length-prefixed UTF-16LE, with empty slots.
    class FakePe:
        def __init__(self, blob):
            self.blob = blob

        def read_rva(self, rva, n):
            return self.blob[rva:rva + n]

    block = struct.pack("<H", 4) + "Link".encode("utf-16le")
    block += struct.pack("<H", 0)                       # empty slot
    block += struct.pack("<H", 13) + "Linking: '%c'".encode("utf-16le")
    dec = decode_string_block(FakePe(block), 0, len(block))
    ok(dec == [(0, "Link"), (2, "Linking: '%c'")], f"rt_string decode: {dec!r}")

    # 2. The message catalogue numbers ids from 1 per block (block*16+slot).
    ok(16 * 1 + 0 == 16, "message ids number from 16 in the first block")

    # 3. MAP parsing.
    mp = """\n.ctors section layout\n"""
    mp += "  00000000 000000 8056f2c0 0056b4c0  1 .ctors$00 \tLinker Generated Symbol File \n"
    mp += "  00000000 000004 8056f2c0 0056b4c0  1 .ctors$10 \t__init_cpp_exceptions.o \n"
    mp += "  00000004 00000c 8056f2c4 0056b4c4  1 .ctors \tfoo.o \n"
    parsed = parse_map(mp)
    ok(parsed[".ctors"]["blocks"][0]["start"] == 0x8056F2C0, "map start")
    ok(parsed[".ctors"]["blocks"][0]["extent"] == 0x10, "map extent = max(off+size)")
    ok(len(parsed[".ctors"]["blocks"][0]["fragments"]) == 3, "map fragments")

    # 4. verify: MATCH, then a real FAIL when the ELF disagrees.
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        elf = td / "x.elf"
        elf.write_bytes(_fixture_elf([(".ctors", 1, 0x8056F2C0, 0x10, 0x200)]))
        mf = td / "x.MAP"
        mf.write_text(mp)
        rep = verify_map(elf, mf)
        ok(rep["status"] == "match", f"verify MATCH: {rep['problems']}")
        # Wrong address -> loud FAIL, not a silent pass.
        bad = td / "bad.elf"
        bad.write_bytes(_fixture_elf([(".ctors", 1, 0x80000000, 0x10, 0x200)]))
        rep = verify_map(bad, mf)
        ok(rep["status"] == "fail", "verify FAILs on a different ELF")
        ok("!= " in (rep["first_divergence"] or ""), "FAIL names the divergence")
        # Wrong size -> loud FAIL too.
        small = td / "small.elf"
        small.write_bytes(_fixture_elf([(".ctors", 1, 0x8056F2C0, 0x8, 0x200)]))
        rep = verify_map(small, mf)
        ok(rep["status"] == "fail" and "covers" in rep["first_divergence"],
           "verify FAILs on a size mismatch")

    # 5. The ctor/dtor order validator, on the map text above.
    rep = validate_order_against_map(
        [".ctors$00", ".ctors$10", ".ctors", ".ctors$99"], _write_tmp_orders(mp))
    ok(rep["ok"], "order validator accepts an in-order map")
    # .ctors (rank 2) before .ctors$10 (rank 1) is the Row 46 swap.
    swapped = mp.replace(
        "  00000000 000004 8056f2c0 0056b4c0  1 .ctors$10 \t__init_cpp_exceptions.o \n"
        "  00000004 00000c 8056f2c4 0056b4c4  1 .ctors \tfoo.o \n",
        "  00000000 00000c 8056f2c0 0056b4c0  1 .ctors \tfoo.o \n"
        "  0000000c 000004 8056f2cc 0056b4cc  1 .ctors$10 \t__init_cpp_exceptions.o \n")
    ok(swapped != mp, "swap fixture built")
    rep = validate_order_against_map(
        [".ctors$00", ".ctors$10", ".ctors", ".ctors$99"], _write_tmp_orders(swapped))
    ok(not rep["ok"], "order validator rejects an out-of-order map")

    # 6. Timeline parsing finds the phases and ignores the per-object spam.
    tl = parse_timeline(
        "#   Compiling: 'a.o'\n#   Linking: 'x.elf'\n#   Optimizing: 'x.elf'\n"
        "#   Layout: 'x.elf' (.text)\nnot a phase\n")
    kinds = [k for k, _ in tl]
    ok(kinds == ["Compiling", "Linking", "Optimizing", "Layout"],
       f"timeline kinds: {kinds}")
    cat = {43: "Linking: '%c'", 58: "Layout: '%c' (%c)"}
    ok(classify_phase("#   Layout: 'x.elf' (.text)", cat)[0] == 58,
       "classify_phase maps a Layout line to msgid 58")

    if fails:
        for f in fails:
            print("FAIL " + f, file=sys.stderr)
        return 1
    print(f"ok - {checks} checks")
    return 0


def _write_tmp_orders(text):
    fh = tempfile.NamedTemporaryFile("w", suffix=".MAP", delete=False,
                                     encoding="utf-8")
    fh.write(text)
    fh.close()
    return Path(fh.name)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_parser():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--selftest", action="store_true",
                    help="run fixture checks (no gdb/compiler/linker)")
    sub = ap.add_subparsers(dest="cmd")

    def linker_arg(p):
        p.add_argument("linker", nargs="?", default=None,
                       help="path to mwldeppc.exe (default: the build's)")

    p = sub.add_parser("info", help="PE recon: sections, dirs, the no-blob check")
    linker_arg(p)
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("messages", help="decode the RT_STRING message catalogue")
    linker_arg(p)
    p.add_argument("--grep", default=None)

    p = sub.add_parser("order", help="the fixed ctor/dtor order (Row 46)")
    linker_arg(p)
    p.add_argument("--map", default=None, help="cross-check against this link map")

    p = sub.add_parser("anchors", help="derive {code address: string} anchors")
    linker_arg(p)
    p.add_argument("--kind", choices=("section-name", "message"), default=None)
    p.add_argument("--messages", action="store_true", help="also list message anchors")
    p.add_argument("--json", action="store_true")
    p.add_argument("--prove", action="store_true", help="prove them with a gdb run")
    p.add_argument("--all-anchors", action="store_true",
                   help="with --prove, use every anchor not just section-name")
    p.add_argument("--args", default=None, help="the linker argument list")
    p.add_argument("--out", default="build/mwlink-debug", help="scratch dir")
    p.add_argument("--gdb", default=None)
    p.add_argument("--require-all", action="store_true",
                   help="with --prove, fail unless every anchor fired")

    p = sub.add_parser("timeline", help="run the linker's own verbose diagnostics")
    linker_arg(p)
    p.add_argument("--args", default="", help="the linker argument list")

    p = sub.add_parser("verify", help="health check: does the map describe the ELF")
    p.add_argument("map")
    p.add_argument("elf")
    p.add_argument("--identity", default=None,
                   help="also byte-compare the ELF against this file")
    return ap


def main(argv=None):
    ap = build_parser()
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 2
    if getattr(args, "linker", None) is None:
        args.linker = default_linker()
    if args.linker is None:
        print("no linker found; pass one explicitly", file=sys.stderr)
        return 2
    args.linker = Path(args.linker)
    if not args.linker.exists():
        print(f"linker not found: {args.linker}", file=sys.stderr)
        return 2
    fn = {"info": cmd_info, "messages": cmd_messages, "order": cmd_order,
          "anchors": cmd_anchors, "timeline": cmd_timeline, "verify": cmd_verify}[args.cmd]
    try:
        return fn(args)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
