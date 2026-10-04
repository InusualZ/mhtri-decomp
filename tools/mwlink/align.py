"""Where mwld aligns a fragment's address, derived from the code that prints the map's '*fill*' row.
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import re

from tools.lib.project import Splits
from tools.mwlink.anchors import _disas_text, _function_start, derive_anchors


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

