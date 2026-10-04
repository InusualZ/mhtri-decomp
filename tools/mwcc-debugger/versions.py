"""Version-specific MWCC debug data: one table per compiler build, identified by the executable's hash.
Spec: docs/tools/spec/mwcc-debugger.md. CLI: none (data module of mwcc_debugger.py)."""
from __future__ import annotations

from dataclasses import dataclass, field, fields
from pathlib import Path
from typing import Optional

# PORT: the PE reader is the repository's one copy; mwcc_debugger.py (and every locate/ tool that
# imports this module) puts the repository root on sys.path first, inside gdb as well.
from tools.lib.binary.pe import MACHINE_I386, Pe

IMAGE_BASE = 0x400000


@dataclass
class MwccVersion:
    name: str
    layout: str
    image_base: int = IMAGE_BASE
    codegen_start_addr: int = 0
    codegen_end_addr: int = 0
    gfunction_addr: Optional[int] = None
    cmangler_getlinkname_addr: int = 0
    nodenames_addr: int = 0
    nodenames_size: int = 0
    ast_breakpoints: dict = field(default_factory=dict)
    opcodeinfo_addr: int = 0
    opcodeinfo_size: int = 0
    opcodeinfo_stride: int = 0x12
    pcbasicblocks_addr: int = 0
    pcode_breakpoints: dict = field(default_factory=dict)
    regalloc_breakpoint_addr: Optional[int] = None
    interferencegraph_addr: int = 0
    used_virtual_registers_gpr_addr: int = 0
    used_virtual_registers_fpr_addr: int = 0
    coloring_class_addr: Optional[int] = None
    arguments_addr: Optional[int] = None
    locals_addr: Optional[int] = None
    temps_addr: Optional[int] = None
    frame_base_size_addr: Optional[int] = None
    frame_call_args_size_addr: Optional[int] = None
    linkname_needs_call: bool = True
    supports_ast: bool = True
    supports_variables: bool = True
    regalloc_assigned_stack: int = 4
    # coloring class number -> "GPR"/"FPR"; other classes are reported and
    # skipped (see MwccVersion docstring).
    coloring_classes: dict = field(
        default_factory=lambda: {4: "GPR", 3: "FPR"}
    )
    def absolute(self) -> "MwccVersion":
        """Turn every RVA in this row into an absolute VA (what gdb wants)."""
        for f in fields(self):
            value = getattr(self, f.name)
            if f.name.endswith("_addr") and isinstance(value, int):
                setattr(self, f.name, value + self.image_base)
            elif f.name in ("ast_breakpoints", "pcode_breakpoints") and value:
                setattr(
                    self,
                    f.name,
                    {k + self.image_base: v for k, v in value.items()},
                )
        return self


# ---------------------------------------------------------------------------
# The PE / CodeView reader is tools/lib/binary/pe.py (one copy, shared with the
# linker debugger).  Several Wii-era mwcceppc.exe builds ship their own CodeView
# 'NB11' symbol blob (the Metrowerks linker appends it after the last section),
# which names every function and global together with its section and offset.
# That is how the Wii/1.3 row below was cross-checked: locate/dissect.py
# resolves the same symbols by name, and _verify_symbols() re-checks the row
# against the binary every time the tool runs.
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# The rows
# ---------------------------------------------------------------------------

# GC/1.1 and GC/2.6 were lifted mechanically out of upstream's
# init_mwcc_version() by locate/extract_upstream_tables.py (which only converts
# the absolute VAs to RVAs); re-run that script after bumping the vendored
# upstream copy and diff this table.
_GC_ROWS = {
    "GC/1.1": dict(
            codegen_start_addr=0x351b0,
            codegen_end_addr=0x35da9,
            gfunction_addr=None,
            cmangler_getlinkname_addr=0xc2c70,
            nodenames_addr=0x161cb4,
            nodenames_size=75,
            ast_breakpoints={
                0x3538d: 'initial-code', 0x353b4: 'after-optimizations', 0x353b9: 'final-code',
            },
            opcodeinfo_addr=0x1664b0,
            opcodeinfo_size=468,
            pcbasicblocks_addr=0x188474,
            pcode_breakpoints={
                0x35aef: 'initial-code', 0xc4b96: 'after-common-subexpression-elimination',
                0xc4bc9: 'after-copy-propagation', 0xc4bf9: 'after-add-propagation',
                0xc5036: 'after-common-subexpression-elimination',
                0xc5069: 'after-copy-propagation', 0xc50a3: 'after-add-propagation',
                0xc5106: 'after-loop-code-motion', 0xc5136: 'after-loop-strength-reduction',
                0xc513d: 'after-copy-propagation', 0xc516e: 'after-loop-transforms',
                0xc5175: 'after-copy-propagation', 0xc517b: 'after-add-propagation',
                0xc51b7: 'after-copy-propagation', 0xc51eb: 'after-constant-propagation',
                0xc521b: 'after-load-deletion', 0xc524b: 'after-add-propagation',
                0xc527e: 'after-common-subexpression-elimination',
                0xc5285: 'after-copy-propagation',
                0xc4c56: 'after-common-subexpression-elimination',
                0xc4c89: 'after-copy-propagation', 0xc4cc3: 'after-add-propagation',
                0xc4d26: 'after-loop-code-motion', 0xc4d56: 'after-loop-strength-reduction',
                0xc4d5d: 'after-copy-propagation', 0xc4d8e: 'after-loop-transforms',
                0xc4d95: 'after-copy-propagation', 0xc4d9b: 'after-add-propagation',
                0xc4dd7: 'after-copy-propagation', 0xc4e0b: 'after-constant-propagation',
                0xc4e3b: 'after-load-deletion', 0xc4e6d: 'after-copy-propagation',
                0xc4e7c: 'after-add-propagation', 0xc4eb5: 'after-array-register-transforms',
                0xc4ee5: 'after-constant-propagation', 0xc4eec: 'after-copy-propagation',
                0xc4f1d: 'after-common-subexpression-elimination',
                0xc4f27: 'after-copy-propagation', 0xc4fbb: 'after-code-motion',
                0xc4fee: 'after-common-subexpression-elimination',
                0xc4ff5: 'after-copy-propagation', 0x35b6e: 'after-scheduling',
                0x35bca: 'after-peephole-forward', 0x35bee: 'before-regalloc',
                0x35bf3: 'after-regalloc', 0x35ca8: 'after-prologue-epilogue',
                0x35d10: 'after-peephole', 0x35d65: 'after-scheduling',
            },
            regalloc_breakpoint_addr=0xceb04,
            interferencegraph_addr=0x18863c,
            used_virtual_registers_gpr_addr=0x188c72,
            used_virtual_registers_fpr_addr=0x188c70,
            coloring_class_addr=None,
            arguments_addr=0x18886c,
            locals_addr=0x1887b8,
            temps_addr=0x1806c0,
            frame_base_size_addr=0x1888cc,
            frame_call_args_size_addr=0x18792c,
    ),
    "GC/2.6": dict(
            codegen_start_addr=0x33492,
            codegen_end_addr=0x340b1,
            gfunction_addr=0x1e9ec0,
            cmangler_getlinkname_addr=0xfe6a0,
            nodenames_addr=0x1bc980,
            nodenames_size=77,
            ast_breakpoints={
                0x33566: 'initial-code', 0x33599: 'after-optimizations', 0x3359e: 'final-code',
            },
            opcodeinfo_addr=0x1c0fa8,
            opcodeinfo_size=471,
            pcbasicblocks_addr=0x1ea748,
            pcode_breakpoints={
                0x33cf5: 'initial-code', 0x10054e: 'after-common-subexpression-elimination',
                0x100576: 'after-copy-propagation', 0x10057e: 'after-add-propagation',
                0x100981: 'after-peephole-forward',
                0x1009b4: 'after-common-subexpression-elimination',
                0x1009dc: 'after-copy-propagation', 0x1009ee: 'after-add-propagation',
                0x100a1c: 'after-loop-code-motion', 0x100a5e: 'after-loop-strength-reduction',
                0x100a86: 'after-copy-propagation', 0x100a96: 'after-loop-transforms',
                0x100abe: 'after-copy-propagation', 0x100ac1: 'after-add-propagation',
                0x100ae3: 'after-copy-propagation', 0x100b11: 'after-constant-propagation',
                0x100b37: 'after-load-deletion', 0x100b3e: 'after-add-propagation',
                0x100b6b: 'after-peephole-forward',
                0x100b9e: 'after-common-subexpression-elimination',
                0x100bc6: 'after-copy-propagation', 0x1005d0: 'after-peephole-forward',
                0x100604: 'after-common-subexpression-elimination',
                0x10062c: 'after-copy-propagation', 0x10063e: 'after-add-propagation',
                0x10066c: 'after-loop-code-motion', 0x1006ae: 'after-loop-strength-reduction',
                0x1006d6: 'after-copy-propagation', 0x1006f0: 'after-loop-transforms',
                0x100718: 'after-copy-propagation', 0x100720: 'after-add-propagation',
                0x100732: 'after-copy-propagation', 0x100746: 'after-constant-propagation',
                0x10076c: 'after-load-deletion', 0x10077e: 'after-copy-propagation',
                0x100786: 'after-add-propagation', 0x10079e: 'after-array-register-transforms',
                0x1007c3: 'after-constant-propagation', 0x1007f4: 'after-copy-propagation',
                0x100822: 'after-peephole-forward',
                0x100855: 'after-common-subexpression-elimination',
                0x10087d: 'after-copy-propagation', 0x1008f0: 'after-code-motion',
                0x100923: 'after-common-subexpression-elimination',
                0x10094b: 'after-copy-propagation', 0x33e0c: 'after-scheduling',
                0x33e79: 'after-peephole-forward', 0x33ea6: 'before-regalloc',
                0x33eab: 'after-regalloc', 0x33efd: 'after-common-subexpression-elimination',
                0x33f8c: 'after-prologue-epilogue', 0x34006: 'after-peephole',
                0x34063: 'after-scheduling.txt',
            },
            regalloc_breakpoint_addr=0x1089a9,
            interferencegraph_addr=0x1ea768,
            used_virtual_registers_gpr_addr=0x1eaa3c,
            used_virtual_registers_fpr_addr=0x1eaa38,
            coloring_class_addr=0x1eb2cf,
            arguments_addr=None,
            locals_addr=None,
            temps_addr=None,
            frame_base_size_addr=None,
            frame_call_args_size_addr=None,
    ),
}

# Wii/1.3 (build/compilers/Wii/1.3/mwcceppc.exe, `mw_version` Wii/1.3 in
# configure.py).  NOT the GC/1.3.2/2.6 layout: this build is a later Metrowerks
# core (0x272D6A bytes of .text against GC/1.3.2's 0x17C5BC), it ships its own
# CodeView symbol blob, and its object/PCode/block/interference-graph records
# are all different.  locate/README.md is the evidence trail; every data
# address below is re-checked against that symbol blob at run time.
_WII13_ROW = {
    "layout": "wii13",
    # `_CodeGen_Generator` entry is 0x6bef0; this is the instruction right
    # after `mov [0x777000], eax` stores the current function into _gFunction.
    "codegen_start_addr": 0x6C02F,
    # shared tail of `_CodeGen_Generator` (0x6cea1: `add esp, 0x50` on the path
    # every return takes; the function's own `ret` is at 0x6cea8).
    "codegen_end_addr": 0x6CEA1,
    "gfunction_addr": 0x377000,  # _gFunction
    "cmangler_getlinkname_addr": 0x16F7F0,  # _CMangler_GetLinkName
    # The compiler's own node-name table.  _IRO_InitializeNodeNamesArray
    # (0x1de320) fills exactly 89 slots at 0x33a054 with the default name and
    # then overwrites them with pointers to the ECOBJREF/EINTCONST/... strings;
    # the first slot is EPOSTINC and index 0x42 (66) is EINFO, the same
    # numbering GC/1.1 uses.
    "nodenames_addr": 0x33A054,
    "nodenames_size": 89,
    # [esp] is the statement list at each of these three points inside
    # _CodeGen_Generator.  after-optimizations/final-code are 5 bytes apart
    # because that is one `call` (the return address of the frontend pass that
    # expands the data references), the same skeleton upstream's GC rows have.
    "ast_breakpoints": {
        0x6C183: "initial-code",
        0x6C19B: "after-optimizations",
        0x6C1A0: "final-code",
    },
    "opcodeinfo_addr": 0x2F3B9A,  # _opcodeinfo
    # 1577 == the bound `_pcodestats_dump_opcode` compares an opcode against
    # (0x5ecdeb: `cmp word ptr [esp + 0x28], 0x629`), and the table ends
    # exactly where _vletoppc begins: (0x2fc320 - 0x2f3b9a) / 0x16 == 1577.
    "opcodeinfo_size": 1577,
    "opcodeinfo_stride": 0x16,
    "pcbasicblocks_addr": 0x376EDC,  # _pcbasicblocks
    "pcode_breakpoints": {
        0x6c18c: 'after-frontend-optimizer', 0x6c1a0: 'after-toc-expansion',
        0x6c1c0: 'after-initpcode', 0x6c350: 'after-optimizations',
        0x6c915: 'after-inline-asm-translation', 0x6cb92: 'after-unreachable-block-deletion',
        0x6cbe5: 'after-global-optimization', 0x6cc0d: 'after-peephole-merge-blocks',
        0x6cc55: 'after-dead-code-elimination', 0x6cc5f: 'after-regalloc',
        0x6cc72: 'after-dead-code-elimination', 0x6cc7a: 'after-pseudo-op-expansion',
        0x6cc94: 'after-common-subexpression-elimination', 0x6cdd2: 'after-prologue-epilogue',
        0x6cdf1: 'after-peephole-merge-blocks', 0x6cdfc: 'after-peephole',
        0x6ce1a: 'after-scheduling', 0x6ce36: 'after-peephole', 0x6ce49: 'after-assembly',
        0x6ce56: 'after-switch-tables', 0x6ce60: 'after-code-labels',
        0x6cec9: 'after-scheduling', 0x6cedb: 'after-peephole-forward',
        0x6ceef: 'after-peephole', 0x1b8dff: 'after-peephole-merge-blocks',
        0x1b8e09: 'after-peephole-forward', 0x1b8e13: 'after-copy-propagation',
        0x1b8e22: 'after-peephole', 0x1b8e64: 'after-peephole-forward',
        0x1b8e6e: 'after-copy-propagation', 0x1b8e79: 'after-common-subexpression-elimination',
        0x1b8e82: 'after-copy-propagation', 0x1b8e8a: 'after-add-propagation',
        0x1b8ead: 'after-common-subexpression-elimination',
        0x1b8eb3: 'after-constant-propagation', 0x1b8ebb: 'after-copy-propagation',
        0x1b8ecd: 'after-add-propagation', 0x1b8ef7: 'after-loop-code-motion',
        0x1b8f11: 'after-loop-strength-reduction', 0x1b8f23: 'after-loop-transforms',
        0x1b8f34: 'after-copy-propagation', 0x1b8f3c: 'after-add-propagation',
        0x1b8f4e: 'after-copy-propagation', 0x1b8f55: 'after-constant-propagation',
        0x1b8f64: 'after-load-deletion', 0x1b8f76: 'after-copy-propagation',
        0x1b8f7e: 'after-add-propagation', 0x1b8f84: 'after-array-register-transforms',
        0x1b8fa2: 'after-peephole-merge-blocks', 0x1b8fac: 'after-peephole-forward',
        0x1b8fb5: 'after-common-subexpression-elimination',
        0x1b8fd1: 'after-vector-array-transforms', 0x1b8fdd: 'after-copy-propagation',
        0x1b8fe7: 'after-copy-propagation', 0x1b9016: 'after-memory-reference-rewrite',
        0x1b903e: 'after-copy-propagation', 0x1b9068: 'after-copy-propagation',
        0x1b9085: 'after-constant-propagation', 0x1b909a: 'after-copy-propagation',
        0x1b90a9: 'after-copy-propagation', 0x1b90be: 'after-loop-code-motion',
        0x1b90c6: 'after-common-subexpression-elimination',
        0x1b90dd: 'after-common-subexpression-elimination',
        0x1b90e3: 'after-constant-propagation', 0x1b90eb: 'after-copy-propagation',
        0x1b90fd: 'after-add-propagation', 0x1b9127: 'after-loop-code-motion',
        0x1b9141: 'after-loop-strength-reduction', 0x1b9152: 'after-copy-propagation',
        0x1b9159: 'after-loop-transforms', 0x1b916a: 'after-copy-propagation',
        0x1b9172: 'after-add-propagation', 0x1b9184: 'after-copy-propagation',
        0x1b918b: 'after-constant-propagation', 0x1b91a5: 'after-peephole-merge-blocks',
        0x1b91af: 'after-peephole-forward', 0x1b91b8: 'after-common-subexpression-elimination',
        0x1b91c5: 'after-memory-reference-rewrite', 0x1b91ed: 'after-copy-propagation',
        0x1b9216: 'after-load-deletion', 0x1b921d: 'after-add-propagation',
        0x1b9939: 'after-interference-graph',
    },
    # Just *after* `call 0x5b9b10` in _colorinstructions (0x1b9942 returns to
    # 0x1b9947): the colorer has written every node's physical register and its
    # priority-ordered assigned-node list is still on top of the stack (the
    # `pop ecx` is not until 0x1b994e).
    "regalloc_breakpoint_addr": 0x1B9947,
    "interferencegraph_addr": 0x37814C,  # _interferencegraph
    "used_virtual_registers_gpr_addr": 0x37D578,  # _used_virtual_registers+0x10
    "used_virtual_registers_fpr_addr": 0x37D574,  # _used_virtual_registers+0x0C
    "coloring_class_addr": 0x3994B2,  # _coloring_class
    "regalloc_assigned_stack": 0,
    "linkname_needs_call": False,
    "supports_ast": False,
    "supports_variables": False,
}

# Bytes at 0x6bef0 in the Wii/1.3 compiler: the `_CodeGen_Generator` prologue.
# Used as the detection probe together with the CodeView blob.
_WII13_SIGNATURE = bytes.fromhex("5356575583ec508b7c24648b47048b1f")

# (version, RVA, required bytes).  Upstream probed ten bytes at 0x541BBC and
# 0x58D224; the RVAs here are the same numbers minus the image base.
_PROBES = [
    ("GC/1.1", 0x141BBC, b"Metrowerks"),
    ("GC/2.6", 0x18D224, b"Metrowerks"),
    ("Wii/1.3", 0x6BEF0, _WII13_SIGNATURE),
]

# Data symbols that must agree with the Wii/1.3 row; checked at load time.
_WII13_SYMBOLS = {
    "gfunction_addr": "_gFunction",
    "cmangler_getlinkname_addr": "_CMangler_GetLinkName",
    "opcodeinfo_addr": "_opcodeinfo",
    "pcbasicblocks_addr": "_pcbasicblocks",
    "interferencegraph_addr": "_interferencegraph",
    "coloring_class_addr": "_coloring_class",
}

# Rows whose addresses are not the whole story: the record decoders differ, so
# the layout tag is data too.
_LAYOUT_FLAGS = {
    "GC/1.1": dict(
        layout="gc11",
        opcodeinfo_stride=0x10,
        supports_variables=True,
        coloring_classes={0: "GPR", 1: "FPR"},
    ),
    "GC/2.6": dict(
        layout="gc26",
        opcodeinfo_stride=0x12,
        supports_variables=False,
        coloring_classes={4: "GPR", 3: "FPR"},
    ),
}


def detect(exe):
    """Return a MwccVersion for a compiler binary, or None if it is not known."""
    exe = Path(exe)
    if not exe.exists():
        return None
    pe = Pe(exe)
    if pe.machine != MACHINE_I386:
        return None
    for name, rva, expected in _PROBES:
        if pe.read(rva, len(expected)) == expected:
            return build(name, exe)
    return None


def known():
    """Names this build knows, best-effort (detection is by byte signature)."""
    return [name for name, _rva, _b in _PROBES]


def row(name):
    """The raw row for a build name - RVAs, not VAs, and no compiler binary.

    `build()` starts from this and converts to VAs after checking the binary;
    locate/verify_pcode.py needs the breakpoint table on its own, because it is
    handed a dump and an object and has no executable to detect a build from.
    """
    if name == "Wii/1.3":
        return dict(_WII13_ROW)
    if name in _GC_ROWS:
        merged = dict(_GC_ROWS[name])
        merged.update(_LAYOUT_FLAGS[name])
        return merged
    raise KeyError(name)


def build(name, exe):
    """Instantiate a row and convert its RVAs to absolute VAs."""
    version = MwccVersion(name=name, **row(name))
    pe = Pe(exe)
    if version.image_base != pe.image_base:
        raise SystemExit(
            f"mwcc-debugger: the {name} version table assumes image base "
            f"{version.image_base:#x} but {pe.path} has {pe.image_base:#x}"
        )
    if name == "Wii/1.3":
        _verify_symbols(version, pe)
    return version.absolute()


def _verify_symbols(version, pe):
    """Re-check the Wii/1.3 row against the compiler's own symbol blob.

    A mismatch means the shipped table describes a different binary than the
    one being debugged, which would produce plausible-looking garbage - so this
    fails loudly rather than dumping nonsense.
    """
    syms = {name: rva for rva, name, _sec in pe.codeview_symbols()}
    if not syms:
        return
    problems = []
    for field, symbol in _WII13_SYMBOLS.items():
        want = getattr(version, field)
        got = syms.get(symbol)
        if got is None:
            problems.append(f"{symbol} is absent from the symbol blob")
        elif got != want:
            problems.append(f"{symbol} is at {got:#x}, the table says {want:#x}")
    if problems:
        raise SystemExit(
            "mwcc-debugger: the Wii/1.3 version table does not match "
            f"{pe.path}:\n  " + "\n  ".join(problems)
        )
