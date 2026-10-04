#!/usr/bin/env python3
"""Name a unit's generated callees from the target object's relocations: owner, state and call shape.
Spec: docs/tools/spec/callees.md. CLI: callees.py <unit> [--json] [--limit N] [--no-shape] [--no-scan] | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import struct
import sys

from tools.lib import proc as _proc
from tools.lib import repo as _repo
from tools.lib import units as _units
from tools.lib import names as libnames
from tools.lib import ppc as _ppc
from tools.lib.binary import objdump as lib_objdump
from tools.lib.binary.elf import Elf, ElfError, reloc_name
from tools.lib.project import Ownership, Splits  # the one ownership index; the lint's too
from tools.lib.project.ownership import owner_label, source_exists
from tools.symbols import symedit  # the bounded, classified in-repo reference scan (its public API)

ROOT = str(pathlib.Path(__file__).resolve().parents[2])

CODE_SECTIONS = (".text", ".init")
CALL_TYPES = (10,)          # R_PPC_REL24 - the `bl` form (what a C call compiles to)
# Which symbol the *target* object's relocation names; MWCC also emits 109 (EABI SDA21) for pool loads.


def is_generated(name):
    return libnames.is_generated(name, "rule7")


# --------------------------------------------------------------------------------------------------
# the object: which generated symbols each side's bodies reference
# --------------------------------------------------------------------------------------------------
def code_references(blob):
    """-> ({name: [ref, ...]}, {name}) for generated names referenced from a code section of `blob`.

    A *reference* is a relocation whose target section is code: that is the body's own use of the symbol
    (a call, or the address of a data label taken for a load). The second return value is every name the
    object *defines* here, so the caller can say whether the callee is this unit's own or another's.
    """
    try:
        elf = Elf.read(blob)
    except ElfError:
        raise ValueError("not an ELF object") from None
    defined = {s.name for s in elf.symbols if s.shndx}
    refs = {}
    for r in elf.relocs():
        name = r.symbol_name if r.symbol < len(elf.symbols) else None
        if r.section not in CODE_SECTIONS or not is_generated(name):
            continue
        refs.setdefault(name, []).append(dict(
            offset=r.offset, type=r.type, type_name=reloc_name(r.type, "type-%d"),
            call=r.type in CALL_TYPES, addend=r.addend,
            defined_here=name in defined))
    return refs, defined


def merge_sides(target_refs, ours_refs):
    """-> [{name, target:[refs], ours:[refs], defined_here}] for the union, target names first."""
    rows = []
    for name in sorted(set(target_refs) | set(ours_refs),
                       key=lambda n: (n not in target_refs, n)):
        t, o = target_refs.get(name, []), ours_refs.get(name, [])
        rows.append(dict(name=name, target=t, ours=o,
                         defined_here=any(r["defined_here"] for r in t + o)))
    return rows


# --------------------------------------------------------------------------------------------------
# ownership: symbols.txt + splits.txt, through the lint's own index
# --------------------------------------------------------------------------------------------------
classify_owner = owner_label            # (label, state, unit) for one `Ownership.resolve` result
make_source_exists = source_exists      # does the unit named in `splits.txt` have its file under `src/`?


# --------------------------------------------------------------------------------------------------
# the call shape: read the reference site out of the target's disassembly
# --------------------------------------------------------------------------------------------------
FN_RE = re.compile(r"^\.fn\s+([^,\s]+)")


def parse_disasm(text):
    """-> (insns, base) from `dtk elf disasm` output (lines read by `lib.binary.objdump.tokenize`).

    `insns` is a list of dicts (`offset`, `va`, `mnemonic`, `operands`, `func`, `raw`) in file order, and
    `base` is the object's `.text` base address (the first function header's `va - offset`), so a
    relocation's `.text` offset selects the instruction whose `va == base + offset`. `func` is the `.fn`
    the instruction belongs to, so a backward window can stop at the function's own start.
    """
    insns, base, func = [], None, None
    for line in text.splitlines():
        tok = lib_objdump.tokenize(line)
        if tok is None:
            continue
        if tok.kind == "fn":
            func = tok.name
        elif tok.kind == "endfn":
            func = None
        elif tok.kind == "header":
            if tok.name in CODE_SECTIONS and base is None:
                base = tok.address - tok.offset
        elif tok.kind == "insn" and base is not None and line.startswith("/*"):
            insns.append(dict(offset=tok.address - base, va=tok.address, mnemonic=tok.mnemonic.rstrip("."),
                              operands=tok.operands, func=func, raw=tok.text))
    return insns, base


# The register read/write decode of a disassembled instruction is `lib.ppc.decode_rw` (one decoder).
_CALL = _ppc.CALL_MNEMONICS
decode_rw = _ppc.decode_rw


def call_shape(insns, index, window=24):
    """-> (args, returns_used, note) inferred from the instruction at `index` (a call).

    The argument registers are those the caller *materialises* since the last branch or call - the window
    MWCC sets arguments in - and the arity is the highest such register (`r3..rN`). When none is written
    the call passes nothing the caller materialised; if the instruction before the window is a call, `r3`
    carries its return into the first argument and that is said explicitly (`0 (r3 live-in)`) rather than
    guessed one way or the other. The return is "used" when the first read of `r3` after the call precedes
    the next branch or call. `note` records what made the answer uncertain.
    """
    writes, unknown_back, stop = set(), 0, "start"
    func = insns[index].get("func") if 0 <= index < len(insns) else None
    j = index - 1
    while j >= 0 and index - j <= window:
        insn = insns[j]
        if func is not None and insn.get("func") != func:
            break
        reads, w, is_branch, is_call, decoded = decode_rw(insn["mnemonic"], insn["operands"])
        if is_branch:
            stop = "call" if is_call else "branch"
            break
        if not decoded:
            unknown_back += 1
        writes |= {g for g in w if 3 <= g <= 10}
        j -= 1
    if writes:
        hi = max(writes)
        args = "r3" if hi == 3 else "r3..r%d" % hi
    elif stop == "call":
        args = "0 (r3 live-in)"
    elif stop == "branch":
        # The window reached a branch without materialising an argument: r3 can still be live from the
        # other side of the branch, and only dataflow would settle it - say so rather than claim 0.
        args = "0?"
    else:
        args = "0"
    used, unknown_fwd = False, 0
    k = index + 1
    while k < len(insns) and k - index <= window:
        insn = insns[k]
        reads, w, is_branch, is_call, decoded = decode_rw(insn["mnemonic"], insn["operands"])
        if is_call or is_branch:
            break
        if not decoded:
            unknown_fwd += 1
        if 3 in reads and not used:
            used = True
            break
        if 3 in w:
            break       # the return is overwritten before any read: it is dead
        k += 1
    notes = []
    if unknown_back:
        notes.append("%d undecoded before" % unknown_back)
    if unknown_fwd:
        notes.append("%d undecoded after" % unknown_fwd)
    if used:
        ret = "used"
    elif unknown_fwd:
        ret = "?"
    else:
        ret = "unused"
    if notes:
        ret += " (%s)" % ", ".join(notes)
    return args, ret, "; ".join(notes)


def _short_reloc(type_name):
    """`R_PPC_ADDR16_HA` -> `HA`, `R_PPC_EMB_SDA21` -> `SDA21` - the cell has to stay one line."""
    return (type_name or "?").replace("R_PPC_", "").replace("ADDR16_", "").replace("EMB_", "")


def shape_column(row, insn_index, insns):
    """The `shape` cell for one symbol's reference row - from its first call site, else the reloc type."""
    calls = [r for r in row["target"] if r["call"]]
    if not calls:
        types = sorted({_short_reloc(r["type_name"]) for r in row["target"] + row["ours"]})
        return "data (%s)" % (", ".join(types) if types else "?"), None
    ref = calls[0]
    insn = insn_index.get(ref["offset"])
    if insn is None:
        return "call (site not in the disassembly)", None
    args, ret, _note = call_shape(insns, insn)
    return "call args=%s ret=%s" % (args, ret), {"args": args, "ret": ret,
                                                "caller": insns[insn].get("func")}


# --------------------------------------------------------------------------------------------------
# disassembly and reference scan
# --------------------------------------------------------------------------------------------------
def disassemble(obj, tool=None):
    """`dtk elf disasm` of `obj` as text, or (None, reason) when the tool or the run is unavailable.

    The target object is the one with a `.note.split` (the split piece); our own object disassembles too,
    but the target is the authority and is what the tool passes here.
    """
    tool = tool or os.path.join(ROOT, "build", "tools", "dtk.exe")
    if not os.path.exists(tool):
        return None, "dtk not found at %s (call shape unavailable)" % os.path.relpath(tool, ROOT)
    try:
        return lib_objdump.dtk_disasm(obj, tool, cwd=ROOT), None
    except RuntimeError as exc:
        return None, str(exc)


def scan_refs(names, roots=("src", "include"), limit=40, root=None):
    """-> {name: {"groups": ..., "files": [rel, ...], "cross_file": bool, "n_code": int}}.

    The same bounded, `path`-aware scan `symedit.find_refs` + `symedit.group_hits` do, but in **one pass**:
    `find_refs` loops every name over every line (O(names x lines)), which is 60 s on a 275-symbol unit,
    while a single alternation is linear. The classification is still symedit's `ref_kinds`, so the `path`
    bucket that protects `#include`s is applied here too.
    """
    names = [n for n in names if n]
    if not names:
        return {}
    root = root or ROOT
    # Longest first, so a shorter stem can never win an alternation over a longer one; `\b` already
    # stops `fn_800501` from matching inside `fn_80050100` (a digit is a word character).
    ordered = sorted(set(names), key=len, reverse=True)
    pattern = re.compile("|".join(r"\b%s\b" % re.escape(n) for n in ordered))
    hits = {n: [] for n in ordered}
    for rel_root in roots:
        base_root = os.path.join(root, rel_root)
        if not os.path.isdir(base_root):
            continue
        for base, dirs, files in os.walk(base_root):
            dirs[:] = [d for d in dirs if d not in (".git", "build", "__pycache__")]
            for fn in files:
                if not fn.endswith(symedit.REF_SUFFIXES):
                    continue
                p = os.path.join(base, fn)
                try:
                    with open(p, "r", encoding="utf-8", errors="replace") as fh:
                        for lineno, line in enumerate(fh, 1):
                            found = {m.group(0) for m in pattern.finditer(line)}
                            if not found:
                                continue
                            rel = os.path.relpath(p, root).replace(os.sep, "/")
                            for name in found:
                                if len(hits[name]) < limit:
                                    hits[name].append((rel, lineno, line.strip()[:160]))
                except OSError:
                    continue
    out = {}
    for name in names:
        groups = symedit.group_hits(name, hits.get(name, []))
        files = sorted({h[0] for h in groups["code"]})
        out[name] = dict(groups=groups, files=files, cross_file=len(files) > 1,
                         n_code=len(groups["code"]))
    return out


# --------------------------------------------------------------------------------------------------
# the report
# --------------------------------------------------------------------------------------------------
def report(unit, args, root=ROOT):
    """The whole survey for `unit` as a dict, so `main` can print it and a test can read it."""
    target = unit.obj_target
    ours = unit.obj_ours
    side_note = None
    if not os.path.exists(target):
        if not os.path.exists(ours):
            raise SystemExit("no split target object for %s\n  expected: %s\n"
                             "  (a proposal unit has no split object until it is registered)"
                             % (unit.report_name, os.path.relpath(target, root)))
        target, ours, side_note = ours, None, "target object missing; read our object instead"
    with open(target, "rb") as fh:
        target_refs, _defined = code_references(fh.read())
    ours_refs = {}
    if ours and os.path.exists(ours):
        try:
            with open(ours, "rb") as fh:
                ours_refs, _ = code_references(fh.read())
        except ValueError:
            ours_refs = {}
    rows = merge_sides(target_refs, ours_refs)

    ownership = Ownership.load(root)
    exists = make_source_exists(root)
    refs = scan_refs([r["name"] for r in rows], root=root) if not args.no_scan else {}

    insns, insn_index, disasm_note = [], {}, None
    calls = [r for row in rows for r in row["target"] if r["call"]]
    if calls and not args.no_shape:
        text, disasm_note = disassemble(target)
        if text:
            insns, _base = parse_disasm(text)
            insn_index = {i["offset"]: n for n, i in enumerate(insns)}
        else:
            insns, insn_index = [], {}
    for row in rows:
        if not row["target"] and row["ours"]:
            row["shape"] = "ours only"
            row["caller"] = None
        else:
            row["shape"], info = shape_column(row, insn_index, insns)
            row["caller"] = (info or {}).get("caller")
        res = ownership.resolve(row["name"]) if ownership is not None else None
        label, state, owner_unit = classify_owner(res, exists)
        band = (res or {}).get("module") if (res or {}).get("kind") == "unsplit" else None
        row.update(owner_label=label, owner_state=state, owner_unit=owner_unit,
                   owner_band=band,
                   owner_show=owner_unit or (("band %s" % band) if band else "-"),
                   sides=("target" if row["target"] else "") +
                         ("+ours" if row["target"] and row["ours"] else
                          ("ours" if row["ours"] else "")),
                   sites_target=len(row["target"]), sites_ours=len(row["ours"]))
        info = refs.get(row["name"], {})
        row["ref_files"] = info.get("files", [])
        row["cross_file"] = info.get("cross_file", False)
        row["ref_detail"] = info.get("groups", {})
    rows.sort(key=lambda r: (r["owner_state"], r["name"]))
    return dict(unit=unit, target=target, ours=ours, side_note=side_note, rows=rows,
                disasm_note=disasm_note, scanned=not args.no_scan,
                text_range=text_range(unit, root))


def text_range(unit, root):
    """The unit's `.text` split range and size, from `splits.txt`; None when it is not registered.

    The block is keyed by the source path relative to `src/` (`Network/network_state.cpp`), which is the
    same key rule 2's ownership index uses.
    """
    path = os.path.join(root, "config", "RMHE08", "splits.txt")
    if not os.path.exists(path):
        return None
    want = os.path.relpath(unit.source, os.path.join(root, "src")).replace(os.sep, "/")
    block = next((b for b in Splits.read(path).blocks if b.unit == want), None)
    for r in (block.ranges if block else ()):
        if r.section == ".text":
            return dict(start=r.start, end=r.end, size=r.size, rename=r.rename)
    return None


def print_report(rep):
    unit = rep["unit"]
    print("== %s: the generated symbols its bodies reference" % unit.report_name)
    print("   src    %s" % os.path.relpath(unit.source, ROOT))
    print("   object %s%s" % (os.path.relpath(rep["target"], ROOT),
                              "  (read instead of the target)" if rep["side_note"] else ""))
    if rep["text_range"]:
        t = rep["text_range"]
        print("   .text  0x%08X..0x%08X (%d B)%s" % (t["start"], t["end"], t["size"],
                                                      "  rename:%s" % t["rename"] if t["rename"] else ""))
    rows = rep["rows"]
    if not rows:
        print()
        print("   no generated references - every symbol its bodies reference has a real name.")
        print("   (nothing to rename before this batch; write the body.)")
        return 0
    target_n = sum(r["sites_target"] for r in rows)
    print()
    print("   %d generated symbol(s), %d reference site(s) in the target object: rename every one"
          % (len(rows), target_n))
    print("   before this batch *adds* a reference (conventions rule 7).")
    if rep["disasm_note"]:
        print("   note: %s" % rep["disasm_note"])
    print()
    hdr = "%-3s %-16s %-6s %-5s %-30s %-13s %-38s %-4s %s" % (
        "#", "symbol", "sides", "sites", "owner", "state", "shape (first site)", "files", "flags")
    print(hdr)
    print("-" * len(hdr))
    for i, r in enumerate(rows, 1):
        flags = []
        if r["cross_file"]:
            flags.append("CROSS-FILE")
        if r["defined_here"]:
            flags.append("defined-here")
        if r["ours"] and not r["target"]:
            flags.append("RULE7-OURS")
        print("%-3d %-16s %-6s %-5d %-30s %-13s %-38s %-4d %s" % (
            i, r["name"], r["sides"], r["sites_target"] + r["sites_ours"],
            r["owner_show"][:30], r["owner_state"], r["shape"][:38],
            len(r["ref_files"]), " ".join(flags)))
    states = sorted({r["owner_state"] for r in rows})
    legend = {"reconstructed": "owner has source in src/ (rename its source too)",
              "registered": "a split range, no source yet (the map row is the only half)",
              "unsplit": "no registered owner (declare in include/unsplit/)",
              "unmapped": "not in the symbol map",
              "duplicate": "more than one map row"}
    print()
    for s in states:
        n = sum(1 for r in rows if r["owner_state"] == s)
        print("   %-13s %-2d  %s" % (s, n, legend.get(s, "")))
    if any(r["cross_file"] for r in rows):
        print("   %-13s %-2d  its code references live in more than one file: use a repo-wide rename"
              % ("CROSS-FILE", sum(1 for r in rows if r["cross_file"])))
    if not rep["scanned"]:
        return 0
    cross = [r for r in rows if r["cross_file"]]
    ours_only = [r for r in rows if r["ours"] and not r["target"]]
    if cross:
        print()
        print("   references spanning more than one file - use a repo-wide rename, not a local edit:")
        for r in cross:
            print("     %s  (%s)" % (r["name"], ", ".join(r["ref_files"])))
    if ours_only:
        print()
        print("   in OUR object but not the target - the batch's own new references (fix before landing):")
        for r in ours_only:
            print("     %s" % r["name"])
    print()
    print("   next: pick the name from the callee's owner/source, then"
          "  python tools/symbols/symedit.py rename <old> <new>")
    print("         (verify with  python .claude/skills/mwcc-unit-matching/scripts/mt.py diff -u %s <new>)"
          % unit.report_name)
    return 0


# --------------------------------------------------------------------------------------------------
# self-test: fixtures only (no build tree, no compiler), so the run is identical in MAIN and a fresh
# worktree. `dossier.selftest` is the model: the ELF, the map, the splits and the source tree are built
# into a temp directory and the whole pipeline is exercised against them.
# --------------------------------------------------------------------------------------------------
def _fixture_elf(text_bytes, syms, relocs):
    """An ELF32-BE MWCC-shaped object: `syms` = (name, value, size, info, shndx), `relocs` = (off, sym, type).

    Only what `code_references` reads is built: a `.text`, a SHT_RELA targeting it, a `.symtab`/`.strtab`
    pair and the section-name table. The check is that this tool's own readers work on a known input, not
    that MWCC's format is re-implemented.
    """
    shstr = bytearray(b"\0")
    names = {}
    for n in ("", ".text", ".rela.text", ".symtab", ".strtab", ".shstrtab"):
        names[n] = len(shstr)
        shstr += n.encode() + b"\0"
    strtab = bytearray(b"\0")
    soff = [0]                          # index 0 is the null symbol: its name offset is 0
    for name in [s[0] for s in syms[1:]]:
        soff.append(len(strtab))
        strtab += name.encode() + b"\0"
    symtab = bytearray()
    for i, (name, value, size, info, shndx) in enumerate(syms):
        symtab += struct.pack(">IIIBBH", soff[i], value, size, info, 0, shndx)
    rela = bytearray()
    for off, sym, type_ in relocs:
        rela += struct.pack(">IIi", off, (sym << 8) | type_, 0)

    def pad(b, n=4):
        while len(b) % n:
            b += b"\0"
        return b

    out = bytearray(b"\0" * 52)                     # the ELF header, filled in last
    text_off = len(out)
    out += pad(bytes(text_bytes))
    rela_off = len(out)
    out += pad(bytes(rela))
    sym_off = len(out)
    out += pad(bytes(symtab))
    str_off = len(out)
    out += pad(bytes(strtab))
    shstr_off = len(out)
    out += pad(bytes(shstr))
    shoff = len(out)
    shdrs = [
        (0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                                      # null
        (names[".text"], 1, 6, 0, text_off, len(text_bytes), 0, 0, 4, 0),
        (names[".rela.text"], 4, 0x40, 0, rela_off, len(rela), 3, 1, 4, 12),
        (names[".symtab"], 2, 0, 0, sym_off, len(symtab), 4, 1, 4, 16),
        (names[".strtab"], 3, 0, 0, str_off, len(strtab), 0, 0, 1, 0),
        (names[".shstrtab"], 3, 0, 0, shstr_off, len(shstr), 0, 0, 1, 0),
    ]
    for sh in shdrs:
        out += struct.pack(">IIIIIIIIII", *sh)
    header = bytearray(b"\x7fELF\x01\x02\x01" + b"\0" * 9)
    header += struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0, 40, len(shdrs), 5)
    out[0:52] = header
    return bytes(out)


def _fixture_symbols():
    return "\n".join([
        "caller = .text:0x80001000; // type:function size:0x20 scope:global",
        "fn_12345678 = .text:0x80002000; // type:function size:0x10 scope:global",
        "fn_11111111 = .text:0x80003000; // type:function size:0x10 scope:global",
        "realName = .text:0x80004000; // type:function size:0x10 scope:global",
        "lbl_87654321 = .data:0x80500000; // type:object size:0x4 scope:global",
        ""]) + "\n"


def _fixture_splits():
    return ("Sections:\n\t.text       type:code align:4\n\n"
            "Lib/file.cpp:\n\t.text       start:0x80003000 end:0x80003100\n")


def selftest():
    """Fixture-driven checks for every reader this tool has; returns a process exit code."""
    import argparse
    import io
    import contextlib
    import shutil
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s\n      got  %r\n      want %r" % (name, got, want))

    # --- generated-name detection ---------------------------------------------------------------
    check("fn_ is generated", is_generated("fn_8018B3B8"), True)
    check("lbl_ is generated", is_generated("lbl_80594EE0"), True)
    check("loc_ is generated", is_generated("loc_805113B0"), True)
    check("a real name is not generated", is_generated("resetNetworkState3"), False)
    check("a bare fn_ is not generated", is_generated("fn_"), False)
    check("a non-hex stem is not generated", is_generated("lbl_not_a_label"), False)

    # --- decode_rw: the register read/write rules the call shape rests on --------------------------
    check("decode li writes r3", decode_rw("li", "r3, 0x0")[:2], (set(), {3}))
    check("decode mr writes the dest and reads the source", decode_rw("mr", "r31, r3")[:2], ({3}, {31}))
    check("decode stw reads data and base", decode_rw("stw", "r0, 0x24(r1)")[:2], ({0, 1}, set()))
    check("decode lwz writes the dest and reads the base", decode_rw("lwz", "r5, 0xb90(r29)")[:2],
          ({29}, {5}))
    check("decode cmpwi reads its operand", decode_rw("cmpwi", "r3, 0x0")[:2], ({3}, set()))
    check("decode clrlwi reads then writes r0", decode_rw("clrlwi", "r0, r0, 24")[:2], ({0}, {0}))
    check("decode bl is a branch and a call", decode_rw("bl", "fn_1")[2:4], (True, True))
    check("decode beq is a branch, not a call", decode_rw("beq", ".L_1")[2:4], (True, False))
    check("decode a float op touches no GPR", decode_rw("fadds", "f1, f0, f1")[:2], (set(), set()))
    check("decode an sda21 float load ignores r0", decode_rw("lfs", "f0, lbl_1@sda21(r0)")[:2],
          (set(), set()))
    check("decode psq_lx reads its address GPRs", decode_rw("psq_lx", "f0, r3, r4, 0, 0")[:2],
          ({3, 4}, set()))
    check("decode an unknown mnemonic is reported undecoded", decode_rw("vperm", "r3, r4, r5, r6")[4],
          False)

    # --- parse_disasm + call_shape ---------------------------------------------------------------
    dis = "\n".join([
        "# .text:0x0 | 0x80001000 | size: 0x40",
        ".fn caller, global",
        "/* 80001000 00000000  38 60 00 01 */\tli r3, 0x1",
        "/* 80001004 00000004  38 80 00 02 */\tli r4, 0x2",
        "/* 80001008 00000008  48 00 00 01 */\tbl fn_A",
        "/* 8000100c 0000000c  7C A3 2B 78 */\tmr r5, r3",
        "/* 80001010 00000010  48 00 00 01 */\tbl fn_B",
        "/* 80001014 00000014  80 61 00 14 */\tlwz r3, 0x14(r1)",
        "/* 80001018 00000018  38 03 00 E1 */\taddi r0, r3, 0xe1",
        "/* 8000101c 0000001c  4E 80 00 20 */\tblr",
        ".endfn caller",
        "",
    ])
    insns, base = parse_disasm(dis)
    check("parse_disasm derives the text base", base, 0x80001000)
    check("parse_disasm keeps every instruction", len(insns), 8)
    check("parse_disasm computes the object offset", insns[2]["offset"], 8)
    check("parse_disasm splits mnemonic and operands", (insns[5]["mnemonic"], insns[5]["operands"]),
          ("lwz", "r3, 0x14(r1)"))
    check("call_shape reads the args the caller materialises", call_shape(insns, 2)[:2], ("r3..r4", "used"))
    check("call_shape: a following call makes the return dead", call_shape(insns, 4)[:2], ("r3..r5", "unused"))

    # the bug this pins: an `lwz r3` after a call *kills* the return; `addi r0, r3, ...` later is not a use
    dead = parse_disasm(dis)[0]
    check("call_shape: a reloaded r3 is not the return used", call_shape(dead, 4)[1], "unused")
    live_in = parse_disasm("# .text:0x0 | 0x80001000 | size: 0x8\n"
                           "/* 80001000 00000000  48 00 00 01 */\tbl fn_A\n"
                           "/* 80001004 00000004  48 00 00 01 */\tbl fn_B\n")[0]
    check("call_shape: an empty window after a call is r3 live-in", call_shape(live_in, 1)[:2],
          ("0 (r3 live-in)", "unused"))
    branch = parse_disasm("# .text:0x0 | 0x80001000 | size: 0x8\n"
                          "/* 80001000 00000000  41 82 00 00 */\tbeq .L_1\n"
                          "/* 80001004 00000004  48 00 00 01 */\tbl fn_A\n")[0]
    check("call_shape: a branch leaves r3 possibly live", call_shape(branch, 1)[:2], ("0?", "unused"))
    at_start = parse_disasm("# .text:0x0 | 0x80001000 | size: 0x8\n"
                            ".fn first, global\n"
                            "/* 80001000 00000000  4E 80 00 20 */\tblr\n"
                            ".endfn first\n"
                            "# .text:0x4 | 0x80001004 | size: 0x4\n"
                            ".fn second, global\n"
                            "/* 80001004 00000004  48 00 00 01 */\tbl fn_A\n")[0]
    check("call_shape stops at the function start", call_shape(at_start, 1)[:2], ("0", "unused"))
    check("parse_disasm records the owning function", [i["func"] for i in at_start], ["first", "second"])
    undec = parse_disasm("# .text:0x0 | 0x80001000 | size: 0x8\n"
                         "/* 80001000 00000000  48 00 00 01 */\tbl fn_A\n"
                         "/* 80001004 00000004  00 00 00 00 */\tvperm r3, r4, r5, r6\n")[0]
    check("call_shape reports ? rather than guessing across an undecoded instruction",
          call_shape(undec, 0)[1].startswith("?"), True)

    # --- classify_owner --------------------------------------------------------------------------
    check("owner: reconstructed", classify_owner({"kind": "owned", "unit": "Lib/a.cpp"}, lambda u: True),
          ("Lib/a.cpp", "reconstructed", "Lib/a.cpp"))
    check("owner: registered but no source",
          classify_owner({"kind": "owned", "unit": "Lib/a.cpp"}, lambda u: False),
          ("Lib/a.cpp (no source yet)", "registered", "Lib/a.cpp"))
    check("owner: unsplit", classify_owner({"kind": "unsplit", "module": "Network"}, lambda u: False),
          ("unsplit (Network)", "unsplit", None))
    check("owner: duplicate", classify_owner({"kind": "dup"}, lambda u: False)[1], "duplicate")
    check("owner: unmapped", classify_owner(None, lambda u: False)[1], "unmapped")

    # --- the ELF reference extraction ------------------------------------------------------------
    syms = [("", 0, 0, 0, 0), ("caller", 0, 0x20, 0x12, 1),
            ("fn_12345678", 0, 0, 0x10, 0), ("lbl_87654321", 0, 0, 0x10, 0),
            ("realName", 0, 0, 0x10, 0)]
    relocs = [(0x04, 2, 10), (0x08, 3, 6), (0x0C, 3, 4), (0x10, 4, 10)]
    elf = _fixture_elf(b"\x48\x00\x00\x01" * 8, syms, relocs)
    refs, defined = code_references(elf)
    check("ELF: the generated call is a reference", sorted(refs), ["fn_12345678", "lbl_87654321"])
    check("ELF: the call is marked a call", refs["fn_12345678"][0]["call"], True)
    check("ELF: two address relocations are one data reference", len(refs["lbl_87654321"]), 2)
    check("ELF: the data reference is not a call", refs["lbl_87654321"][0]["call"], False)
    check("ELF: a real name is not reported", "realName" in refs, False)
    check("ELF: the defined set is the object's own symbols", sorted(defined), ["caller"])

    # --- the end-to-end report against a fixture repository ---------------------------------------
    tmp = tempfile.mkdtemp(prefix="callees-fixture-")
    try:
        for rel in ("config/RMHE08", "src/Lib", "include/Lib", "build/RMHE08/obj/Lib"):
            os.makedirs(os.path.join(tmp, rel), exist_ok=True)
        with open(os.path.join(tmp, "config/RMHE08/symbols.txt"), "w", encoding="utf-8") as fh:
            fh.write(_fixture_symbols())
        with open(os.path.join(tmp, "config/RMHE08/splits.txt"), "w", encoding="utf-8") as fh:
            fh.write(_fixture_splits())
        with open(os.path.join(tmp, "src/Lib/file.cpp"), "w", encoding="utf-8") as fh:
            fh.write("extern void fn_12345678(void);\n"
                     "extern unsigned char lbl_87654321[];\n"
                     "void caller(void) { fn_12345678(); lbl_87654321[0] = 1; }\n")
        with open(os.path.join(tmp, "include/Lib/shared.h"), "w", encoding="utf-8") as fh:
            fh.write('#include "Lib/fn_12345678.h"\n'
                     "extern void fn_12345678(void);\n"
                     "/* fn_99999999 is only a comment */\n")
        with open(os.path.join(tmp, "build/RMHE08/obj/Lib/file.o"), "wb") as fh:
            fh.write(elf)
        unit = _units.Unit("Lib/file", ".cpp", tmp)
        rep = report(unit, argparse.Namespace(no_scan=False, no_shape=True), root=tmp)
        by_name = {r["name"]: r for r in rep["rows"]}
        check("report: only generated names are rows", sorted(by_name),
              ["fn_12345678", "lbl_87654321"])
        check("report: the owner is resolved as an unsplit address", by_name["fn_12345678"]["owner_state"],
              "unsplit")
        check("report: a multi-file reference is flagged CROSS-FILE", by_name["fn_12345678"]["cross_file"],
              True)
        check("report: the path mention is not a code file",
              "include/Lib/fn_12345678.h" in by_name["fn_12345678"]["ref_files"], False)
        check("report: the file list is the code references", by_name["fn_12345678"]["ref_files"],
              ["include/Lib/shared.h", "src/Lib/file.cpp"])
        check("report: a single-file reference is not CROSS-FILE", by_name["lbl_87654321"]["cross_file"],
              False)
        check("report: without a disassembly the shape says so", by_name["fn_12345678"]["shape"],
              "call (site not in the disassembly)")
        check("report: the splits range is read", rep["text_range"],
              {"start": 0x80003000, "end": 0x80003100, "size": 0x100, "rename": None})

        # the plain-language good answer
        quiet_elf = _fixture_elf(b"\x48\x00\x00\x01" * 4,
                                 [("", 0, 0, 0, 0), ("caller", 0, 0x10, 0x12, 1), ("realName", 0, 0, 0x10, 0)],
                                 [(4, 2, 10)])
        with open(os.path.join(tmp, "build/RMHE08/obj/Lib/file.o"), "wb") as fh:
            fh.write(quiet_elf)
        rep2 = report(unit, argparse.Namespace(no_scan=False, no_shape=True), root=tmp)
        check("report: a unit with no generated references has no rows", rep2["rows"], [])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            print_report(rep2)
        check("report: the no-reference answer is stated plainly",
              "no generated references" in buf.getvalue(), True)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main(argv=None):
    if ("--selftest" in (argv if argv is not None else sys.argv[1:])):
        return selftest()
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", nargs="?", help="a unit spec: Lib/file, main/Lib/file, src/Lib/file.c, or an object")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--limit", type=int, default=40, help="max in-repo references reported per symbol")
    ap.add_argument("--no-shape", action="store_true", help="skip the disassembly (no call shape)")
    ap.add_argument("--no-scan", action="store_true", help="skip the in-repo reference scan")
    args = ap.parse_args(argv)
    if not args.unit:
        ap.error("a unit spec is required (try `Lib/file`)")
    _proc.install_spawn_retry()  # a launch Windows refuses transiently (WinError 5) is retried
    unit = _units.Unit.resolve(args.unit, _repo.repo_root())
    rep = report(unit, args)
    if args.json:
        out = dict(unit=unit.report_name, src=os.path.relpath(unit.source, ROOT),
                   object=os.path.relpath(rep["target"], ROOT), text_range=rep["text_range"],
                   disasm_note=rep["disasm_note"], scanned=rep["scanned"],
                   references=[dict(name=r["name"], sides=r["sides"], shape=r["shape"],
                                    caller=r["caller"],
                                    target_sites=r["sites_target"], our_sites=r["sites_ours"],
                                    owner=r["owner_label"], owner_state=r["owner_state"],
                                    defined_here=r["defined_here"], cross_file=r["cross_file"],
                                    files=r["ref_files"]) for r in rep["rows"]])
        print(json.dumps(out, indent=2))
        return 0
    return print_report(rep)


if __name__ == "__main__":
    sys.exit(main())
