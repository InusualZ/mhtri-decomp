"""The link map: parse it, read its generated symbols, and verify it describes the output ELF.
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import re
from pathlib import Path

from tools.lib.binary import elf


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


class OutputElf:
    """The linked ELF as ``verify`` and ``trace`` read it: section dicts and ``{name: [value, ...]}``, read
    through ``lib.binary.elf`` (the one ELF reader)."""

    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        if self.data[:4] != b"\x7fELF":
            raise ValueError(f"{self.path} is not an ELF")
        if self.data[4] != 1:
            raise ValueError(f"{self.path} is not ELF32")
        self.elf = elf.Elf(self.data, str(self.path))

    def sections(self):
        return [{"name": s.name, "type": s.type, "addr": s.addr, "offset": s.offset, "size": s.size,
                 "flags": s.flags} for s in self.elf.sections]

    def symbols(self):
        """``{name: [value, ...]}`` from the output ELF's own symbol table.

        This is the artifact's view of a symbol's address - the second,
        independent witness for the map's rows (and where ``_SDA_BASE_``,
        which the map only prints in its symbol-file listing, comes from).
        """
        out = {}
        for sym in self.elf.symbols:
            if sym.name:
                out.setdefault(sym.name, []).append(sym.value)
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
    out_elf = OutputElf(elf_path)
    sections = {s["name"]: s for s in out_elf.sections()}
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

