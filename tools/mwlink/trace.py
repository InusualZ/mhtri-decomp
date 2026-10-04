"""One input object followed through one real link: landing, symbol resolution, relocation read-back.
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import re
import struct
from pathlib import Path

from tools.lib.binary import elf
from tools.lib.binary.elf import SHN_ABS, SHN_UNDEF, SHT_NOBITS, STB_LOCAL, STT_FILE, STT_SECTION
from tools.mwlink.mapfile import OutputElf, _map_rows, is_section_row, parse_map_symbols


def reloc_name(t):
    """A relocation type's short name (``ADDR32``, ``EMB_SDA21``), or ``type N`` for a number with no name; the
    names are ``lib.binary.elf``'s ``R_PPC_*`` table."""
    name = elf.RELOC_NAMES.get(t)
    return name[len("R_PPC_"):] if name else f"type {t}"


class MwObject:
    """One input object: its sections, symbols and relocations, as dicts, read through ``lib.binary.elf``."""

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
        parsed = elf.Elf(d, str(self.path))
        self.shentsize, self.shnum = parsed.shentsize, parsed.shnum
        self.sections = [{"index": s.index, "name": s.name, "type": s.type, "flags": s.flags, "addr": s.addr,
                          "offset": s.offset, "size": s.size, "link": s.link, "info": s.info, "align": s.align,
                          "entsize": s.entsize, "sh_name": s.name_offset} for s in parsed.sections]
        nsec = len(self.sections)
        self.symbols = []
        for sym in parsed.symbols:
            sh = self.sections[sym.shndx] if sym.shndx < nsec else None
            self.symbols.append({
                "index": sym.index, "name": sym.name, "value": sym.value, "size": sym.size,
                "bind": sym.bind, "type": sym.type, "shndx": sym.shndx,
                "shndx_name": sh["name"] if sh else ("ABS" if sym.shndx == SHN_ABS else "UNDEF"),
                "shndx_value": sh["addr"] if sh else 0,
            })
        self.relocs = []
        for rel in parsed.relocs():
            info = parsed.sections[rel.rela_index].info
            self.relocs.append({
                "section": rel.rela,
                "target": self.sections[info]["name"] if info < nsec else "?",
                "target_index": info,
                "offset": rel.offset, "type": rel.type, "sym_index": rel.symbol,
                "addend": rel.addend,
                "sym_name": self.symbols[rel.symbol]["name"] if rel.symbol < len(self.symbols) else "?",
            })

    def section(self, name):
        for s in self.sections:
            if s["name"] == name:
                return s
        return None

    def contents(self, sec):
        """The section's bytes; empty for a NOBITS section (.bss/.sbss)."""
        if sec["type"] == SHT_NOBITS or sec["size"] == 0:
            return b""
        return self.data[sec["offset"]:sec["offset"] + sec["size"]]

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

    out_elf = OutputElf(elf_path) if elf_path and Path(elf_path).exists() else None
    elf_secs = {s["name"]: s for s in out_elf.sections()} if out_elf else {}
    elf_data = out_elf.data if out_elf else b""
    elf_syms = out_elf.symbols() if out_elf else {}

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
                "verdict": "DROPPED", "elf": str(elf_path) if out_elf else None,
                "map": None, "rows_total": 0}

    report = {"object": str(obj_path), "object_name": name, "kept": True,
              "in_rsp": None if rsp is None else any(
                  Path(ln.replace("\\", "/")).name == name for ln in rsp),
              "rows_total": len(mine), "sections": [], "symbols": [],
              "relocations": [], "ctor_dtor": [], "problems": problems,
              "elf": str(elf_path) if out_elf else None, "map": None}

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
        if out_elf and body and r["size"] and osec is not None:
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
        if out_elf and out_sec in elf_secs:
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

