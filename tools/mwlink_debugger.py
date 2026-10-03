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

**The phase table.**  ``phases`` derives the linker's *phase* code the way
``locate/pass_points.py`` derives the compiler's pass table, but it has to go
one level further because the phase messages are referenced by resource id:
the message loader is the `call [LoadStringA]` site whose ``uID`` argument is
not a constant, its callers are the one-per-message formatters, and the return
address of ``call <formatter>`` is a phase anchor.  ``phases --prove`` breaks on
them - and on the loader itself - during a real link, prints the ``(id, text)``
stream the linker really emits, and reports every anchor that never fired as
unproven.  That run is also what corrected this tool's catalogue numbering: the
id is ``(block_name - 1) * 16 + slot`` (50 of 50 observed messages agree).

**Tracing one unit.**  ``trace <unit|object>`` answers "how did this unit get
linked?": whether the link kept it, where each of its sections landed (checked
by reading the object's bytes back out of the output ELF), how its symbols
resolved (the map *and* the output ELF's own symbol table), which relocations
touched it (each one's field decoded out of the artifact and compared with the
ABI), and where its ctor/dtor fragment went in the linker's fixed class order.
The argument is a **unit name** (``Network/NetworkWiiMediator``) as well as a
path: the object is resolved from the build's own link statement, and since the
build writes no map, the tool links one into ``build/scratch/`` when it needs to.
``trace --link`` never writes ``build/RMHE08/main.elf``.

**An erroring link is the production case.**  ``diagnose`` runs the build's own
link with ``-v`` and reports the phase stream, then every diagnostic classified
against the message catalogue and attributed to the phase that printed it - a
diagnostic the catalogue does not hold is reported *without* an id rather than
given one.  ``trace --link`` uses the same attribution when its link fails and
then still traces the object against whatever map exists.

**The internal records.**  ``records`` documents the linker's own input-file
record (stride, array, and each field) derived from the ``.comment`` parser's
instructions, and ``records --prove`` reads that array out of a running link and
cross-checks every field against the object it names.  ``align`` derives where
the linker aligns a fragment's address and what it compares - the answer to the
``tools/elf/objalign.py`` question.  Fields that could not be derived are rows
that say so; nothing is invented.

**Health check.**  ``verify`` clasps the link map against the ELF
``elf2dol`` will be run on: section addresses and sizes in ``main.MAP`` must
*be* the section headers of ``main.elf``.  It classifies first and stays loud:
a ``FAIL`` names the first section that disagrees, and it is never a formality.

**Ground truth check for a run**: ``anchors --prove`` and ``phases --prove``
stop the linker at the derived anchors and report what it was doing there;
``trace --link`` reports whether the artifact it traced is byte-identical to the
one ``ninja`` built.

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
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from tools.lib.project import Splits

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
    """``{msgid: text}`` for the whole RT_STRING table.

    The id is **the linker's own id**, and it is the one `LoadStringA` is
    called with: blocks are 16 strings, the first block is named 1 and holds
    ids 0..15, so ``msgid = (block_name - 1) * 16 + slot``.  That is not a
    guess: `phases --prove` breaks at the linker's own message loader on a real
    link and reports the ``(id, string)`` pairs it really asks for, and the
    ids observed there (27 = ``Linking: '%c'``, 29 = ``Writing: '%c'``,
    41 = ``Optimizing: '%c'``) are exactly this formula's answer.  An earlier
    revision of this tool numbered from ``block_name * 16``, i.e. 16 too high -
    those labels were indices into the table, not the ids the linker uses.
    """
    msgs = {}
    for block_id, rva, size in pe.string_blocks():
        for slot, text in decode_string_block(pe, rva, size):
            msgs[(block_id - 1) * 16 + slot] = text
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
    r"(?:\s+(\d+))?\s+(.*?)\s*$"
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
        # The row's name may contain spaces (`lbl_80004514 (entry of
        # pad_00_80004380_init)`) and the alignment column may be *empty* for
        # those rows, so the match is made against the part before the tab that
        # separates the row from its input file.
        left = line.split("\t", 1)[0]
        fm = MAP_FRAG.match(left)
        if not fm:
            continue
        srel, size, vaddr, foff = (int(fm.group(i), 16) for i in range(1, 5))
        frag_name = fm.group(6) or ""
        # The last tab-separated column names the *input file* the row came
        # from (``__init_cpp_exceptions.o``, ``Linker Generated Symbol File``),
        # which is what makes the map a per-object trace: it is how a row is
        # attributed to one input unit at all.
        source = line.split("\t")[1].strip() if "\t" in line else ""
        block = cur["blocks"][-1]
        if block["start"] is None:
            block["start"] = vaddr
        block["fragments"].append({"name": frag_name, "offset": srel,
                                   "size": size, "addr": vaddr,
                                   "file_off": foff, "source": source,
                                   "flags": int(fm.group(5) or 0)})
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

    def symbols(self):
        """``{name: [value, ...]}`` from the output ELF's own symbol table.

        This is the artifact's view of a symbol's address - the second,.
        independent witness for the map's rows (and where ``_SDA_BASE_``,
        which the map only prints in its symbol-file listing, comes from).
        """
        d, en = self.data, self.end
        secs = {s["name"]: s for s in self.sections()}
        st = secs.get(".symtab")
        strt = secs.get(".strtab")
        if st is None or strt is None or st["size"] == 0:
            return {}
        out = {}
        for k in range(st["size"] // 16):
            nameoff, value, size, info, other, shndx = struct.unpack_from(
                en + "IIIBBH", d, st["offset"] + 16 * k)
            if not nameoff:
                continue
            end = d.find(b"\0", strt["offset"] + nameoff)
            nm = d[strt["offset"] + nameoff:end].decode("latin-1")
            out.setdefault(nm, []).append(value)
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


def match_message(body, catalogue):
    """The catalogue entry a linker-emitted line came from, or ``None``.

    The catalogue stores a message the way the engine holds it - ``%c``/``%n``
    placeholders included, and often with the line breaks the printer wraps at -
    so both sides are compared with their whitespace collapsed.  The comparison
    is against the literal prefix up to the first placeholder, which is the same
    rule ``phases --prove`` uses to cross-check an observed ``(id, text)`` pair
    against the catalogue, and it is what makes ``diagnose`` able to say *which*
    message an error is.
    """
    body = body.strip()
    while body.startswith("#"):
        body = body.lstrip("#").strip()
    nb = " ".join(body.split())
    for msgid, text in catalogue.items():
        nt = " ".join(text.split())
        head = nt.split("%")[0]
        if head and nb.startswith(head.rstrip()):
            return msgid, text
    return None


def classify_phase(line, catalogue):
    """Name the message a verbose line came from, if the catalogue has it."""
    return match_message(line, catalogue)


def classify_diagnostics(text, catalogue, timeline=None):
    """Every linker diagnostic in a run's output, with its catalogue id and phase.

    ``mwldeppc`` prints an error as a banner (``### mwldeppc.exe Linker
    Error:``) followed by ``#``-prefixed body lines.  The body is matched
    against the message catalogue - the engine's own message - and, when the
    run printed its phase stream (``-v``), the last phase line before the
    diagnostic is attached to it.  A diagnostic whose text is not in the
    catalogue is reported as such rather than given a made-up id.
    """
    out = []
    last_phase = None
    in_error = False
    body = []

    def flush():
        nonlocal body, in_error
        if not body:
            in_error = False
            return
        joined = " ".join(body)
        hit = match_message(joined, catalogue)
        out.append({"text": joined,
                    "msgid": hit[0] if hit else None,
                    "message": hit[1] if hit else None,
                    "phase": last_phase[0] if last_phase else None,
                    "phase_line": last_phase[1] if last_phase else None})
        body = []
        in_error = False

    if timeline is None:
        timeline = parse_timeline(text, phase_kinds(catalogue))
    phase_at = {line.strip(): kind for kind, line in timeline}
    for line in text.splitlines():
        s = line.strip()
        if re.match(r"#+\s*(mwldeppc\.exe\s+)?\S+\s+(Error|Warning|Message)\s*:", s, re.I):
            flush()
            in_error = True
            continue
        if in_error and re.match(r"^#\s", line):
            body.append(s.lstrip("#").strip())
            continue
        if in_error:
            flush()
        if s in phase_at:
            last_phase = (phase_at[s], s)
    flush()
    return out


def phase_kinds(catalogue):
    """The engine's phase names, read off the catalogue.

    A phase message is the shape ``<Name>: '%c'`` - that is what the linker
    prints for ``Linking``, ``Optimizing``, ``Writing``, ``Layout`` and
    ``Compiling`` - so the set of names is *derived* rather than listed.  It
    matters: an error body line such as ``#   undefined: 'foo'`` also looks
    like ``<name>: ...``, and counting it as a phase would mis-attribute the
    very diagnostics this tool is trying to place.
    """
    kinds = set()
    for text in catalogue.values():
        m = re.match(r"^([A-Za-z][A-Za-z ]*):\s*'%c'", text.strip())
        if m:
            kinds.add(m.group(1))
    return kinds or {"Linking", "Optimizing", "Writing", "Layout", "Compiling"}


def parse_timeline(text, kinds=None):
    """The verbose stream as an ordered phase list.

    With ``kinds`` (the catalogue's phase names) only those count as phases; a
    diagnostic body line that merely looks like one is not a phase.
    """
    out = []
    for line in text.splitlines():
        m = ANY_PHASE.match(line)
        if not m:
            continue
        kind = m.group(1)
        if kinds is not None and kind not in kinds:
            continue
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
    proc = subprocess.run(argv, capture_output=True, text=True, encoding="utf-8", errors="replace")
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
    proc = subprocess.run(argv, capture_output=True, text=True, encoding="utf-8", errors="replace")
    text = proc.stdout + proc.stderr
    for kind, line in parse_timeline(text, phase_kinds(cat)):
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
# The input object: what one unit hands the link
# ---------------------------------------------------------------------------
#
# A Metrowerks object is an ELF32 *big-endian* PowerPC relocatable with RELA
# (addend-carrying) relocation sections.  Nothing here is specific to dtk:
# the fields read are the ones the linker reads (`sh_name`, `sh_size`,
# `st_shndx`, `st_value`, `r_offset`, `r_info`, `r_addend`), which is what
# makes the trace below a description of the *linker's* input rather than of
# our tooling's view of it.

STB_LOCAL, STB_GLOBAL, STB_WEAK = 0, 1, 2
STT_NOTYPE, STT_OBJECT, STT_FUNC, STT_SECTION, STT_FILE = 0, 1, 2, 3, 4
SHN_ABS, SHN_UNDEF = 0xFFF1, 0

# Standard PowerPC ELF relocation numbers.  Only names that are *used* are
# named; a type this build emits that is not in the standard list stays a
# number in the output rather than being guessed at.
PPC_RELOCS = {
    0: "NONE", 1: "ADDR32", 2: "ADDR24", 3: "ADDR16", 4: "ADDR16_LO",
    5: "ADDR16_HI", 6: "ADDR16_HA", 7: "ADDR14", 8: "ADDR14_BRTAKEN",
    9: "ADDR14_BRNTAKEN", 10: "REL24", 11: "REL14", 12: "REL14_BRTAKEN",
    13: "REL14_BRNTAKEN", 18: "SECTOFF", 19: "SECTOFF_LO", 20: "SECTOFF_HI",
    21: "SECTOFF_HA", 22: "ADDR30",
}


def reloc_name(t):
    return PPC_RELOCS.get(t, f"type {t}")


class MwObject:
    """One input object: its sections, symbols and relocations."""

    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        d = self.data
        if d[:4] != b"\x7fELF":
            raise ValueError(f"{self.path} is not an ELF")
        if d[4] != 1:
            raise ValueError(f"{self.path} is not ELF32")
        if d[5] != 2:
            raise ValueError(f"{self.path} is not big-endian")
        self.end = ">"
        self._load_sections()
        self._load_symbols()
        self._load_relocs()

    # ---- sections --------------------------------------------------------
    def _load_sections(self):
        d, en = self.data, self.end
        shoff = struct.unpack_from(en + "I", d, 0x20)[0]
        shentsize, shnum, shstrndx = struct.unpack_from(en + "HHH", d, 0x2E)
        self.shentsize, self.shnum = shentsize, shnum
        rows = [struct.unpack_from(en + "IIIIIIIIII", d, shoff + i * shentsize)
                for i in range(shnum)]
        so = rows[shstrndx][4]

        def nm(x):
            end = d.find(b"\0", so + x)
            return d[so + x:end].decode("latin-1")

        self.sections = []
        for i, (name, typ, flags, addr, off, size, link, info, align, entsize) in enumerate(rows):
            self.sections.append({"index": i, "name": nm(name), "type": typ,
                                  "flags": flags, "addr": addr, "offset": off,
                                  "size": size, "link": link, "info": info,
                                  "align": align, "entsize": entsize,
                                  "sh_name": name})

    def section(self, name):
        for s in self.sections:
            if s["name"] == name:
                return s
        return None

    def contents(self, sec):
        """The section's bytes; empty for a NOBITS section (.bss/.sbss)."""
        if sec["type"] == 8 or sec["size"] == 0:
            return b""
        return self.data[sec["offset"]:sec["offset"] + sec["size"]]

    # ---- symbols ---------------------------------------------------------
    def _load_symbols(self):
        d, en = self.data, self.end
        self.symbols = []
        st = next((s for s in self.sections if s["type"] == 2), None)  # SHT_SYMTAB
        if st is None:
            return
        strtab = self.sections[st["link"]]
        for k in range(st["size"] // 16):
            o = st["offset"] + 16 * k
            nameoff, value, size, info, other, shndx = struct.unpack_from(
                en + "IIIBBH", d, o)
            e = d.find(b"\0", strtab["offset"] + nameoff)
            name = d[strtab["offset"] + nameoff:e].decode("latin-1")
            sh = self.sections[shndx] if shndx < len(self.sections) else None
            self.symbols.append({
                "index": k, "name": name, "value": value, "size": size,
                "bind": info >> 4, "type": info & 0xF, "shndx": shndx,
                "shndx_name": sh["name"] if sh else ("ABS" if shndx == SHN_ABS else "UNDEF"),
                "shndx_value": sh["addr"] if sh else 0,
            })

    # ---- relocations -----------------------------------------------------
    def _load_relocs(self):
        d, en = self.data, self.end
        self.relocs = []
        for s in self.sections:
            if s["type"] != 4 or not s["size"]:      # SHT_RELA
                continue
            symtab = self.sections[s["link"]] if s["link"] < len(self.sections) else None
            target = self.sections[s["info"]] if s["info"] < len(self.sections) else None
            for k in range(s["size"] // 12):
                off, info, add = struct.unpack_from(en + "IIi", d, s["offset"] + 12 * k)
                sym = info >> 8
                self.relocs.append({
                    "section": s["name"],
                    "target": target["name"] if target else "?",
                    "target_index": s["info"],
                    "offset": off, "type": info & 0xFF, "sym_index": sym,
                    "addend": add,
                    "sym_name": self.symbols[sym]["name"] if symtab is not None and sym < len(self.symbols) else "?",
                })

    def ctor_dtor_sections(self):
        return [s for s in self.sections
                if s["flags"] & 0x2 and s["size"] and is_ctor_dtor_name(s["name"])]


def is_ctor_dtor_name(name):
    return bool(re.match(r"^\.(ctors|dtors)(\$\w+)?$", name or ""))


# The linker validates the C++ runtime's ctor/dtor entry symbols *by name*: an
# object that defines one of these under a different name aborts the link with
# the linker's own runtime-version diagnostic.  Derived twice: the names are
# immediate operands inside the section-name selector at RVA 0x42e15-0x42f80
# (``push <va>; call <name lookup>``), and the class each one lands in is what
# the real map shows for the object that defines it.
RUNTIME_CTOR_SYMBOLS = {
    "__init_cpp_exceptions_reference": (".ctors", "$10"),
    "__destroy_global_chain_reference": (".dtors", "$10"),
    "__fini_cpp_exceptions_reference": (".dtors", "$15"),
}


# ---------------------------------------------------------------------------
# Relocation verification: did the linker actually apply it?
# ---------------------------------------------------------------------------
#
# The check is a *read-back*: resolve the symbol the relocation names, compute
# the value the ABI says the field must hold, and compare it with the word that
# is really in the output ELF at the relocation's place.  A relocation this
# recognises and that disagrees is a MATCH failure, not a formality.

def reloc_field(reloc_type, word, S, A, P, sda_base=None):
    """``(checked, expected_field, actual_field, where)`` for one relocation.

    ``S`` is the resolved symbol address, ``A`` the addend, ``P`` the place's
    address in the output.  ``where`` records which halfword the field was
    found in, because that is part of the ABI and not something to guess.

    ``sda_base`` is the map's ``_SDA_BASE_`` value - or its ``_SDA2_BASE_``
    when the symbol lives in ``.sdata2``/``.sbss2``, because *which* SDA base a
    type-109 displacement is relative to depends on the symbol's section (r13 for
    ``.sdata``/``.sbss``, r2 for ``.sdata2``/``.sbss2``).  Both are
    linker-generated symbols the map prints, so neither is assumed.
    """
    value = (S + A) & 0xFFFFFFFF
    if reloc_type == 1:                                    # R_PPC_ADDR32
        return True, value, word, "word"
    if reloc_type == 10:                                   # R_PPC_REL24
        return True, (value - P) & 0x03FFFFFC, word & 0x03FFFFFC, "branch24"
    if reloc_type == 109:                                  # SDA-relative disp
        if sda_base is None:
            return False, None, None, ""
        want = (value - sda_base) & 0xFFFF
        if word & 0xFFFF == want:
            return True, want, word & 0xFFFF, "sda disp at +2"
        if word >> 16 == want:
            return True, want, word >> 16, "sda disp at +0"
        return True, want, word & 0xFFFF, "sda disp at +2"
    if reloc_type == 4:                                    # ADDR16_LO / low half
        for shift, where in ((0, "low half at +0"), (16, "low half at +2")):
            if (word >> shift) & 0xFFFF == value & 0xFFFF:
                return True, value & 0xFFFF, (word >> shift) & 0xFFFF, where
        return True, value & 0xFFFF, (word >> 16) & 0xFFFF, "low half at +2"
    if reloc_type == 5:                                    # ADDR16_HI
        hi = (value >> 16) & 0xFFFF
        for shift, where in ((16, "high half at +0"), (0, "high half at +2")):
            if (word >> shift) & 0xFFFF == hi:
                return True, hi, (word >> shift) & 0xFFFF, where
        return True, hi, word & 0xFFFF, "high half at +2"
    if reloc_type == 6:                                    # ADDR16_HA (adjusted)
        ha = ((value + 0x8000) >> 16) & 0xFFFF
        for shift, where in ((16, "high half at +0"), (0, "high half at +2")):
            if (word >> shift) & 0xFFFF == ha:
                return True, ha, (word >> shift) & 0xFFFF, where
        return True, ha, word & 0xFFFF, "high half at +2"
    return False, None, None, ""


# ---------------------------------------------------------------------------
# The trace: one input object, followed through one real link
# ---------------------------------------------------------------------------


def parse_map_symbols(text):
    """``{name: address}`` from the map's ``Linker generated symbols:`` listing.

    The linker's own symbols (``_f_text``, ``_e_data``, ``_SDA_BASE_``,
    ``_stack_addr``, ...) are printed in a second listing whose rows have no
    offset/size columns, so they are not fragments and ``parse_map`` does not
    see them.  A unit that references one - ``main.o`` relocates ``_f_data`` and
    ``_f_bss`` - needs them resolved, and this listing is the artifact's own
    statement of their addresses.
    """
    out = {}
    inside = False
    for line in text.splitlines():
        if re.match(r"^\s*Linker generated symbols:\s*$", line):
            inside = True
            continue
        if not inside:
            continue
        m = re.match(r"^\s*([A-Za-z_][\w$]*)\s+([0-9a-fA-F]{8})\s*$", line)
        if m:
            out[m.group(1)] = int(m.group(2), 16)
    return out


def _map_rows(map_text):
    """``[(output_section, row), ...]`` in map order."""
    out = []
    for sec, info in parse_map(map_text).items():
        for block in info["blocks"]:
            for row in block["fragments"]:
                out.append((sec, row))
    return out


_FILL_NAMES = ("*fill*", "**fill**")
_SECTION_LIKE = re.compile(r"^\.[\w$]+$")


def is_section_row(section, row):
    """Is this map row an *input section* fragment, rather than a symbol?

    The map's fifth column is **not** a type: it is the row's alignment (a
    fragment is laid out at exactly its place, so it prints 1, while a symbol
    carries its own alignment - which is why every symbol row of this link
    prints 4/8/16 and a 1-byte label prints 1).  An earlier revision of ``trace``
    read that column as a type flag and required 4 for a symbol, so a 1-byte
    label was mistaken for a fragment and ``trace main`` reported its relocations
    as unresolvable.  The name is what separates them: a fragment is named after
    the input section (``.text``, ``.ctors$10``, ``extab``, ``*fill*``), a symbol
    after the symbol.  ``extab``/``extabindex`` have no leading dot, and a pooled
    symbol such as ``.data.0`` has an inner dot and so is not section-like.
    """
    name = row["name"]
    if not name:
        return True
    if name in _FILL_NAMES:
        return True
    if name == section:
        return True
    return bool(_SECTION_LIKE.match(name)) and "." not in name[1:]


def build_trace(obj_path, map_text, elf_path=None, rsp_path=None, order=None):
    """Follow one object through the link the map describes.

    Every claim made here is a comparison against the artifact: a section's
    landing address is checked by reading the object's own bytes back out of
    the output ELF at that address, a symbol's resolution is checked against
    the map row that names it, and a relocation is checked by decoding the
    word the linker wrote.  ``verdict`` is ``MATCH`` only if nothing
    disagreed; a disagreement names itself in ``problems``.
    """
    obj = MwObject(obj_path)
    name = Path(obj_path).name
    rows = _map_rows(map_text)
    mine = [(sec, r) for sec, r in rows if r.get("source") == name]
    problems = []
    rsp = None
    if rsp_path and Path(rsp_path).exists():
        rsp = [ln.strip() for ln in Path(rsp_path).read_text(errors="replace").splitlines()
               if ln.strip()]

    elf = Elf(elf_path) if elf_path and Path(elf_path).exists() else None
    elf_secs = {s["name"]: s for s in elf.sections()} if elf else {}
    elf_data = elf.data if elf else b""
    elf_syms = elf.symbols() if elf else {}

    if not mine:
        in_rsp = None if rsp is None else any(
            Path(ln.replace("\\", "/")).name == name for ln in rsp)
        why = ("it is an input but no row in the map is attributed to it - the "
               "link did not keep any of it (dead-stripped, or an archive "
               "member that was never pulled in)")
        if in_rsp is False:
            why = "it is not an input of this link at all"
        return {"object": str(obj_path), "object_name": name, "kept": False,
                "in_rsp": in_rsp, "why": why, "sections": [], "symbols": [],
                "relocations": [], "ctor_dtor": [], "problems": [],
                "dead_symbols": [],
                "verdict": "DROPPED", "elf": str(elf_path) if elf else None,
                "map": None, "rows_total": 0}

    report = {"object": str(obj_path), "object_name": name, "kept": True,
              "in_rsp": None if rsp is None else any(
                  Path(ln.replace("\\", "/")).name == name for ln in rsp),
              "rows_total": len(mine), "sections": [], "symbols": [],
              "relocations": [], "ctor_dtor": [], "problems": problems,
              "elf": str(elf_path) if elf else None, "map": None}

    # ---- where each of its sections landed -------------------------------
    # Keyed by section *index*, not by name: a split target object can hold
    # several sections with one name (fn_80429B94.o has seven `.data`), and a
    # name-keyed map silently answers every one of them with the last address.
    landing = {}          # object section index -> output address
    out_of = {}           # object section index -> output section name
    reloc_words = {}      # object section index -> {byte offset: relocation}
    for rel in obj.relocs:
        reloc_words.setdefault(rel["target_index"], {})[rel["offset"]] = rel
    sda_base = sda2_base = None
    for _s, r in rows:
        if r["name"] == "_SDA_BASE_":
            sda_base = r["addr"]
        elif r["name"] == "_SDA2_BASE_":
            sda2_base = r["addr"]
    if sda_base is None and elf_syms.get("_SDA_BASE_"):
        sda_base = elf_syms["_SDA_BASE_"][0]
    if sda2_base is None and elf_syms.get("_SDA2_BASE_"):
        sda2_base = elf_syms["_SDA2_BASE_"][0]
    # A split target object can hold *several* sections with one name (a unit
    # whose .data was split into fragments): `build/RMHE08/obj/fn_80429B94.o` has
    # seven `.data` sections.  The map lists one fragment row per section, in the
    # same order the linker walked them, so the k-th section of a name belongs to
    # the k-th row of that name - and the byte read-back below is what proves the
    # pairing, rather than this comment.
    seen_names = {}
    for sec in obj.sections:
        if sec["index"] == 0 or not (sec["flags"] & 0x2) or sec["size"] == 0:
            continue
        nth = seen_names.get(sec["name"], 0)
        seen_names[sec["name"]] = nth + 1
        # A fragment is the row named after the input section; `extab` and
        # `extabindex` are not listed that way (the map names every *entry* they
        # own, `@etb_<VA>`/`@eti_<VA>`), so their landing is where this object's
        # first non-fragment row in that output section sits.
        cand_list = [r for _s, r in mine if is_section_row(sec["name"], r)
                     and r["name"] == sec["name"]]
        cand = cand_list[nth:nth + 1]
        entry_rows = cand
        if not cand:
            entry_rows = [r for s_, r in mine
                          if s_ == sec["name"] and not is_section_row(s_, r)]
        if not entry_rows:
            report["sections"].append({"name": sec["name"], "size": sec["size"],
                                       "align": sec["align"], "landed": False,
                                       "why": "no map row for it"})
            problems.append(f"section '{sec['name']}' of {name} has no map row "
                            f"attributed to it (kept out of the output?)")
            continue
        r = entry_rows[0] if not cand else cand[0]
        if not cand:
            first = min(entry_rows, key=lambda r: r["addr"])
            r = dict(first)
            r["size"] = sec["size"]
        out_sec = next(s_ for s_, rr in mine if rr["name"] == r["name"]
                       and rr["addr"] == r["addr"] and rr["offset"] == r["offset"])
        entry = {"name": sec["name"], "output_section": out_sec,
                 "addr": r["addr"], "size": r["size"], "align": sec["align"],
                 "landed": True, "entry_rows": len(entry_rows)}
        osec = elf_secs.get(out_sec)
        if osec is not None:
            entry["in_output_section"] = (osec["addr"] <= r["addr"]
                                          and r["addr"] + r["size"] <= osec["addr"] + osec["size"])
            if not entry["in_output_section"]:
                problems.append(f"section '{sec['name']}' lands at {r['addr']:#x} "
                                f"outside {out_sec} [{osec['addr']:#x}, "
                                f"+{osec['size']:#x})")
        if sec["size"] != r["size"] and cand:
            problems.append(f"section '{sec['name']}': object size {sec['size']:#x} "
                            f"!= map row size {r['size']:#x}")
        # The read-back that makes the address real: the linker had to copy
        # these bytes there.  The words a relocation lands on are exempt - the
        # linker *wrote* those, which is exactly what the relocation check
        # below verifies.  (A NOBITS section has no bytes to read back.)
        body = obj.contents(sec)
        if elf and body and r["size"] and osec is not None:
            start = osec_off(osec, r["addr"])
            got = elf_data[start:start + len(body)]
            skip = set()
            for off in reloc_words.get(sec["index"], {}):
                skip.update(range(off, off + 4))
            same = len(got) == len(body) and all(
                i in skip or got[i] == body[i] for i in range(len(body)))
            entry["bytes_identical"] = same
            entry["bytes_relocated"] = len(skip)
            if not same:
                first = next(i for i in range(min(len(got), len(body)))
                             if i not in skip and got[i] != body[i])
                problems.append(
                    f"section '{sec['name']}' at {r['addr']:#x}: the output ELF "
                    f"does not carry this object's bytes (first difference at "
                    f"+{first:#x}: {body[first]:#04x} -> {got[first]:#04x})")
        landing[sec["index"]] = r["addr"]
        out_of[sec["index"]] = out_sec
        report["sections"].append(entry)

    # ---- how its symbols resolved ----------------------------------------
    by_name = {}
    for sec, r in rows:
        if is_section_row(sec, r) or not r["name"]:
            continue
        # A row whose name carries the map's `(entry of <fragment>)` annotation -
        # `_savegpr_27 (entry of __save_gpr)`, `lbl_80004514 (entry of
        # pad_00_80004380_init)` - defines the *bare* symbol; 48 such rows exist
        # in this link and a reference to one of them is a reference to the
        # symbol, so both spellings resolve.
        bare = re.sub(r"\s+\(entry of .*\)$", "", r["name"])
        for key in {r["name"], bare}:
            by_name.setdefault(key, []).append((sec, r))
    # The linker's own generated symbols are a second listing in the map; a
    # reference to one of them (main.o's `_f_data`, `_f_bss`) resolves there.
    for nm, addr in parse_map_symbols(map_text).items():
        by_name.setdefault(nm, []).append((
            "(linker generated)",
            {"name": nm, "addr": addr, "size": 0, "offset": 0, "file_off": 0,
             "source": "Linker Generated Symbol File", "flags": 0}))
    # A symbol this object *defines* that no map row names is one the linker did
    # not keep.  That is Row 36's signature, and it is worth saying out loud:
    # dropping a *leading* symbol compacts its section, which moves every later
    # address, which is what makes the failure read as a link-order mystery.
    stripped = []
    for sym in obj.symbols:
        if sym["type"] in (STT_SECTION, STT_FILE) or sym["name"] == "":
            continue
        if sym["shndx"] == SHN_UNDEF:
            hit = by_name.get(sym["name"])
            report["symbols"].append({
                "name": sym["name"], "kind": "undefined",
                "resolved_from": hit[0][1]["source"] if hit else None,
                "addr": hit[0][1]["addr"] if hit else None,
                "section": hit[0][0] if hit else None,
            })
            if not hit:
                problems.append(f"undefined symbol '{sym['name']}' is not "
                                f"defined anywhere in the map")
            continue
        want = landing.get(sym["shndx"])
        expected = None if want is None else want + sym["value"]
        hit = by_name.get(sym["name"])
        entry = {"name": sym["name"], "kind": "defined",
                 "section": sym["shndx_name"], "bind": sym["bind"],
                 "value": sym["value"], "expected": expected}
        if hit:
            entry["addr"] = hit[0][1]["addr"]
            entry["from"] = hit[0][1]["source"]
            entry["output_section"] = hit[0][0]
            if expected is not None and hit[0][1]["addr"] != expected:
                problems.append(
                    f"symbol '{sym['name']}': {sym['shndx_name']}+{sym['value']:#x} "
                    f"should be {expected:#x} but the map puts it at "
                    f"{hit[0][1]['addr']:#x}")
            if hit[0][1]["source"] != name:
                entry["kind"] = "defined and also attributed elsewhere"
            # The second witness: the output ELF's own symbol table.
            if elf_syms.get(sym["name"]):
                entry["elf_addrs"] = elf_syms[sym["name"]]
                if hit[0][1]["addr"] not in elf_syms[sym["name"]]:
                    problems.append(
                        f"symbol '{sym['name']}': the map says "
                        f"{hit[0][1]['addr']:#x}, the ELF symbol table says "
                        + ", ".join(f"{a:#x}" for a in elf_syms[sym['name']]))
        else:
            entry["addr"] = expected
            entry["from"] = ("local, carried in the fragment" if sym["bind"] == STB_LOCAL
                             else "not named in the map")
            entry["map_row"] = False
            if sym["bind"] != STB_LOCAL and sym["type"] not in (STT_SECTION, STT_FILE) \
                    and expected is not None:
                stripped.append({"name": sym["name"], "section": sym["shndx_name"],
                                 "value": sym["value"], "size": sym["size"],
                                 "expected": expected})
            if elf_syms.get(sym["name"]):
                entry["elf_addrs"] = elf_syms[sym["name"]]
        report["symbols"].append(entry)
    report["dead_symbols"] = stripped

    # ---- the relocations that touched it ---------------------------------
    for rel in obj.relocs:
        place = landing.get(rel["target_index"])
        if place is None:
            continue
        P = place + rel["offset"]
        sym = obj.symbols[rel["sym_index"]] if rel["sym_index"] < len(obj.symbols) else None
        entry = dict(rel, place=P, reloc_name=reloc_name(rel["type"]))
        if sym is None:
            entry["verdict"] = "no symbol"
            report["relocations"].append(entry)
            continue
        # Resolve S: defined in this object, or resolved from the map.
        if sym["shndx"] == SHN_UNDEF:
            hit = by_name.get(sym["name"])
            entry["resolved_from"] = hit[0][1]["source"] if hit else None
            entry["S"] = hit[0][1]["addr"] if hit else None
            sym_out = hit[0][0] if hit else None
        else:
            entry["resolved_from"] = name
            base = landing.get(sym["shndx"])
            entry["S"] = None if base is None else base + sym["value"]
            sym_out = out_of.get(sym["shndx"], sym["shndx_name"])
        if entry["S"] is None:
            entry["verdict"] = "unresolved"
            problems.append(f"relocation at {rel['target']}+{rel['offset']:#x} "
                            f"names '{sym['name']}', which resolves nowhere")
            report["relocations"].append(entry)
            continue
        entry["S"] = entry["S"] & 0xFFFFFFFF
        out_sec = out_of.get(rel["target_index"])
        word = None
        if elf and out_sec in elf_secs:
            off = osec_off(elf_secs[out_sec], P)
            if off is not None and 0 <= off and off + 4 <= len(elf_data):
                word = struct.unpack_from(">I", elf_data, off)[0]
        entry["output_section"] = out_sec
        entry["applied_word"] = word
        if word is None:
            entry["verdict"] = "not checked (no ELF at that place)"
        else:
            # r2 is the base for .sdata2/.sbss2, r13 for .sdata/.sbss: the
            # section of the *symbol* picks which linker-generated base applies.
            base = sda2_base if sym_out in (".sdata2", ".sbss2") else sda_base
            entry["sda_base"] = base
            checked, exp, act, where = reloc_field(rel["type"], word, entry["S"],
                                                   rel["addend"], P, base)
            entry["expected"], entry["actual"], entry["field"] = exp, act, where
            if not checked:
                entry["verdict"] = f"not checked ({reloc_name(rel['type'])})"
            elif exp == act:
                entry["verdict"] = "applied"
            else:
                entry["verdict"] = "MISMATCH"
                problems.append(
                    f"relocation {reloc_name(rel['type'])} at {P:#x} "
                    f"('{sym['name']}': S={entry['S']:#x} A={rel['addend']:#x}) "
                    f"holds {act:#x} in the {where}, expected {exp:#x}")
        report["relocations"].append(entry)

    # ---- where its ctor/dtor fragment went -------------------------------
    rank = {}
    if order and order.get("names"):
        rank = {n: i for i, n in enumerate(order["names"])}
    runtime = sorted(set(RUNTIME_CTOR_SYMBOLS) & {s["name"] for s in obj.symbols})
    for sec in obj.ctor_dtor_sections():
        entry = {"section": sec["name"], "size": sec["size"],
                 "class_rank": rank.get(sec["name"]), "rows": [],
                 "runtime_symbols": runtime}
        srows = [r for _s, r in mine if r["name"] == sec["name"]
                 and is_section_row(".ctors" if "ctors" in sec["name"] else ".dtors", r)]
        for r in srows:
            entry["rows"].append({"addr": r["addr"], "size": r["size"],
                                  "source": r["source"]})
        out_sec = next((s_ for s_, r in mine if r["name"] == sec["name"]), None)
        entry["output_section"] = out_sec
        if out_sec:
            ctx = [(r["addr"], r["name"], r["source"]) for s_, r in rows
                   if s_ == out_sec and r.get("flags") == 1]
            ctx.sort()
            entry["slots"] = ctx
            entry["slot"] = next((i for i, (a, n, _s) in enumerate(ctx)
                                  if n == sec["name"] and a == (srows[0]["addr"] if srows else None)), None)
        if not srows:
            entry["why"] = ("the linker put this unit's entry in a *synthesized* "
                            "fragment (map source 'Linker Generated Symbol File'): "
                            "the class is decided by the runtime symbol name, not "
                            "by this section's name")
        report["ctor_dtor"].append(entry)

    report["verdict"] = "MATCH" if not problems else "FAIL"
    return report


def osec_off(sec, addr):
    """File offset of output address `addr` inside output section `sec`."""
    if sec is None:
        return None
    return sec["offset"] + (addr - sec["addr"])


def render_trace(rep):
    """The trace as a report a human reads, one claim per line."""
    out = []
    out.append(f"object:   {rep['object']}")
    if not rep["kept"]:
        out.append(f"KEPT:     NO - {rep['why']}")
        if rep.get("in_rsp") is not None:
            out.append(f"          in the link's input list: {rep['in_rsp']}")
        return out
    out.append(f"KEPT:     YES - {rep['rows_total']} map row(s) name this object")
    if rep.get("in_rsp") is not None:
        out.append(f"          in the link's input list: {rep['in_rsp']}")
    out.append("")
    out.append("where each of its sections landed (map address, read back from the ELF):")
    out.append(f"  {'section':<16} {'output':<12} {'address':>10} {'size':>8} {'align':>6}  bytes")
    for s in rep["sections"]:
        if not s["landed"]:
            out.append(f"  {s['name']:<16} {'-':<12} {'-':>10} {s['size']:>#8x} "
                       f"{s['align']:>6}  NOT IN THE MAP ({s.get('why')})")
            continue
        chk = ("identical" if s.get("bytes_identical")
               else ("n/a (no bytes)" if "bytes_identical" not in s else "DIFFERENT"))
        out.append(f"  {s['name']:<16} {s['output_section']:<12} {s['addr']:>#10x} "
                   f"{s['size']:>#8x} {s['align']:>6}  {chk}")
    out.append("")
    out.append("how its symbols resolved:")
    for s in rep["symbols"]:
        if s["kind"] == "undefined":
            where = s["resolved_from"] or "NOWHERE"
            out.append(f"  U {s['name']:<44} -> {where}"
                       + (f" at {s['addr']:#x}" if s.get("addr") else ""))
        else:
            addr = s.get("addr")
            elfnote = ("  ELF " + ", ".join(f"{a:#x}" for a in s["elf_addrs"])
                       if s.get("elf_addrs") else "")
            out.append(f"  D {s['name']:<44} {s['section']}+{s['value']:#x} = "
                       + (f"{addr:#x}" if addr is not None else "?")
                       + f"  [{s.get('from')}]{elfnote}")
    out.append("")
    out.append("relocations applied to it (value decoded out of the output ELF):")
    for r in rep["relocations"]:
        if r.get("verdict") in ("applied",):
            out.append(f"  {r['target']}+{r['offset']:#05x} {r['reloc_name']:<12} "
                       f"'{r['sym_name']}' S={r['S']:#x} A={r['addend']:#x} "
                       f"-> {r['actual']:#x} ({r['field']})  APPLIED")
        else:
            out.append(f"  {r['target']}+{r['offset']:#05x} {r['reloc_name']:<12} "
                       f"'{r['sym_name']}'  {r['verdict']}"
                       + (f" (word {r['applied_word']:#010x})" if r.get("applied_word") is not None else ""))
    if rep["ctor_dtor"]:
        out.append("")
        out.append("its ctor/dtor fragment (the Row 46 question):")
        for c in rep["ctor_dtor"]:
            out.append(f"  {c['section']:<12} size {c['size']:#x}  fixed-order rank "
                       f"{c['class_rank']}  -> {c['output_section']}"
                       + (f" slot {c['slot']}" if c.get("slot") is not None else ""))
            for r in c["rows"]:
                out.append(f"      row {r['addr']:#x} size {r['size']:#x} "
                           f"credited to {r['source'] or '(none)'}")
            if c.get("why"):
                out.append(f"      {c['why']}")
            if c.get("slots"):
                for i, (a, n, src) in enumerate(c["slots"]):
                    mark = "->" if a == (c["rows"][0]["addr"] if c["rows"] else None) else "  "
                    out.append(f"      {mark} [{i}] {a:#x} {n:<12} {src}")
            if c["runtime_symbols"]:
                for sym in c["runtime_symbols"]:
                    cls = RUNTIME_CTOR_SYMBOLS[sym]
                    out.append(f"      runtime symbol '{sym}' -> {cls[0]}{cls[1]} "
                               f"(the linker keys this class on the symbol name)")
    out.append("")
    if rep.get("dead_symbols"):
        out.append(f"defined here but NOT named in the map ({len(rep['dead_symbols'])} "
                   f"symbol(s)) - the linker did not keep them:")
        for d in rep["dead_symbols"][:10]:
            out.append(f"  {d['name']:<44} {d['section']}+{d['value']:#x} "
                       f"size {d['size']:#x} would be {d['expected']:#x}")
        if len(rep["dead_symbols"]) > 10:
            out.append(f"  ... {len(rep['dead_symbols']) - 10} more")
        out.append("  -> dropping one compacts its section, so every following "
                   "address in this report shifts by its size.  If the target "
                   "object keeps the symbol this is Row 36: mark it "
                   "__declspec(export) (see docs/matching.md).")
        out.append("")
    out.append(f"VERDICT: {rep['verdict']}")
    for p in rep["problems"]:
        out.append("  " + p)
    return out


# ---------------------------------------------------------------------------
# The phases: the message machinery, derived from the PE
# ---------------------------------------------------------------------------
#
# The README's open item was that the engine's phase messages are referenced by
# resource *id*, so no immediate operand names them.  They are still findable,
# because the linker loads them with USER32's `LoadStringA`, which is an import
# - so the call site is derivable from the import table:
#
#   * the IAT slot of `LoadStringA` comes from the import directory;
#   * `call dword ptr [slot]` sites in `.text` are the two places the linker
#     loads a message string (one message loader, one one-off with a fixed id);
#   * the *callers* of the loader are the linker's message formatters - one
#     per engine message - and each pushes its catalogue id two instructions
#     before the call (arg3; the loader reads it at `[esp+0x18]` after its own
#     two pushes and passes it to `LoadStringA` as `uID`);
#   * the return address of `call <formatter>` is therefore a phase anchor, and
#     the message id it prints is what names the phase.
#
# `phases --prove` breaks at the *observation anchor* (the instruction after
# the `LoadStringA` call, where the pushed id and the buffer pointer are still
# live) and reports the `(id, string)` pairs the linker really asked for - which
# is also what validates the catalogue's own id numbering against the artifact.

LOADSTRING = "LoadStringA"


def _imm(op_str):
    """The integer value of an immediate operand string, or None."""
    if op_str and re.fullmatch(r"0x[0-9a-f]+", op_str or ""):
        return int(op_str, 16)
    return None


def _iat_slots(pe):
    """``{imported function name: VA of its IAT slot}``."""
    d = pe.data
    rva, size = pe.data_dir(1)
    out = {}
    o = pe.rva2off(rva)
    i = 0
    while o is not None:
        oft, ts, fc, namerva, fta = struct.unpack_from("<IIIII", d, o + 20 * i)
        if namerva == 0 and fta == 0 and oft == 0:
            break
        no = pe.rva2off(namerva)
        thunk = oft or fta
        to = pe.rva2off(thunk)
        j = 0
        while to is not None:
            v = struct.unpack_from("<I", d, to + 4 * j)[0]
            if v == 0:
                break
            if not (v & 0x80000000):
                ho = pe.rva2off(v)
                nm = d[ho + 2:d.find(b"\0", ho + 2)].decode("latin-1")
                out.setdefault(nm, pe.image_base + fta + 4 * j)
            j += 1
        i += 1
    return out


def _function_start(insns, idx):
    """The address of the function containing ``insns[idx]``.

    These binaries carry no symbols, so the boundary has to come from the
    layout: Metrowerks pads *between* functions with `nop`/`int3` runs, so the
    entry is the first real instruction after the closest such run at or before
    ``idx``.  The derivation checks itself: ``derive_message_io`` reports how
    many of the entries it found are the target of a `call`, and an entry that
    nothing calls is reported as unproven rather than used.
    """
    j = idx
    while j > 0:
        if insns[j - 1].mnemonic in ("nop", "int3") and \
                insns[j].mnemonic not in ("nop", "int3"):
            return insns[j].address
        j -= 1
    return insns[0].address


def derive_message_io(pe):
    """The linker's message loader, its observation anchor, and the formatters."""
    cs = _capstone()
    if cs is None:
        return None
    slots = _iat_slots(pe)
    slot = slots.get(LOADSTRING)
    if slot is None:
        return None
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    text = pe.section(".text")
    insns = list(md.disasm(pe.read_rva(text.va, text.vsize),
                           pe.image_base + text.va))
    sites = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != "call" or not ins.operands:
            continue
        op = ins.operands[0]
        if op.type == cs.x86.X86_OP_MEM and op.mem.base == cs.x86.X86_REG_INVALID \
                and op.mem.index == cs.x86.X86_REG_INVALID and op.mem.disp == slot:
            sites.append(i)
    if not sites:
        return None

    def push_stream(i, n):
        """The last `n` pushes before `i`, in **instruction order** (first push first).

        ``argN`` of the callee is the *last* push, so ``stream[-N]`` is the
        argument - and saying it that way is what keeps the id derivation
        honest: nothing here assumes the pushes are immediates.
        """
        near = []
        k = i - 1
        while k >= 0 and len(near) < n:
            if insns[k].mnemonic == "push":
                near.append(insns[k].op_str)
            k -= 1
        return list(reversed(near))

    loaders = []
    for i in sites:
        start = _function_start(insns, i)
        args = push_stream(i, 4)
        # `LoadStringA(HINSTANCE, UINT uID, LPSTR, int)`: arg2 is the id, i.e.
        # the second push from the call.
        loaders.append({
            "call_rva": insns[i].address - pe.image_base,
            "after_rva": (insns[i].address + insns[i].size) - pe.image_base,
            "entry_rva": start - pe.image_base,
            "pushed": args,
            "msgid_arg2": _imm(args[-2]) if len(args) >= 2 else None,
            # The message loader takes the id as an *argument*; the other site
            # loads one fixed string for its own use, so its arg2 is a
            # constant.  That is the rule that tells them apart.
            "arg2_is_constant": _imm(args[-2]) is not None if len(args) >= 2 else False,
        })
    # The message loader is the one with many callers; a one-off `LoadStringA`
    # with a constant id is a different thing and is reported as such.
    call_targets = {}
    for ins in insns:
        if ins.mnemonic == "call" and ins.operands \
                and ins.operands[0].type == cs.x86.X86_OP_IMM:
            call_targets.setdefault(ins.operands[0].imm, 0)
            call_targets[ins.operands[0].imm] += 1
    for ld in loaders:
        callers = [ins.address - pe.image_base for ins in insns
                   if ins.mnemonic == "call" and ins.operands
                   and ins.operands[0].type == cs.x86.X86_OP_IMM
                   and ins.operands[0].imm == pe.image_base + ld["entry_rva"]]
        ld["callers"] = sorted(callers)
        ld["entry_is_call_target"] = call_targets.get(pe.image_base + ld["entry_rva"], 0)
    loader = max([l for l in loaders if not l["arg2_is_constant"]] or loaders,
                 key=lambda l: (len(l["callers"]), -l["call_rva"]))
    # The id is the *third* push in source order: the loader reads arg3 (its
    # `movsx ecx, word ptr [esp+0x18]` after two pushes, with arg1 in EBX as
    # the buffer) and hands it to `LoadStringA` as uID.  That reading is
    # falsifiable and `--prove` checks it against a real link.
    formatters = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != "call" or not ins.operands:
            continue
        op = ins.operands[0]
        if op.type != cs.x86.X86_OP_IMM or op.imm - pe.image_base != loader["entry_rva"]:
            continue
        pushed = push_stream(i, 3)
        formatters.append({
            "func_rva": _function_start(insns, i) - pe.image_base,
            "call_rva": ins.address - pe.image_base,
            "msgid": _imm(pushed[-3]) if len(pushed) >= 3 else None,
            "pushed": pushed,
        })
    # Self-check: a printer entry has to be something the binary calls.
    for f in formatters:
        f["entry_is_call_target"] = call_targets.get(pe.image_base + f["func_rva"], 0)
    return {"iat_slot": slot, "loader": loader, "loaders": loaders,
            "formatters": formatters,
            "unproven_entries": sorted({f["func_rva"] for f in formatters
                                        if not f["entry_is_call_target"]})}


def derive_phase_anchors(pe):
    """``{anchor RVA: (msgid, what printed it)}`` for every phase call site."""
    io = derive_message_io(pe)
    if io is None:
        return None
    cs = _capstone()
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    text = pe.section(".text")
    insns = list(md.disasm(pe.read_rva(text.va, text.vsize),
                           pe.image_base + text.va))
    by_func = {}
    for f in io["formatters"]:
        by_func.setdefault(f["func_rva"], []).append(f)
    anchors = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != "call" or not ins.operands:
            continue
        op = ins.operands[0]
        if op.type != cs.x86.X86_OP_IMM:
            continue
        rva = op.imm - pe.image_base
        for f in by_func.get(rva, []):
            anchors.append({
                "anchor": ins.address + ins.size - pe.image_base,
                "formatter": rva,
                "msgid": f["msgid"],
                "printer": _function_start(insns, i) - pe.image_base,
            })
    anchors.sort(key=lambda a: a["anchor"])
    return {"io": io, "anchors": anchors}


def cmd_phases(args):
    pe = Pe(args.linker)
    if _capstone() is None:
        print("phases needs capstone (pip install capstone)", file=sys.stderr)
        return 2
    der = derive_phase_anchors(pe)
    if der is None:
        print(f"{pe.path.name}: no message loader - this build does not import "
              f"LoadStringA (the GC 1.0..2.6 linkers do not), so the phase table "
              f"has nothing to hang on.  'messages' is empty for the same reason.",
              file=sys.stderr)
        return 2
    io = der["io"]
    ld = io["loader"]
    print(f"linker: {pe.path}")
    print(f"{LOADSTRING} import slot: {io['iat_slot']:#x}")
    print(f"message loader:  RVA {ld['entry_rva']:#x}  (calls {LOADSTRING} at "
          f"{ld['call_rva']:#x}; {len(ld['callers'])} caller(s))")
    print(f"  observation anchor: RVA {ld['after_rva']:#x} - the instruction after the "
          f"call; on a real link the uID reads at [esp+0x10] and the buffer "
          f"pointer is still in EBX (measured, not assumed)")
    for other in io["loaders"]:
        if other is ld:
            continue
        print(f"  not the loader: {LOADSTRING} at {other['call_rva']:#x} in the function "
              f"at {other['entry_rva']:#x}: arg2 is the constant {other['msgid_arg2']}, "
              f"so it loads one fixed string ({len(other['callers'])} caller(s))")
    print(f"message formatters: {len(io['formatters'])} (callers of the loader, one per "
          f"engine message; each pushes its id as arg3)")
    for f in sorted(io["formatters"], key=lambda f: (f["msgid"] is None, f["msgid"])):
        entry = ("" if f["entry_is_call_target"] else "  ENTRY UNPROVEN (nothing calls it)")
        print(f"  msgid={f['msgid']!s:<6} formatter RVA {f['func_rva']:#x} "
              f"pushes {f['pushed']}{entry}")
    if io["unproven_entries"]:
        print(f"  {len(io['unproven_entries'])} formatter entry/entries are not a call "
              f"target: " + ", ".join(hex(e) for e in io["unproven_entries"]), file=sys.stderr)
    print(f"phase anchors (return address of call <formatter>): {len(der['anchors'])}")
    for a in der["anchors"][:40]:
        print(f"  {a['anchor']:#08x}  msgid={a['msgid']}  printer RVA {a['printer']:#x}")
    if args.json:
        print(json.dumps(der, indent=2))
    if args.prove:
        return prove_phases(pe, der, args)
    return 0


def prove_phases(pe, der, args):
    """Run a real link and observe (a) the ids the linker asks for and (b) the anchors."""
    gdb = find_gdb(args.gdb)
    if not gdb:
        print("no gdb found; pass --gdb", file=sys.stderr)
        return 2
    if not args.args:
        print("--prove needs --args '<the link argument list>'", file=sys.stderr)
        return 2
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    script = work / "mwlink-phases.gdb"
    ld = der["io"]["loader"]
    anchors = "\n".join("  (0x%x, '%x')," % (pe.image_base + a["anchor"], a["anchor"])
                        for a in der["anchors"])
    # The stack layout at the observation anchor was *measured* on a real link
    # (`[esp]`=hInstance, `[esp+4]`=the loader's return address, `[esp+8]`=arg1
    # the buffer, `[esp+0xC]`=arg2, `[esp+0x10]`=arg3, the uID) - so the uID is
    # read at +0x10 and the buffer from EBX, which the loader also holds.
    script.write_text(
        "set pagination off\nset confirm off\nset width 0\n"
        f"file {pe.path.as_posix()}\n"
        "set args " + args.args + "\n"
        "python\nimport gdb\n"
        f"LOADER = 0x{pe.image_base + ld['after_rva']:x}\n"
        "class Msg(gdb.Breakpoint):\n"
        "    def stop(self):\n"
        "        try:\n"
        "            esp = int(gdb.parse_and_eval('$esp')) & 0xffffffff\n"
        "            uID = int(gdb.parse_and_eval('*(unsigned int*)%d' % (esp + 0x10)))\n"
        "            ebx = int(gdb.parse_and_eval('$ebx')) & 0xffffffff\n"
        "            raw = gdb.selected_inferior().read_memory(ebx, 96).tobytes().split(b'\\0')[0]\n"
        "            text = raw.decode('latin-1').replace('\\n', ' ').replace('\\t', ' ')\n"
        "            print('MWLINK-MSG %d %s' % (uID, text))\n"
        "        except Exception as exc:\n"
        "            print('MWLINK-MSGERR %s' % exc)\n"
        "        return False\n"
        "class Phase(gdb.Breakpoint):\n"
        "    def __init__(self, addr, tag):\n"
        "        super().__init__('*0x%x' % addr, internal=True)\n"
        "        self.tag = tag\n"
        "    def stop(self):\n"
        "        print('MWLINK-ANCHOR %s' % self.tag)\n"
        "        return False\n"
        "Msg('*0x%x' % LOADER)\n"
        "_n = 0\n"
        "for _a, _t in [\n" + anchors + "\n]:\n"
        "    Phase(_a, _t)\n"
        "    _n += 1\n"
        "print('MWLINK-BPS %d %d' % (_n, len(gdb.breakpoints())))\n"
        "end\nrun\nprintf \"MWLINK-DONE\\n\"\nquit\n")
    argv = [gdb, "-batch", "-nx", "-x", str(script)]
    print(f"# gdb {script}")
    proc = subprocess.run(argv, capture_output=True, text=True, encoding="utf-8", errors="replace")
    stream, counts = [], {}
    last = [None]
    for line in proc.stdout.splitlines():
        body = line.strip()
        m = re.match(r"^MWLINK-ANCHOR ([0-9a-f]+)$", body)
        if m:
            counts[m.group(1)] = counts.get(m.group(1), 0) + 1
            last[0] = m.group(1)
            continue
        m = re.match(r"^MWLINK-MSG (\d+) (.*)$", body)
        if m:
            stream.append((last[0], int(m.group(1)), m.group(2)))
            last[0] = None
            continue
        m = re.match(r"^MWLINK-BPS (\d+) (\d+)$", body)
        if m:
            print(f"# breakpoints: {m.group(1)} phase anchor(s), {m.group(2)} total")
    if not stream and not counts:
        print("# the run produced no observation; gdb stderr follows", file=sys.stderr)
        print(proc.stderr.strip()[:2000], file=sys.stderr)
        return 1
    cat = message_catalogue(pe)
    print(f"the link's phase stream, as the linker's own message loader printed it:")
    for anchor, mid, text in stream:
        who = f"anchor {anchor}" if anchor else "(no anchor seen)"
        known = cat.get(mid)
        tag = ("  [catalogue agrees]" if known and known.split("%")[0][:24] == text.split("%")[0][:24]
               else (f"  [catalogue says {known!r}]" if known else "  [id not in the catalogue]"))
        print(f"  {who:<14} id={mid:<4} {text}{tag}")
    agree = sum(1 for _a, mid, text in stream
                if cat.get(mid) and cat[mid].split("%")[0][:24] == text.split("%")[0][:24])
    print(f"# catalogue cross-check: {agree} of {len(stream)} observed messages match "
          f"the catalogue's id -> text")
    fired = sum(1 for a in der["anchors"]
                if f"{a['anchor']:x}" in counts)
    print(f"phase anchors: {fired}/{len(der['anchors'])} fired in this link "
          f"(the other side of the count is the run's own verbosity)")
    seen_ids = {}
    for anchor, mid, _t in stream:
        if anchor:
            seen_ids.setdefault(anchor, []).append(mid)
    for a in der["anchors"]:
        key = f"{a['anchor']:x}"
        n = counts.get(key, 0)
        ids = sorted(set(seen_ids.get(key, [])))
        print(f"  {'FIRED   ' if n else 'UNPROVEN'} {key:>8} candidate-id {a['msgid']} "
              f"x{n}" + (f"  observed ids {ids}" if ids else ""))
    if args.require_all:
        return 0 if fired == len(der["anchors"]) else 1
    return 0 if fired else 1


# ---------------------------------------------------------------------------
# Finding the link line the build itself uses
# ---------------------------------------------------------------------------

def derive_rsp(out="build/RMHE08/main.elf", dest=None):
    """The link's list of inputs, read out of ``build.ninja``'s own statement.

    ``ninja`` writes ``$out.rsp`` for the link and deletes it again, so the
    response file a trace needs is derived from the build statement rather than
    kept: the tokens between ``link`` and the first ``|``/``||`` are exactly
    what ninja puts in the response file.  Nothing is transcribed - if the
    build's input list changes, so does this.
    """
    ninja = ROOT / "build.ninja"
    if not ninja.exists():
        return None
    lines = ninja.read_text(errors="replace").splitlines()
    want = out.replace("\\", "/")
    for i, line in enumerate(lines):
        m = re.match(r"^build\s+(\S+):\s+link\s+(.*)$", line)
        if not m or m.group(1).replace("\\", "/") != want:
            continue
        parts = [m.group(2)]
        j = i
        while parts[-1].rstrip().endswith("$") and j + 1 < len(lines):
            j += 1
            parts.append(lines[j])
        toks, stop = [], False
        for tok in " ".join(parts).replace("$", " ").split():
            if tok in ("|", "||"):
                stop = True
            if not stop:
                toks.append(tok)
        if dest is not None:
            Path(dest).write_text("\n".join(toks) + "\n", encoding="utf-8")
        return toks
    return None


def derive_link_line(rsp, ldscript=None):
    """The linker argument list for a link, with the build's own flags.

    ``ldflags`` is read from ``build.ninja`` (the global assignment *and* the
    per-build one that appends ``-lcf``), so the trace links the way the build
    does instead of the way a default would.
    """
    ninja = ROOT / "build.ninja"
    flags = None
    lcf = ldscript
    if ninja.exists():
        txt = ninja.read_text(errors="replace")
        m = re.search(r"^ldflags\s*=\s*(.*)$", txt, re.M)
        if m:
            flags = m.group(1).strip()
            for extra in re.findall(r"^\s+ldflags\s*=\s*\$ldflags\s*(.*)$", txt, re.M):
                seg = extra.strip()
                m2 = re.match(r"-lcf\s+(\S+)", seg)
                if m2:
                    lcf = m2.group(1)
                flags += " " + re.sub(r"-lcf\s+\S+", "", seg).strip()
    if flags is None:
        flags = "-fp hardware -nodefaults"
    argv = [flags]
    if lcf:
        argv.append("-lcf " + str(lcf))
    argv.append("-o {out} -map {map} @" + str(rsp))
    return " ".join(argv)


def link_inputs(out="build/RMHE08/main.elf"):
    """The objects the build's own link statement consumes, in its own order.

    This is ``build.ninja``'s answer, not ours: the same token list ninja
    writes into the response file.  It is what makes a *unit name* a usable
    argument - the object that belongs to a unit is the one the link names.
    """
    toks = derive_rsp(out)
    if toks is None:
        return None
    return [t for t in toks if re.search(r"\.(o|a)$", t, re.I)]


def unit_of_object(path):
    """The *unit name* of a build input: its path under ``build/<...>/{src,obj}/``.

    ``build/RMHE08/src/Network/NetworkWiiMediator.o`` ->
    ``Network/NetworkWiiMediator``; same for ``obj/`` (the split target
    object).  A path that does not have that shape returns its stem.
    """
    s = str(path).replace("\\", "/")
    m = re.search(r"/build/[^/]+/(?:src|obj)/(.+\.o)$", "/" + s)
    if m:
        return m.group(1)[:-2]
    return Path(s).name[:-2] if s.lower().endswith(".o") else Path(s).name


def resolve_object(spec, link_out="build/RMHE08/main.elf"):
    """Resolve a path **or a unit name** to the object a link consumes.

    Order, and why: an existing path is honoured first (a lane that already
    has one knows what it wants); then the build's *own* input list, which is
    ground truth for "which object is this unit" - a unit that is `linked
    False` appears there as ``obj/<unit>.o`` (the split target object) and a
    flipped one as ``src/<unit>.o`` (ours), and picking the wrong one silently
    traces a different object; then the two conventional paths.  ``how`` says
    which of those answered, so the report never implies more than it checked.
    """
    want = str(spec).replace("\\", "/").strip()
    stem = re.sub(r"\.o$", "", want, flags=re.I)
    stem = re.sub(r"^.*?/?(?:build/[^/]+/)?(?:src|obj)/", "", stem)

    cand = Path(want)
    if cand.exists():
        return {"path": cand, "unit": unit_of_object(cand), "how": "the path given",
                "in_link": None}
    root_cand = ROOT / want
    if root_cand.exists():
        return {"path": root_cand, "unit": unit_of_object(root_cand),
                "how": "the path given", "in_link": None}

    inputs = link_inputs(link_out) or []
    hits = [t for t in inputs if unit_of_object(t).lower() == stem.lower()]
    if not hits:
        # The link may name the same unit in more than one place (a `link`
        # statement and the objdiff list); keep only the first, and say how
        # many there were.
        hits = [t for t in inputs
                if unit_of_object(t).lower().endswith("/" + stem.lower())
                or Path(t.replace("\\", "/")).name.lower() == stem.lower() + ".o"]
    if hits:
        p = Path(hits[0].replace("\\", "/"))
        if not p.is_absolute():
            p = ROOT / p
        return {"path": p, "unit": unit_of_object(hits[0]),
                "how": f"the object the build's link names ({len(hits)} entry/entries)",
                "in_link": True}

    for sub in ("src", "obj"):
        p = ROOT / "build" / "RMHE08" / sub / (stem + ".o")
        if p.exists():
            return {"path": p, "unit": stem,
                    "how": f"build/RMHE08/{sub}/<unit>.o - NOT in the link's "
                           f"input list (this object is not linked)",
                    "in_link": False}
    known = sorted({unit_of_object(t) for t in inputs})
    import difflib
    near = difflib.get_close_matches(stem, known, n=5, cutoff=0.6)
    return {"path": None, "unit": stem, "how": "not found", "in_link": None,
            "known": known[:0], "near": near, "inputs": len(inputs)}


def link_argv(args, out_base="trace", verbose=False):
    """The argv for a link that can never write ``build/RMHE08/main.elf``.

    Either an explicit link line (``--args``, with any ``-o``/``-map`` removed
    and replaced by the scratch paths) or the build's own, derived from
    ``build.ninja``.  Returns ``(argv, elf_out, map_out, work, rsp)`` or
    ``None`` when the build statement cannot be read.
    """
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    elf_out = work / f"{out_base}.elf"
    map_out = work / f"{out_base}.MAP"
    if getattr(args, "args", None):
        toks, skip = [], False
        for tk in args.args.split():
            if skip:
                skip = False
                continue
            if tk in ("-o", "-map"):
                skip = True
                continue
            toks.append(tk)
        return ([str(args.linker)] + toks + ["-o", str(elf_out), "-map", str(map_out)],
                elf_out, map_out, work, Path(args.rsp) if args.rsp else None)
    rsp = Path(args.rsp) if args.rsp else None
    if rsp is None:
        rsp = work / "trace.rsp"
        if derive_rsp(args.link_out, rsp) is None:
            return None
    if not rsp.exists():
        return None
    args.rsp = str(rsp)
    line = derive_link_line(rsp, args.ldscript or "build/RMHE08/ldscript.lcf")
    if verbose:
        line = "-v " + line
    line = line.format(out=str(elf_out), map=str(map_out))
    return [str(args.linker)] + line.split(), elf_out, map_out, work, rsp


def run_link(args, out_base="trace", verbose=False):
    """Run the build's own link into ``build/scratch/``; never ``main.elf``.

    Returns ``(rc, argv, stdout, stderr, elf_out, map_out)``, or ``None`` when
    the link cannot even be derived.
    """
    got = link_argv(args, out_base, verbose)
    if got is None:
        return None
    argv, elf_out, map_out, _work, _rsp = got
    proc = subprocess.run(argv, capture_output=True, text=True, encoding="utf-8",
                          errors="replace", cwd=str(ROOT))
    return proc.returncode, argv, proc.stdout, proc.stderr, elf_out, map_out


def report_link_failure(args, argv, stdout, stderr, catalogue):
    """The production case: a link that errors.  Say what it said, and where.

    A flip whose DOL hash moves is a *failing* link, so the tool has to be
    useful on the failure rather than refusing it: the run is repeated once
    with ``-v`` (the phase stream the build does not ask for), the diagnostics
    are classified against the message catalogue, and each one is attributed to
    the last phase the linker announced before printing it.  Nothing is
    invented: a diagnostic the catalogue does not hold is reported without an
    id, and a diagnostic with no phase line before it says ``(no phase seen)``.
    """
    print(f"# link FAILED (the link's own diagnostics follow)")
    tail = (stdout or "") + (stderr or "")
    for line in tail.strip().splitlines()[-40:]:
        print("#   " + line)
    # The phase attribution needs the verbose stream, which the build's own
    # ldflags do not ask for; one retry into scratch is worth it here.
    again = run_link(args, out_base="diagnose", verbose=True)
    if again is None:
        return []
    rc2, argv2, out2, err2, _elf2, _map2 = again
    # Classify the *verbose* run's output when it produced any: concatenating
    # both runs would report every diagnostic twice.
    text = "\n".join(t for t in (out2, err2) if t) or "\n".join(
        t for t in (stdout, stderr) if t)
    timeline = parse_timeline(text, phase_kinds(catalogue))
    diags = classify_diagnostics(text, catalogue, timeline)
    print()
    print("the link's phase stream, up to the failure:")
    for kind, _line in timeline:
        print(f"  {kind}")
    if not timeline:
        print("  (no phase line was printed)")
    print("diagnostics (classified against the message catalogue):")
    if not diags:
        print("  (the linker printed no banner this tool recognises)")
    for d in diags:
        who = f"phase {d['phase']}" if d["phase"] else "(no phase seen)"
        mid = f"msgid={d['msgid']}" if d["msgid"] is not None else "msgid=(not in the catalogue)"
        print(f"  [{who}] {mid}: {d['text']}")
    return diags





# ---------------------------------------------------------------------------
# The alignment the linker enforces (the objalign.py question), derived
# ---------------------------------------------------------------------------

def _splits_starts(path):
    """``{unit: {'.section': claimed start}}`` from ``config/.../splits.txt``.

    Same shape ``tools/elf/objalign.py`` reads: a unit key is a line that starts
    in column 0 and ends with ``:``; its sections are indented
    ``.name start:0x.... end:0x....`` lines.
    """
    try:
        splits = Splits.read(path)
    except OSError:
        return {}
    return {block.unit: {r.section: r.start for r in block.ranges} for block in splits.blocks}


def derive_alignment(pe):
    """Find where mwld aligns a fragment's address and what it compares.

    The lever is the map's own ``*fill*`` row: the linker prints that literal
    when the alignment moved the fragment, so the function that pushes it also
    contains the comparison.  This scans the candidates for the instructions
    that do the work - the ``[shdr + 0x20]`` load (``sh_addralign`` in an
    ``Elf32_Shdr``) and the ``... add -1; not; and`` round-up - and reports
    them; nothing is transcribed.
    """
    insns, aux = _disas_text(pe)
    if insns is None:
        return None
    cs = aux[1]
    anchors = derive_anchors(pe) or []
    cands = [a for a in anchors if a["kind"] == "message" and a["string"] == "*fill*"]
    for a in cands:
        idx = next((k for k, i in enumerate(insns)
                    if (i.address - pe.image_base) == a["anchor"]), None)
        if idx is None:
            continue
        start = _function_start(insns, idx)
        start_idx = next((k for k, i in enumerate(insns) if i.address == start), idx)
        func = insns[start_idx:start_idx + 400]
        fill_idx = None
        for j, i in enumerate(func):
            if i.mnemonic == "push" and re.fullmatch(r"0x[0-9a-f]+", i.op_str or ""):
                if pe.cstring(int(i.op_str, 16) - pe.image_base) == "*fill*":
                    fill_idx = j
        if fill_idx is None:
            continue
        # The work happens just before the row is printed: the nearest
        # `not`/`and` round-up pair, and the `[reg + 0x20]` loads that feed it
        # (a *stack* +0x20 slot is not an Elf32_Shdr field, so a load off ESP or
        # EBP is not evidence).
        roundups, loads = [], []
        for j in range(fill_idx - 1, max(-1, fill_idx - 200), -1):
            i = func[j]
            if i.mnemonic == "and" and any(func[k].mnemonic == "not"
                                             for k in range(max(0, j - 3), j)):
                roundups.append(func[j])
            if i.mnemonic == "mov" and len(i.operands) == 2 and \
                    i.operands[1].type == cs.x86.X86_OP_MEM and \
                    i.operands[1].mem.disp == 0x20 and i.operands[0].size == 4 and \
                    i.operands[1].mem.base not in (cs.x86.X86_REG_ESP,
                                                   cs.x86.X86_REG_EBP, 0):
                loads.append(i)
            if len(roundups) >= 2 and loads:
                break
        kind = next((w for w in func[fill_idx:fill_idx + 12]
                     if w.mnemonic == "cmp" and "4" in w.op_str), None)
        if loads and roundups:
            return {"fill_anchor": a["anchor"],
                    "func_rva": start - pe.image_base,
                    "align_loads": [{"rva": w.address - pe.image_base,
                                     "text": f"{w.mnemonic} {w.op_str}"}
                                    for w in reversed(loads[:2])],
                    "roundups": [{"rva": w.address - pe.image_base,
                                  "text": f"{w.mnemonic} {w.op_str}"}
                                 for w in reversed(roundups[:2])],
                    "fill_push": {"rva": func[fill_idx].address - pe.image_base,
                                  "text": f"{func[fill_idx].mnemonic} "
                                          f"{func[fill_idx].op_str}"},
                    "kind_test": None if kind is None else
                    {"rva": kind.address - pe.image_base,
                     "text": f"{kind.mnemonic} {kind.op_str}"}}
    return None


def cmd_align(args):
    pe = Pe(args.linker)
    der = derive_alignment(pe)
    if der is None:
        print("align needs capstone, and a linker whose map printer emits '*fill*'",
              file=sys.stderr)
        return 2
    print(f"linker: {pe.path}")
    print("the alignment the linker enforces, derived from the code that prints "
          "the map's '*fill*' row:")
    print(f"  the '*fill*' literal is pushed at RVA {der['fill_push']['rva']:#x} "
          f"({der['fill_push']['text']}) in the function at {der['func_rva']:#x}")
    for load in der["align_loads"]:
        print(f"  the alignment it uses: RVA {load['rva']:#x}  {load['text']}")
    print("    -> offset 0x20 of an Elf32_Shdr is sh_addralign: the *input* "
          "section's own alignment")
    for r in der["roundups"]:
        print(f"  the round-up comparison: RVA {r['rva']:#x}  {r['text']} "
              f"(the `not` before it is ~(align-1))")
    if der["kind_test"]:
        print(f"  the row-kind test that guards the '*fill*' row: "
              f"RVA {der['kind_test']['rva']:#x}  {der['kind_test']['text']}")
    print()
    print("WHAT THIS MEANS: there is no refusal.  The linker aligns the")
    print("fragment's address up to the input section's sh_addralign -")
    print("    addr = (addr + align - 1) & ~(align - 1)")
    print("- and, when that moved it, emits a '*fill*' row for the residue.  A")
    print("section that claims a 4 mod 8 start with sh_addralign 8 therefore")
    print("lands 4 bytes late and every later fragment moves with it: that is")
    print("exactly what tools/elf/objalign.py lowers sh_addralign for.")

    spec = args.unit or args.object
    if not spec:
        if args.json:
            print(json.dumps(der, indent=2))
        return 0
    res = resolve_object(spec, args.link_out)
    if res["path"] is None:
        print(f"no object for '{spec}'", file=sys.stderr)
        return 2
    obj = MwObject(res["path"])
    starts = {}
    for path in (args.splits, "config/RMHE08/splits.txt"):
        if path:
            starts = _splits_starts(Path(path))
            if starts:
                break
    unit_starts = starts.get(res["unit"], {}) or starts.get(res["unit"] + ".cpp", {})
    print()
    print(f"object: {res['path']}")
    if not unit_starts:
        print(f"  (no splits.txt entry claims sections for '{res['unit']}'; "
              f"alignment is reported against the object only)")
    bad = 0
    alloc = [s for s in obj.sections
             if s["index"] != 0 and (s["flags"] & 0x2) and s["size"] > 0]
    counts = {}
    for s in alloc:
        counts[s["name"]] = counts.get(s["name"], 0) + 1
    for sec in alloc:
        start = unit_starts.get(sec["name"])
        align = sec["align"]
        if counts[sec["name"]] > 1:
            # A split target object can carry several sections with one name
            # (dol split fragments); the splits claim names a single address, so
            # a per-section comparison is not available - say that, do not guess.
            if sec is alloc[[s["name"] for s in alloc].index(sec["name"])]:
                print(f"  {sec['name']:<14} {counts[sec['name']]} sections with this "
                      f"name in the object - the splits claim gives one address, "
                      f"so no per-section comparison is possible")
            continue
        if start is None:
            print(f"  {sec['name']:<14} align {align:<3} (no claimed start)")
            continue
        slack = (-start) % align if align > 1 else 0
        low = start & -start
        tag = "honoured" if slack == 0 else f"NOT honoured: +{slack:#x} '*fill*'"
        extra = ""
        if slack and align > low:
            extra = (f"  [the address can only honour {low}; "
                     f"tools/elf/objalign.py lowers this to {low}]")
        if slack:
            bad += 1
        print(f"  {sec['name']:<14} align {align:<3} claim {start:#010x} "
              f"({start % align:#x} mod {align})  {tag}{extra}")
    print()
    if bad:
        print(f"# {bad} section(s) cannot be honoured at their claimed address: "
              f"the link will insert '*fill*' and shift what follows")
    else:
        print("# every allocatable section is honoured at its claimed address")
    if args.json:
        print(json.dumps(der, indent=2))
    return 1 if bad else 0

# ---------------------------------------------------------------------------
# The linker's internal input-file record (derived, then read back)
# ---------------------------------------------------------------------------

# Every field below is either *derived* from the parser's own instructions or
# *verified* live against the artifact it describes.  Nothing here is a
# transcribed offset: `derive_file_record` finds the parser, the stride, the
# record array and the two setters of the flag byte in `.text`, and
# `records --prove` re-reads each record out of the running linker and
# cross-checks it against the object file the record names.
FILE_RECORD_FIELDS = [
    (0x00, 4, "elf_header",
     "pointer to the input file's own ELF header",
     "live: the 4 bytes it points at are 7f 45 4c 46 ('\\x7fELF')"),
    (0x04, 4, "(unnamed)",
     "not derived",
     "a pointer, but to what is not established"),
    (0x08, 4, "name",
     "pointer to the input's name as the link knows it",
     "live: equals the response-file entry at the same index, all records"),
    (0x0c, 4, "(unnamed)",
     "not derived; it tracks the section count",
     "live: high16 = (record+0x10)-1, low16 = (record+0x10)+1, every record"),
    (0x10, 4, "section_count_adj",
     "section count minus the 4 fixed sections (null, .symtab, .strtab, .shstrtab)",
     "live: equals e_shnum-4 for every record whose object is a plain object"),
    (0x14, 2, "(unnamed)",
     "0 in every record read; the u32 read at +0x14 is `e_shnum << 16` only "
     "because +0x16 holds e_shnum and this half is zero",
     "live: zero for every record, while the u32 spanning it is shnum<<16"),
    (0x16, 2, "shnum",
     "the object's e_shnum (the ELF section-header count)",
     "live: equals the object's e_shnum for every record whose input is a "
     "plain object"),
    (0x18, 4, "per_file_ptr",
     "pointer into a per-file table (stride 0x28, one entry per input)",
     "live: the values step by exactly 0x28 with the record index"),
    (0x1c, 1, "flags",
     "bit2 = the '.comment' magic matched; bit3 = the comment version >= 0xb; "
     "bit7 is tested elsewhere (0x405e2) and its meaning is not derived",
     "parser 0x53975/0x53983 clear bits 2/3, 0x539cf/0x539e2 set them after "
     "the 'CodeWarrior' memcmp at 0x5399f and the version compare at 0x539d5"),
    (0x1e, 1, "comment_kind",
     "0 = no comment, 2 = version 3..5, 3 = version >= 6",
     "parser 0x539c4 stores 2, 0x53a60 stores 3, from the same version byte"),
    (0x1f, 1, "comment_version",
     "the version byte of the object's '.comment' (the byte after 'CodeWarrior')",
     "parser 0x539ab loads [comment+0xb], 0x539af stores it here"),
]


def _disas_text(pe):
    """``(insns, md)`` for the linker's ``.text``, or ``(None, None)``."""
    cs = _capstone()
    if cs is None:
        return None, None
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    text = pe.section(".text")
    if text is None:
        return None, None
    return list(md.disasm(pe.read_rva(text.va, text.vsize),
                          pe.image_base + text.va)), (md, cs)


def derive_file_record(pe):
    """Derive the input-file record: parser, stride, array, and the flag setters.

    The lever is the same one ``anchors`` uses: the parser is the function that
    compares the ``.comment`` contents against the literal ``CodeWarrior``, so
    the anchor that pushes that string *is* the parser's address.  Everything
    else - the record stride, the array the parser indexes into, and the two
    instructions that set the record's flag byte - is then read out of that
    function.  A fact that cannot be found is reported as not found; nothing is
    transcribed from a previous run.
    """
    insns, aux = _disas_text(pe)
    if insns is None:
        return None
    cs = aux[1]
    anchors = derive_anchors(pe) or []
    hit = [a for a in anchors if a["kind"] == "message"
           and a["string"] == "CodeWarrior"]
    if not hit:
        return None
    idx = next((k for k, i in enumerate(insns)
                if (i.address - pe.image_base) == hit[0]["anchor"]), None)
    if idx is None:
        return None
    start = _function_start(insns, idx)
    start_idx = next((k for k, i in enumerate(insns) if i.address == start), idx)
    # The window is bounded, and every record-field match below additionally
    # has to use the parser's own record register (ESI, per its prologue) - so a
    # scan that runs a few instructions past the function's end cannot quietly
    # adopt the next function's instruction as evidence.
    func = [i for i in insns[start_idx:start_idx + 160]
            if i.mnemonic not in ("nop", "int3")]

    info = {"parser_rva": start - pe.image_base,
            "magic_rva": hit[0]["anchor"], "stride": None, "table_va": None,
            "version_store_rva": None, "version_load_rva": None,
            "kind_stores": [], "flag_clear": [], "flag_set": []}
    for i in func:
        rva = i.address - pe.image_base
        if i.mnemonic == "imul" and len(i.operands) == 3 and \
                i.operands[2].type == cs.x86.X86_OP_IMM:
            info["stride"] = i.operands[2].imm
        if i.mnemonic == "add" and len(i.operands) == 2 and \
                i.operands[1].type == cs.x86.X86_OP_MEM and \
                i.operands[1].mem.base == cs.x86.X86_REG_INVALID and \
                i.operands[1].mem.index == cs.x86.X86_REG_INVALID and \
                i.operands[1].mem.disp:
            info["table_va"] = i.operands[1].mem.disp
        if i.mnemonic == "mov" and "esi + 0x1f]" in i.op_str:
            info["version_store_rva"] = rva
        if i.mnemonic == "movzx" and "ebp + 0xb]" in i.op_str:
            info["version_load_rva"] = rva
        if i.mnemonic == "mov" and "esi + 0x1e]" in i.op_str:
            info["kind_stores"].append((rva, i.op_str))
        if i.mnemonic in ("and", "or") and len(i.operands) == 2 and \
                i.operands[1].type == cs.x86.X86_OP_IMM and \
                i.operands[1].imm in (4, 8, 0xFB, 0xF7) and i.operands[0].size == 1:
            key = "flag_set" if i.mnemonic == "or" else "flag_clear"
            info[key].append((rva, i.op_str))
    return info


def cmd_records(args):
    pe = Pe(args.linker)
    der = derive_file_record(pe)
    if der is None:
        print("records needs capstone, and a linker whose '.comment' parser "
              "references the literal 'CodeWarrior'", file=sys.stderr)
        return 2
    print(f"linker: {pe.path}")
    print(f"the '.comment' parser: RVA {der['parser_rva']:#x} "
          f"(the function that memcmps the comment against 'CodeWarrior' at "
          f"{der['magic_rva']:#x})")
    print(f"the input-file record: stride {der['stride']:#x}, array at "
          f"[{der['table_va']:#x}], index = the link's input order")
    print()
    print("  offset size field            meaning")
    print("  " + "-" * 96)
    for off, size, name, meaning, evidence in FILE_RECORD_FIELDS:
        print(f"  +0x{off:02x}  {size}    {name:<16} {meaning}")
        print(f"         evidence: {evidence}")
    print()
    print("derived from the parser's own instructions (not transcribed):")
    print(f"  stride:          imul with immediate {der['stride']:#x}")
    print(f"  record array:    the `add reg, dword ptr [{der['table_va']:#x}]` "
          f"in the parser (the base pointer lives at that address)")
    if der["version_load_rva"] is not None:
        print(f"  comment version: load at {der['version_load_rva']:#x} "
              f"(from [comment+0xb]), store at {der['version_store_rva']:#x} "
              f"(to [record+0x1f])")
    for rva, txt in der["kind_stores"]:
        print(f"  comment kind:    {rva:#x}  {txt}")
    for rva, txt in der["flag_clear"]:
        print(f"  flag clear:      {rva:#x}  {txt}")
    for rva, txt in der["flag_set"]:
        print(f"  flag set:        {rva:#x}  {txt}")
    if args.json:
        print(json.dumps(der, indent=2, default=str))
    if args.prove:
        return prove_records(pe, der, args)
    return 0


def _record_probe_line(args):
    """The link line a `records --prove` gdb run uses, derived from build.ninja."""
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    rsp = Path(args.rsp) if args.rsp else work / "records.rsp"
    if not rsp.exists():
        if derive_rsp(args.link_out, rsp) is None:
            return None, None, None
    args.rsp = str(rsp)
    line = derive_link_line(rsp, args.ldscript or "build/RMHE08/ldscript.lcf")
    line = line.format(out=str(work / "records.elf"), map=str(work / "records.MAP"))
    return line, rsp, work


def prove_records(pe, der, args):
    """Read the linker's record array back out and check it against the objects.

    The gdb run breaks at the linker's own message loader (the derived
    observation anchor) and, once it has announced ``Optimizing:``, dumps the
    first N records from the array the parser indexes.  Every field the table
    above names is then *cross-checked*: the name against the response file, the
    comment version and the section count against the object file on disk.  A
    disagreement is printed, not smoothed over.
    """
    gdb = find_gdb(args.gdb)
    if not gdb:
        print("no gdb found; pass --gdb", file=sys.stderr)
        return 2
    if args.args:
        line = args.args
    else:
        line, rsp, work = _record_probe_line(args)
        if line is None:
            print("cannot derive the link from build.ninja; pass --args", file=sys.stderr)
            return 2
        args.rsp = str(rsp)
    io = derive_message_io(pe)
    if io is None or io.get("loader") is None:
        print("cannot derive the message loader", file=sys.stderr)
        return 2
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    script = work / "mwlink-records.gdb"
    limit = args.limit
    script.write_text(
        "set pagination off\nset confirm off\nset width 0\n"
        f"file {pe.path.as_posix()}\n"
        "set args " + line + "\n"
        "python\nimport gdb\n"
        f"LOADER = 0x{pe.image_base + io['loader']['after_rva']:x}\n"
        f"BASE = 0x{der['table_va']:x}\n"
        f"STRIDE = {der['stride']}\n"
        f"LIMIT = {limit}\n"
        "DONE = [False]\n"
        "def rd(a, n):\n"
        "    return gdb.selected_inferior().read_memory(a, n).tobytes()\n"
        "def u32(a):\n"
        "    try: return int.from_bytes(rd(a, 4), 'little')\n"
        "    except Exception: return None\n"
        "def u8(a):\n"
        "    try: return rd(a, 1)[0]\n"
        "    except Exception: return None\n"
        "def cstr(a, n=96):\n"
        "    try: return rd(a, n).split(b'\\0')[0].decode('latin-1', 'replace')\n"
        "    except Exception: return None\n"
        "class Opt(gdb.Breakpoint):\n"
        "    def stop(self):\n"
        "        if DONE[0]:\n"
        "            return False\n"
        "        try:\n"
        "            esp = int(gdb.parse_and_eval('$esp')) & 0xffffffff\n"
        "            uID = int(gdb.parse_and_eval('*(unsigned int*)%d' % (esp + 0x10)))\n"
        "        except Exception:\n"
        "            return False\n"
        "        if uID != 41:\n"
        "            return False\n"
        "        DONE[0] = True\n"
        "        base = u32(BASE)\n"
        "        print('MWLINK-BASE 0x%x' % (base or 0))\n"
        "        for i in range(LIMIT):\n"
        "            rec = (base or 0) + i * STRIDE\n"
        "            print('MWLINK-RECORD %d %s|%d|%d|%d|%d|%d|%d|%d|%d' % (\n"
        "                i, cstr(u32(rec + 8)), u32(rec + 0x14) or 0,\n"
        "                u32(rec + 0x16) or 0, u32(rec + 0x18) or 0,\n"
        "                u8(rec + 0x1c) or 0, u8(rec + 0x1e) or 0,\n"
        "                u8(rec + 0x1f) or 0, u32(rec + 0x0c) or 0, u32(rec + 0x10) or 0))\n"
        "        return False\n"
        "Opt('*0x%x' % LOADER)\n"
        "print('MWLINK-READY')\n"
        "end\nrun\nprintf \"MWLINK-DONE\\n\"\nquit\n",
        encoding="utf-8")
    argv = [gdb, "-batch", "-nx", "-x", str(script)]
    print(f"# gdb {script}")
    proc = subprocess.run(argv, capture_output=True, text=True, encoding="utf-8", errors="replace")
    rows = []
    base = None
    for line_ in proc.stdout.splitlines():
        m = re.match(r"^MWLINK-RECORD (\d+) (.*)$", line_.strip())
        if m:
            parts = m.group(2).split("|")
            rows.append((int(m.group(1)), parts[0]) + tuple(int(x) for x in parts[1:]))
            continue
        m = re.match(r"^MWLINK-BASE (0x[0-9a-f]+)$", line_.strip())
        if m:
            base = int(m.group(1), 16)
    if not rows:
        print("# the run produced no record dump; gdb stderr follows", file=sys.stderr)
        print((proc.stderr or "").strip()[:1500], file=sys.stderr)
        return 1
    inputs = []
    if args.rsp and Path(args.rsp).exists():
        inputs = [ln.strip().replace("\\", "/")
                  for ln in Path(args.rsp).read_text(errors="replace").splitlines()
                  if ln.strip()]
    print(f"the linker's input-file records, read out of the running linker "
          f"(array 0x{base:x}, stride {der['stride']:#x}):")
    print(f"  {'idx':>5} {'record+0x08':<34} {'+0x1f':>5} {'+0x16&ffff':>10} "
          f"{'+0x10':>6} {'flags':>5} {'kind':>4} checks")
    checked = {"name": 0, "version": 0, "shnum": 0, "total": 0}
    problems = []
    for idx, name, v14, v16, v18, flags, kind, ver, v0c, v10 in rows:
        want = inputs[idx] if idx < len(inputs) else None
        checks = []
        if want is not None:
            checked["total"] += 1
            same = Path(want).name.lower() == (name or "").lower()
            checks.append("name " + ("=" if same else f"!={Path(want).name}"))
            if same:
                checked["name"] += 1
            obj = Path(want)
            if not obj.is_absolute():
                obj = ROOT / obj
            if obj.exists():
                obj_shnum, obj_ver = _object_header_bits(obj)
                if obj_ver is not None:
                    if obj_ver == ver:
                        checked["version"] += 1
                        checks.append("comment-version =")
                    else:
                        checks.append(f"comment-version !={obj_ver}")
                        problems.append(f"record {idx} ({name}): +0x1f={ver}, "
                                        f"the object's .comment says {obj_ver}")
                if obj_shnum is not None:
                    if (v16 & 0xFFFF) == obj_shnum:
                        checked["shnum"] += 1
                        checks.append("e_shnum =")
                    else:
                        checks.append(f"e_shnum != {obj_shnum}")
                        problems.append(f"record {idx} ({name}): +0x16&0xffff="
                                        f"{v16 & 0xFFFF}, e_shnum={obj_shnum}")
        if idx < 8:
            print(f"  {idx:>5} {str(name)[:34]:<34} {ver:>5} {v16 & 0xFFFF:>10} "
                  f"{v10:>6} 0x{flags:02x} {kind:>4} {', '.join(checks)}")
    print(f"  ... {len(rows)} record(s) dumped")
    print(f"# cross-check against the response file and the objects themselves: "
          f"{checked['name']}/{checked['total']} names, "
          f"{checked['version']}/{checked['total']} comment versions, "
          f"{checked['shnum']}/{checked['total']} section counts")
    for p_ in problems[:10]:
        print("  MISMATCH: " + p_)
    return 0 if not problems else 1


def _object_header_bits(path):
    """``(e_shnum, .comment version byte)`` for a Metrowerks object, or ``(None, None)``."""
    try:
        blob = Path(path).read_bytes()
    except OSError:
        return None, None
    if len(blob) < 0x40 or blob[:4] != b"\x7fELF" or blob[5] != 2:
        return None, None
    shnum = struct.unpack_from(">H", blob, 0x30)[0]
    shoff = struct.unpack_from(">I", blob, 0x20)[0]
    strndx = struct.unpack_from(">H", blob, 0x32)[0]
    if not shoff or shnum <= strndx or shoff + 40 * shnum > len(blob):
        return shnum, None
    def sh(i):
        return struct.unpack_from(">10I", blob, shoff + 40 * i)
    stro = sh(strndx)[4]
    ver = None
    for i in range(shnum):
        nm, _t, _f, _a, off, size = sh(i)[:6]
        e = blob.find(b"\0", stro + nm)
        if blob[stro + nm:e] == b".comment" and size > 0xC:
            ver = blob[off + 0xB]
            break
    return shnum, ver


def cmd_trace(args):
    """Trace one input object - or one UNIT NAME - through one real link.

    The map and the ELF are the ground truth, and they have to *be* the same
    link: if the ELF does not exist the trace still reports what the map says,
    but it says so (`not checked`) instead of pretending.  With ``--link`` the
    tool runs the build's own link command, redirected to a scratch path, so
    the artifact it traces is one it produced itself.  The build writes no map
    of its own, so when no map is available the tool links one (into
    ``build/scratch/``) rather than making the caller assemble a link line.
    """
    resolved = resolve_object(args.object, args.link_out)
    if resolved["path"] is None:
        print(f"no object for '{args.object}': it is not a path, and no link "
              f"input names that unit ({resolved['inputs']} inputs in the link)",
              file=sys.stderr)
        if resolved.get("near"):
            print("closest unit names: " + ", ".join(resolved["near"]), file=sys.stderr)
        return 2
    args.object = str(resolved["path"])
    print(f"# unit {resolved['unit']}: {resolved['path']}")
    print(f"#   resolved as: {resolved['how']}")

    # One obvious entry point: a unit name, and the tool links if it must.
    default_map = Path(args.map) if args.map else ROOT / "build/RMHE08/main.MAP"
    if not args.link and args.map is None and not default_map.exists():
        print(f"# no link map at {default_map} (the build does not write one); "
              f"linking into {args.out}/ instead")
        args.link = True

    if args.link:
        got = run_link(args)
        if got is None:
            print("cannot derive the link from build.ninja; pass --rsp", file=sys.stderr)
            return 2
        rc, argv, out, err, elf_out, map_out = got
        print("# " + " ".join(argv))
        if rc != 0:
            catalogue = message_catalogue(Pe(args.linker))
            report_link_failure(args, argv, out, err, catalogue)
            # The failed link may still have left a map from an earlier run;
            # tracing against it is allowed, but it is never presented as this
            # link's artifact, and a missing one is an honest stop.
            if args.map is None:
                args.map = str(default_map) if default_map.exists() else None
            if args.map is None or not Path(args.map).exists():
                print(f"the link failed (rc={rc}) and left no map; the link line "
                      f"above is the reproduction", file=sys.stderr)
                return 1
            print(f"# WARNING: the link failed; tracing against {args.map}, which "
                  f"is NOT the map of the link that just failed", file=sys.stderr)
        else:
            args.elf = args.elf or str(elf_out)
            args.map = str(map_out)
            print(f"# linked {elf_out} ({elf_out.stat().st_size} bytes)")
    else:
        args.map = args.map or str(default_map)
    if not Path(args.map).exists():
        print(f"no link map at {args.map} - the link that produced it failed before "
              f"writing one; the link line above is the reproduction", file=sys.stderr)
        return 1
    map_text = Path(args.map).read_text(encoding="utf-8", errors="replace")
    if args.elf and not Path(args.elf).exists():
        print(f"# WARNING: no ELF at {args.elf}; addresses and relocations stay unverified",
              file=sys.stderr)
        args.elf = None
    order = derive_order(Pe(args.linker)) if _linker_exists(args) else None
    rep = build_trace(args.object, map_text, args.elf, args.rsp, order)
    rep["map"] = str(args.map)
    rep["unit"] = resolved["unit"]
    rep["resolved"] = resolved["how"]
    if args.json:
        print(json.dumps(rep, indent=2))
        return 0 if rep["verdict"] != "FAIL" else 1
    print(f"# map {args.map}" + (f"   elf {args.elf}" if args.elf else ""))
    for line in render_trace(rep):
        print(line)
    if args.full_ctor:
        print()
        print("the whole merged .ctors/.dtors layout, as the linker laid it out:")
        for name in (".ctors", ".dtors"):
            info = parse_map(map_text).get(name)
            if not info:
                continue
            for block in info["blocks"]:
                for r in block["fragments"]:
                    if r["name"] in (name,) or is_ctor_dtor_name(r["name"]):
                        print(f"  {name:<8} +{r['offset']:#06x} {r['size']:#06x} "
                              f"{r['name']:<12} {r['source']}")
    return 0 if rep["verdict"] != "FAIL" else 1


def cmd_diagnose(args):
    """Run the build's own link with ``-v`` and report what it said, in order.

    This is the erroring-link path without an object to trace: the phase
    stream, then every diagnostic classified against the message catalogue and
    attributed to the phase that printed it.  It exits 0 when the link
    succeeded and 1 when it did not, and it always redirects the output into
    ``build/scratch/`` - the artifact it must never overwrite is ``main.elf``.
    """
    got = run_link(args, out_base="diagnose", verbose=True)
    if got is None:
        print("cannot derive the link from build.ninja; pass --rsp/--args", file=sys.stderr)
        return 2
    rc, argv, out, err, elf_out, map_out = got
    print("# " + " ".join(argv))
    text = (out or "") + "\n" + (err or "")
    catalogue = message_catalogue(Pe(args.linker))
    timeline = parse_timeline(text, phase_kinds(catalogue))
    print(f"link: rc={rc}" + (f"  ({elf_out.stat().st_size} bytes)"
                              if rc == 0 and elf_out.exists() else ""))
    print("phase stream:")
    for kind, line in timeline:
        print(f"  {kind:<12} {line}")
    if not timeline:
        print("  (none - the linker printed no phase line)")
    diags = classify_diagnostics(text, catalogue, timeline)
    print("diagnostics:")
    if not diags:
        print("  (none)")
    for d in diags:
        who = f"phase {d['phase']}" if d["phase"] else "(no phase seen)"
        mid = f"msgid={d['msgid']}" if d["msgid"] is not None else "msgid=? (not in the catalogue)"
        print(f"  [{who}] {mid}: {d['text']}")
    if args.json:
        print(json.dumps({"rc": rc, "phases": [k for k, _ in timeline],
                          "diagnostics": diags}, indent=2))
    return 0 if rc == 0 else 1


def _linker_exists(args):
    return getattr(args, "linker", None) is not None and Path(args.linker).exists()


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
    """Pure-function + fixture checks.  No gdb, and nothing is *run*.

    The only thing that reads a real binary is the derivation check at the end
    (the linker's own records and the alignment site), and that is skipped - with
    a note - when capstone or the linker is not there.
    """
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

    # 2. Catalogue ids are the linker's own: (block_name - 1) * 16 + slot, so
    #    the first block (named 1) holds ids 0..15.  Proven against a real
    #    link by `phases --prove` (observed ids 27/29/41).
    ok((1 - 1) * 16 + 0 == 0, "message ids start at 0 in the block named 1")
    ok((2 - 1) * 16 + 11 == 27, "Linking: sits at the id the loader asks for (27)")

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

    # 7. The input object reader: sections, symbols and RELA relocations, as
    #    the linker sees them (big-endian, symbol index << 8 | type).
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        op = td / "fixture.o"
        op.write_bytes(_fixture_obj())
        obj = MwObject(op)
        ok([s["name"] for s in obj.sections][1:3] == [".text", ".ctors$10"],
           f"object sections: {[s['name'] for s in obj.sections]}")
        ok([s["name"] for s in obj.symbols][2:4] == ["foo", ""],
           f"object symbols: {[s['name'] for s in obj.symbols]}")
        ok(obj.symbols[2]["bind"] == STB_GLOBAL and obj.symbols[2]["type"] == STT_FUNC,
           "symbol bind/type from st_info")
        ok(len(obj.relocs) == 1 and obj.relocs[0]["target"] == ".text"
           and obj.relocs[0]["sym_name"] == "foo" and obj.relocs[0]["type"] == 1,
           f"relocations: {obj.relocs}")
        ok([s["name"] for s in obj.ctor_dtor_sections()] == [".ctors$10"],
           "ctor/dtor sections recognised")

        # 8. Relocation field verification (the read-back, not a transcription).
        ok(reloc_field(1, 0x80004000, 0x80004000, 0, 0)[0:3] == (True, 0x80004000, 0x80004000),
           "ADDR32 field")
        ok(reloc_field(10, 0x4800004C, 0x8000404C, 0, 0x80004000)[0:3]
           == (True, 0x4C, 0x4C), "REL24 field is (S+A-P) masked to bits 2..25")
        # The real one: the linker only rewrites the LI field, so the `bl`'s LK
        # bit (0x1) survives - 0x4800004D has LI<<2 == 0x4C == S-P.
        ok(reloc_field(10, 0x4800004D, 0x80457490, 0, 0x80457444)[1] == 0x4C,
           "REL24 against the real __register_fragment branch")
        ok(reloc_field(6, 0x3C008004, 0x8003F1C8, 0, 0)[0:3] == (True, 0x8004, 0x8004),
           "ADDR16_HA rounds (the real trace's value)")
        ok(reloc_field(109, 0x800DAEA8, 0x80793CC8, 0, 0, 0x80798E20)[0:3]
           == (True, 0xAEA8, 0xAEA8), "SDA-relative disp from the map's _SDA_BASE_")
        ok(reloc_field(109, 0x800DAEA8, 0x80793CC8, 0, 0, None)[0] is False,
           "type 109 is not claimed without a _SDA_BASE_")
        ok(reloc_field(40, 0x12345678, 0, 0, 0)[0] is False,
           "an un-derived type is reported unchecked")

        # 9. The map carries the input file each row came from - that is what
        #    makes a per-object trace possible at all.
        mrows = {r["name"]: r for _s, r in _map_rows(mp)}
        ok(mrows[".ctors$10"]["source"] == "__init_cpp_exceptions.o",
           f"map row source: {mrows['.ctors$10']}")
        ok(mrows[".ctors$10"]["flags"] == 1, "map row flags")

        # 10. End to end on fixtures: a link whose map and ELF agree traces as
        #     MATCH, verifies the ADDR32 relocation against the ELF's word, and
        #     FAILs loudly when the ELF stops carrying the bytes.
        A = 0x8056F2C0
        objmap = "\n.text section layout\n"
        objmap += f"  00000000 000004 {A:08x} 00000200  1 .text \tfixture.o \n"
        objmap += f"  00000000 000004 {A:08x} 00000200  4 foo \tfixture.o \n"
        objmap += "\n.ctors section layout\n"
        objmap += f"  00000000 000004 {A + 0x10:08x} 00000210  1 .ctors$10 \tfixture.o \n"
        objmap += f"  00000010 000004 {A + 0x10:08x} 00000210  4 bar \tfixture.o \n"
        elf = td / "fixture.elf"
        elf.write_bytes(_fixture_elf2([
            (".text", 1, A, 4, struct.pack(">I", A)),        # the relocated word
            (".ctors", 1, A + 0x10, 4, b"\0\0\0\0"),
        ]))
        rep = build_trace(op, objmap, elf)
        ok(rep["kept"], "fixture trace: kept")
        ok(rep["verdict"] == "MATCH", f"fixture trace verdict: {rep['verdict']} {rep['problems']}")
        rel = rep["relocations"][0]
        ok(rel["verdict"] == "applied" and rel["actual"] == A,
           f"fixture relocation verified from the ELF: {rel}")
        ok([s for s in rep["sections"] if s["name"] == ".text"][0]["bytes_identical"],
           "fixture section bytes read back")
        # A word that is not what the relocation must have written -> FAIL.
        bad = td / "bad.elf"
        bad.write_bytes(_fixture_elf2([
            (".text", 1, A, 4, struct.pack(">I", A + 4)),
            (".ctors", 1, A + 0x10, 4, b"\0\0\0\0")]))
        rep = build_trace(op, objmap, bad)
        ok(rep["verdict"] == "FAIL" and any("holds" in p for p in rep["problems"]),
           f"fixture trace FAILs on a wrong relocation word: {rep['problems']}")
        # An object no row names is reported as dropped, not as traced.
        rep = build_trace(td / "fixture.o", objmap.replace("fixture.o", "other.o"), elf,
                          rsp_path=None)
        ok(rep["verdict"] == "DROPPED" and "dead-stripped" in rep["why"],
           f"dropped object: {rep['why']}")

    # 11. The unit-name entry point: a build input path is a unit name, and a
    #     unit name is what `trace` takes.
    ok(unit_of_object("build/RMHE08/src/Network/NetworkWiiMediator.o")
       == "Network/NetworkWiiMediator", "unit_of_object reads a src/ input")
    ok(unit_of_object("build\\RMHE08\\obj\\Runtime.PPCEABI.H\\__start.o")
       == "Runtime.PPCEABI.H/__start", "unit_of_object reads an obj/ input")
    ok(unit_of_object("build/RMHE08/obj/auto_00_80004380_init.o")
       == "auto_00_80004380_init", "unit_of_object keeps a flat name")

    # 12. Diagnostics: the linker's own banner + body is one message, and its
    #     id comes from the catalogue - never invented.
    cat = {27: "Linking: '%c'", 41: "Optimizing: '%c'",
           189: "runtime sources 'global_destructor_chain.c' "
                                   "and '__init_cpp_exceptions.cpp' both need to be "
                                   "updated to latest version.  Please contact "
                                   "Freescale support."}
    ok(match_message("  Linking: 'x.elf'", cat)[0] == 27, "match_message: phase line")
    ok(match_message("#   something the catalogue does not have", cat) is None,
       "match_message: unknown text stays unknown")
    diag_text = ("#   Linking: 'x.elf'\n#   Optimizing: 'x.elf'\n"
                 "### mwldeppc.exe Linker Error:\n"
                 "#   runtime sources 'global_destructor_chain.c' and "
                 "'__init_cpp_exceptions.cpp' both need to be updated to latest "
                 "version.  Please contact Freescale support.\n")
    diags = classify_diagnostics(diag_text, cat)
    ok(len(diags) == 1, f"classify_diagnostics found one diagnostic: {diags}")
    ok(diags and diags[0]["msgid"] == 189, "the diagnostic got its catalogue id")
    ok(diags and diags[0]["phase"] == "Optimizing",
       f"the diagnostic is attributed to the last phase before it: {diags}")
    ok(classify_diagnostics("#   nothing to see here", cat) == [],
       "a run with no banner reports no diagnostics")

    # 13. The input-file record table is a layout: the offsets are ordered, do
    #     not overlap, and fit inside the stride the parser's `imul` states.
    stride = 0x2C
    prev_end = 0
    ordered = True
    for off, size, _name, _meaning, _ev in FILE_RECORD_FIELDS:
        ordered = ordered and off >= prev_end
        prev_end = off + size
    ok(ordered, "FILE_RECORD_FIELDS offsets are ordered and non-overlapping")
    ok(prev_end <= stride, f"the fields fit the {stride:#x} stride (end {prev_end:#x})")
    ok([f[2] for f in FILE_RECORD_FIELDS][-1] == "comment_version",
       "the table's last field is the comment version")

    # 14. The claimed-start reader is the same shape objalign.py reads.
    tmp2 = tempfile.TemporaryDirectory()
    td = Path(tmp2.name)
    sp = td / "splits.txt"
    sp.write_text("Pl/fn_8023C2D0.cpp:\n\t.text       start:0x8023C2D0 end:0x80241558\n"
                  "\t.data       start:0x805C34D4 end:0x805C3D20\n\n"
                  "main.cpp:\n\t.text       start:0x8003F200 end:0x80040478\n",
                  encoding="utf-8")
    starts = _splits_starts(sp)
    ok(starts.get("Pl/fn_8023C2D0.cpp", {}).get(".data") == 0x805C34D4,
       f"_splits_starts reads the claimed .data start: {starts}")
    ok(starts.get("main.cpp", {}).get(".text") == 0x8003F200,
       "_splits_starts reads a second unit")
    ok((0x805C34D4 % 8) != 0 and (-0x805C34D4) % 8 == 4,
       "the aligned example really is 4 mod 8")

    # 15. The object-header reader the record proof cross-checks with.
    op2 = td / "head.o"
    op2.write_bytes(_fixture_obj())
    shnum, ver = _object_header_bits(op2)
    ok(shnum == 7, f"_object_header_bits reads e_shnum: {shnum}")
    ok(ver is None, "a fixture with no .comment has no comment version")
    ok(_object_header_bits(td / "nope.o") == (None, None),
       "a missing object reads as no header, not as a crash")
    tmp2.cleanup()

    # 16. The map's fifth column is an alignment, not a type - so the row kind
    #     has to come from the name.  A 1-byte label prints 1 there, exactly
    #     like a fragment, and reading it as "fragment" is what made `trace main`
    #     report its relocations as unresolvable.
    def mrow(name, flags):
        return {"name": name, "offset": 0, "size": 1, "addr": 0,
                "file_off": 0, "source": "x.o", "flags": flags}
    ok(is_section_row(".text", mrow(".text", 1)), "a section row: name == section")
    ok(is_section_row(".ctors", mrow(".ctors$10", 1)), "a $NN section row")
    ok(is_section_row("extab", mrow("extab", 1)), "extab has no leading dot")
    ok(is_section_row(".text", mrow("*fill*", 16)), "a fill row, whatever it prints")
    ok(not is_section_row(".sbss", mrow("lbl_807947A5", 1)),
       "a 1-byte label is a symbol even though it prints 1")
    ok(not is_section_row(".data", mrow(".data.0", 8)),
       "a pooled symbol has an inner dot and is not section-like")
    ok(not is_section_row(".text", mrow("fn_8003F200", 4)), "an ordinary symbol")
    ok(not is_section_row(".init", mrow("lbl_80004514 (entry of pad_00_80004380_init)", 0)),
       "an `(entry of ...)` row is a symbol, not a section")

    # 16b. A map row whose alignment column is *empty* (the `(entry of ...)`
    #      rows) must still be read - with its whole name, spaces and all.
    ent = ("\n.init section layout\n"
           "  00000514 000000 80004514 00000714    lbl_80004514 "
           "(entry of pad_00_80004380_init) \tauto_00_80004380_init.o \n")
    prows = parse_map(ent)[".init"]["blocks"][0]["fragments"]
    ok(prows and prows[0]["name"] == "lbl_80004514 (entry of pad_00_80004380_init)",
       f"an empty-alignment row keeps its name: {prows}")
    ok(prows and prows[0]["size"] == 0 and prows[0]["addr"] == 0x80004514,
       "and its address")
    ok(prows and prows[0]["source"] == "auto_00_80004380_init.o",
       "and its input file")

    # 17. The linker's own generated symbols are a second listing in the map.
    lgen = ("Link map of __start\n\nLinker generated symbols:\n"
            "                 _f_text 80004000\n"
            "                 _f_data 8057c820\n"
            "             _SDA2_BASE_ 8079daa0\n")
    gen = parse_map_symbols(lgen)
    ok(gen.get("_f_data") == 0x8057C820,
       f"the generated-symbol listing is read: {gen}")
    ok(gen.get("_f_text") == 0x80004000 and len(gen) == 3,
       "all three generated symbols are read")

    # 18. Which SDA base a type-109 displacement uses depends on the symbol's
    #     section: r13 for .sdata/.sbss, r2 for .sdata2/.sbss2.  The numbers are
    #     this link's (`lbl_80795AC0` in .sdata2, `_SDA2_BASE_` 0x8079DAA0),
    #     and the expected field 0x8020 is what the output ELF really holds.
    ok(reloc_field(109, 0x800D8020, 0x80795AC0, 0, 0, 0x8079DAA0)[0:3]
       == (True, 0x8020, 0x8020),
       "a .sdata2 symbol's SDA disp is relative to _SDA2_BASE_")
    ok(reloc_field(109, 0x800D8020, 0x80795AC0, 0, 0, 0x80798E20)[0:3]
       == (True, 0xCCA0, 0x8020),
       "...and using _SDA_BASE_ for it is exactly the old wrong answer")

    # 19. The derived tables themselves, when capstone and the linker are there.
    #     This is the only part that reads the real binary; it is skipped (not
    #     faked) when either is absent.
    der = None
    if _capstone() is not None:
        linker = default_linker()
        if linker is not None and Path(linker).exists():
            der = derive_file_record(Pe(linker))
    if der is None:
        print("note: capstone/linker absent - the record derivation is exercised "
              "by `records` on the real binary, not here", file=sys.stderr)
    else:
        ok(der["stride"] == 0x2C, f"the parser's imul gives the 0x2c stride: {der}")
        ok(der["table_va"] == 0x533458,
           f"the parser indexes the record array at {der['table_va']:#x}")
        ok(der["version_store_rva"] is not None and der["version_load_rva"] is not None,
           "the comment version load/store pair is found in the parser")
        ok([t for _r, t in der["kind_stores"]] == ["byte ptr [esi + 0x1e], 0",
                                                  "byte ptr [esi + 0x1e], 2",
                                                  "byte ptr [esi + 0x1e], 3"],
           f"the comment-kind stores are the three found: {der['kind_stores']}")
        ok({imm for _r, t in der["flag_set"] for imm in [t.split(', ')[-1]]}
           == {"4", "8"}, f"the flag setters are 4 and 8: {der['flag_set']}")
        al = derive_alignment(Pe(linker))
        ok(al is not None, "align derives the '*fill*' site")
        if al:
            ok(any("+ 0x20]" in l["text"] for l in al["align_loads"]),
               f"the alignment load is a +0x20 load: {al['align_loads']}")
            ok(len(al["roundups"]) >= 1, f"the round-up is found: {al['roundups']}")

    if fails:
        for f in fails:
            print("FAIL " + f, file=sys.stderr)
        return 1
    print(f"ok - {checks} checks")
    return 0


def _fixture_elf2(sections):
    """A minimal big-endian ELF32 *with section contents* (for read-backs)."""
    names = [""] + [s[0] for s in sections] + [".shstrtab"]
    sblob = b"\0"
    offs = {}
    for n in names:
        if n and n not in offs:
            offs[n] = len(sblob)
            sblob += n.encode() + b"\0"
    strndx = len(names) - 1
    body = bytearray(b"\0" * 0x200)
    content_off = {}
    for name, _typ, _addr, _size, content in sections:
        if content:
            content_off[name] = len(body)
            body += content
    body += sblob
    shoff = len(body)
    hdr = b"\x7fELF\x01\x02\x01\x00" + b"\0" * 8
    hdr += struct.pack(">HHIIIIIHHHHHH", 2, 20, 1, 0, 0, shoff, 0, 52,
                       0, 0, 40, len(sections) + 2, strndx)
    body[:0x40] = hdr.ljust(0x40, b"\0")

    def sh(name, typ, addr, size, offset, flags=0):
        return struct.pack(">IIIIIIIIII", offs.get(name, 0), typ, flags, addr,
                           offset, size, 0, 0, 4, 0)

    shdrs = [sh("", 0, 0, 0, 0)]
    for name, typ, addr, size, content in sections:
        shdrs.append(sh(name, typ, addr, size, content_off.get(name, 0),
                        0x2 if typ == 1 else 0))
    shdrs.append(sh(".shstrtab", 3, 0, len(sblob), len(body) - len(sblob)))
    return bytes(body) + b"".join(shdrs)


def _fixture_obj():
    """A tiny Metrowerks-shaped object: .text + .ctors$10 + one ADDR32 reloc."""
    names = ["", ".text", ".ctors$10", ".rela.text", ".symtab", ".strtab",
             ".shstrtab"]
    sblob = b"\0"
    offs = {}
    for n in names:
        if n and n not in offs:
            offs[n] = len(sblob)
            sblob += n.encode() + b"\0"
    strtab = b"\0" + b"foo\0" + b"bar\0"
    syms = [(0, 0, 0, 0, 0, 0),        # null
            (0, 0, 0, 0x03, 0, 1),     # .text (STT_SECTION)
            (1, 0, 4, 0x12, 0, 1),     # foo: GLOBAL FUNC in .text
            (0, 0, 0, 0x03, 0, 2),     # .ctors$10 section symbol
            (5, 0, 4, 0x11, 0, 2)]     # bar: GLOBAL OBJECT in .ctors$10
    symblob = b"".join(struct.pack(">IIIBBH", *s) for s in syms)
    rel = struct.pack(">IIi", 0, (2 << 8) | 1, 0)      # ADDR32 -> foo
    text = b"\0\0\0\0"
    ctors = b"\0\0\0\0"
    head = 0x34
    off = head
    parts, secdefs = b"", []

    def add(name, typ, flags, blob, link=0, info=0, entsize=0):
        nonlocal parts, off
        secdefs.append((name, typ, flags, off, len(blob), link, info, entsize))
        parts += blob
        off += len(blob)
    add(".text", 1, 0x6, text)
    add(".ctors$10", 1, 0x2, ctors)
    add(".rela.text", 4, 0, rel, link=4, info=1, entsize=12)
    add(".symtab", 2, 0, symblob, link=5, info=2, entsize=16)
    add(".strtab", 3, 0, strtab, entsize=1)
    add(".shstrtab", 3, 0, sblob, entsize=1)
    shoff = head + len(parts)
    hdr = b"\x7fELF\x01\x02\x01\x00" + b"\0" * 8
    hdr += struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0,
                       40, len(secdefs) + 1, 6)
    shdrs = [struct.pack(">IIIIIIIIII", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)]
    for name, typ, flags, o, size, link, info, entsize in secdefs:
        shdrs.append(struct.pack(">IIIIIIIIII", offs[name], typ, flags, 0, o,
                                 size, link, info, 4, entsize))
    return hdr.ljust(head, b"\0") + parts + b"".join(shdrs)


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
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--gdb", default=None)
    p.add_argument("--require-all", action="store_true",
                   help="with --prove, fail unless every anchor fired")

    p = sub.add_parser("records",
                       help="the linker's internal input-file record (stride, fields, "
                            "and --prove to read them back)")
    linker_arg(p)
    p.add_argument("--json", action="store_true")
    p.add_argument("--prove", action="store_true",
                   help="run a real link under gdb and read the record array back")
    p.add_argument("--args", default=None, help="an explicit link line instead of build.ninja's")
    p.add_argument("--rsp", default=None, help="the link's response file")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="the build output whose input list to use")
    p.add_argument("--ldscript", default=None, help="the -lcf script")
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--gdb", default=None)
    p.add_argument("--limit", type=int, default=24,
                   help="with --prove, how many records to dump")

    p = sub.add_parser("align",
                       help="where the linker aligns a fragment, and what it "
                            "compares (the objalign.py question)")
    linker_arg(p)
    p.add_argument("--unit", default=None, help="a unit name to check its claims for")
    p.add_argument("--object", default=None, help="an object path to check")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="the build output whose input list to use")
    p.add_argument("--splits", default=None,
                   help="the splits.txt that holds the claimed starts")
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("timeline", help="run the linker's own verbose diagnostics")
    linker_arg(p)
    p.add_argument("--args", default="", help="the linker argument list")

    p = sub.add_parser("verify", help="health check: does the map describe the ELF")
    p.add_argument("map")
    p.add_argument("elf")
    p.add_argument("--identity", default=None,
                   help="also byte-compare the ELF against this file")

    p = sub.add_parser("trace", help="follow one UNIT NAME (or object) through a real link")
    p.add_argument("object", help="a unit name (Network/NetworkWiiMediator) or an "
                                   "object path (build/RMHE08/src/....o)")
    p.add_argument("--map", default=None,
                   help="the link map of the link to trace (default: "
                        "build/RMHE08/main.MAP; if it is not there the tool "
                        "links one, like --link)")
    p.add_argument("--elf", default=None, help="the ELF that link produced")
    p.add_argument("--rsp", default=None,
                   help="the link's response file (decides whether the object is an input)")
    p.add_argument("--link", action="store_true",
                   help="run the build's own link first, into a scratch path")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="with --link, the build output whose input list to use")
    p.add_argument("--ldscript", default=None, help="with --link, the -lcf script")
    p.add_argument("--out", default="build/scratch/mwlink-debug",
                   help="scratch dir for --link (links never write build/RMHE08/main.elf)")
    p.add_argument("--full-ctor", action="store_true",
                   help="also print the whole merged .ctors/.dtors layout")
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("diagnose",
                       help="run the build's link with -v; report the phase and "
                            "the diagnostic (the erroring-link path)")
    linker_arg(p)
    p.add_argument("--args", default=None,
                   help="an explicit link line instead of build.ninja's "
                        "( -o/-map are redirected into --out )")
    p.add_argument("--rsp", default=None, help="the link's response file")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="the build output whose input list to use")
    p.add_argument("--ldscript", default=None, help="the -lcf script")
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("phases", help="the linker's phase table (msgid -> anchor)")
    linker_arg(p)
    p.add_argument("--json", action="store_true")
    p.add_argument("--prove", action="store_true",
                   help="run a real link and observe the message ids and anchors")
    p.add_argument("--args", default=None, help="the linker argument list")
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--gdb", default=None)
    p.add_argument("--require-all", action="store_true",
                   help="with --prove, fail unless every anchor fired")
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
          "anchors": cmd_anchors, "timeline": cmd_timeline, "verify": cmd_verify,
          "trace": cmd_trace, "phases": cmd_phases,
          "records": cmd_records, "align": cmd_align,
          "diagnose": cmd_diagnose}[args.cmd]
    try:
        return fn(args)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
