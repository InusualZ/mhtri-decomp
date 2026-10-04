#!/usr/bin/env python3
"""Derive the Wii/1.3 PCode breakpoint table from the backend pass call sites.
Spec: docs/tools/spec/mwcc-debugger.md. CLI: pass_points.py <exe> [rva:rva ...]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import capstone

from tools.lib.binary.pe import Pe

# Drivers, in dump order.  (name, start RVA, end RVA)
DRIVERS = [
    ("CodeGen_Generator", 0x6BEF0, 0x6CFA0),
    ("globallyoptimizepcode", 0x1B8DE0, 0x1B9330),
    ("colorinstructions", 0x1B9810, 0x1B9A20),
]

# Rename pass symbols to upstream's pass names where the mapping is known.
NAMES = {
    "propagatecopyinstructions": "after-copy-propagation",
    "propagateaddinstructions": "after-add-propagation",
    "propagateconstants": "after-constant-propagation",
    "removecommonsubexpressions": "after-common-subexpression-elimination",
    "move_loopinvariant_code": "after-loop-code-motion",
    "strengthreduceloops": "after-loop-strength-reduction",
    "optimizeloops": "after-loop-transforms",
    "deletedeadloads": "after-load-deletion",
    "changearraytoregisters": "after-array-register-transforms",
    "vectorarraystoregs": "after-vector-array-transforms",
    "peepholeoptimizeforward": "after-peephole-forward",
    "peepholeoptimizepcode": "after-peephole",
    "peepholemergeblocks": "after-peephole-merge-blocks",
    "scheduleinstructions": "after-scheduling",
    "generate_epilogue": "after-prologue-epilogue",
    "assemblefunction": "after-assembly",
    "colorinstructions": "after-regalloc",
    "eliminatedeadcode": "after-dead-code-elimination",
    "pcode_expand_pseudo_ops": "after-pseudo-op-expansion",
    "rewrite_memory_references": "after-memory-reference-rewrite",
    "buildinterferencegraph": "after-interference-graph",
    "dumpswitchtables": "after-switch-tables",
    "dumpcodelabels": "after-code-labels",
    "InlineAsm_TranslateIRtoPCode": "after-inline-asm-translation",
    "globallyoptimizepcode": "after-global-optimization",
    "COpt_Optimizer": "after-frontend-optimizer",
    "expandTOCreferences": "after-toc-expansion",
    "DumpIR": "after-optimizations",
    "deleteunreachableblocks": "after-unreachable-block-deletion",
    "initpcode": "after-initpcode",
}


def main():
    exe = sys.argv[1]
    pe = Pe(exe)
    symmap = pe.symbol_map()
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    seen = {}
    for _driver, start, end in DRIVERS:
        code = pe.read_rva(start, end - start)
        for ins in md.disasm(code, pe.image_base + start):
            if ins.mnemonic != "call" or not ins.operands:
                continue
            op = ins.operands[0]
            if op.type != capstone.x86.X86_OP_IMM:
                continue
            name = symmap.get(op.imm - pe.image_base)
            if not name:
                continue
            key = name.lstrip("_")
            label = NAMES.get(key)
            if label is None:
                continue
            ret = ins.address + ins.size - pe.image_base
            seen.setdefault(ret, label)
    for rva, label in sorted(seen.items()):
        print(f"        {rva:#07x}: {label!r},")
    print(f"# {len(seen)} breakpoints", file=sys.stderr)


if __name__ == "__main__":
    main()
