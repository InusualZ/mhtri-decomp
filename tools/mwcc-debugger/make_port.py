#!/usr/bin/env python3
"""Turn the vendored upstream mwcc_debugger.py into our Windows/Wii port.

This is the one-shot transformation used to produce tools/mwcc-debugger/
mwcc_debugger.py from the vendored upstream copy in tools/mwcc-debugger/
upstream/mwcc_debugger.py.  It is kept so the port's diff is reproducible and
reviewable: run it and diff the result against the committed file.

    python make_port.py upstream/mwcc_debugger.py mwcc_debugger.py

Every replacement below is anchored on text that must be present, so a change
in the upstream copy makes this script fail loudly instead of silently
producing a wrong port.
"""
from __future__ import annotations

import re
import sys


def sub_once(src: str, old: str, new: str, what: str) -> str:
    assert src.count(old) == 1, f"anchor {what!r} matched {src.count(old)} times"
    return src.replace(old, new)


HEADER_OLD = '''#!/usr/bin/env python3
from __future__ import annotations

import argparse
from collections import defaultdict, OrderedDict
from dataclasses import dataclass
from enum import Enum
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
from typing import Optional, Tuple

try:
    import gdb

    IN_GDB = True
except ImportError:
    IN_GDB = False
'''

HEADER_NEW = '''#!/usr/bin/env python3
"""mwcc_debugger.py - dump MWCC compiler internals while it compiles a file.

Windows-native port of cadmic/mwcc-debugger (see PROVENANCE.md for the upstream
commit and the list of changes).  Upstream runs the Windows compiler under
`retrowin32` - an emulator whose only purpose is to run a Windows x86 binary on
a POSIX host - and attaches gdb to its gdb stub.  On Windows the compiler
already runs natively, so the emulator is the part that goes away: this port
runs `gdb` directly on `mwcceppc.exe`.  The emulator path is still available
for POSIX hosts via `--emulator PATH`.

Two roles, as upstream:

  * started as `python mwcc_debugger.py ...` -> `start_gdb()`: work out which
    compiler build we were handed, sanity check the tool chain, then exec gdb
    with this same file as its command script.
  * sourced by gdb                             -> `run_compiler()`: set the
    breakpoints for that build and dump compiler state to text files.

Everything build-specific (addresses, table sizes, record layouts) lives in
versions.py; this file is mechanism only.

PORT: changes from upstream are marked with a `PORT:` comment.
"""
from __future__ import annotations

import argparse
from collections import defaultdict, OrderedDict
from dataclasses import dataclass
from enum import Enum
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import sys
import time
from typing import Optional, Tuple

try:
    import gdb

    IN_GDB = True
except ImportError:
    IN_GDB = False

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
# PORT: the version tables live in a data module (upstream hard-coded them
# inside init_mwcc_version()).  versions.py has no dependencies, so it loads
# both in the launcher and inside gdb's embedded Python.
import versions  # noqa: E402


def read_mem(addr: int, size: int) -> memoryview:
    """PORT: one read helper (upstream spelled read_memory out at each site)."""
    return gdb.selected_inferior().read_memory(addr, size)
'''

VERSION_OLD_RE = re.compile(
    r"@dataclass\nclass MwccVersion:.*?\n    print\(f\"MWCC version: \{MWCC_VERSION\.name\}\"\)\n",
    re.S,
)

VERSION_NEW = '''MWCC_VERSION: "versions.MwccVersion" = None
MWCC_EMULATOR: Optional[str] = None
MWCC_GDB_PORT: int = 9001
FUNCTION_NAME: str = ""
OUTPUT_DIR: str = ""
# PORT: for the native path - the compiler executable and its argument string,
# already escaped for gdb's own parser (see gdb_escape_path/_gdb_quote_args).
MWCC_EXE_GDB: str = ""
MWCC_ARGS_GDB: str = ""


def init_mwcc_version():
    """PORT: version identification is table-driven (versions.py) and happens
    in the *launcher*, from the PE file, before gdb is started.  Upstream
    sniffed ten bytes at a hard-coded address inside init_mwcc_version()."""
    global MWCC_VERSION
    if MWCC_VERSION is not None:
        return
    raw = os.environ.get("MWCC_DEBUGGER_PARAMS")
    if not raw:
        raise SystemExit(
            "mwcc-debugger: start this file through itself "
            "(python tools/mwcc-debugger/mwcc_debugger.py ...); "
            "MWCC_DEBUGGER_PARAMS is not set"
        )
    params = json.loads(raw)
    globals().update(
        FUNCTION_NAME=params["function_name"],
        OUTPUT_DIR=params["output_dir"],
        MWCC_EMULATOR=params.get("emulator"),
        MWCC_GDB_PORT=params.get("gdb_port", 9001),
        MWCC_EXE_GDB=params.get("gdb_file", ""),
        MWCC_ARGS_GDB=params.get("gdb_args", ""),
    )
    MWCC_VERSION = versions.build(params["version"], Path(params["exe"]))
    print(f"MWCC version: {MWCC_VERSION.name}")
'''

OBJ_OLD = '''    def load(cls, addr: int, load_linkname=False) -> MwccObject:
        if load_linkname:
'''
OBJ_NEW = '''    def load(cls, addr: int, load_linkname=False) -> MwccObject:
        if MWCC_VERSION.layout == "wii13":
            return cls._load_wii13(addr, load_linkname)
        if load_linkname:
'''

OBJ_TAIL_OLD = '''        return cls(
            name=name,
            stack_offset=stack_offset,
            size=size,
            linkname=linkname,
        )
'''
OBJ_TAIL_NEW = '''        return cls(
            name=name,
            stack_offset=stack_offset,
            size=size,
            linkname=linkname,
        )

    @classmethod
    def _load_wii13(cls, addr: int, load_linkname=False) -> MwccObject:
        """Wii/1.3 object record.

        PORT/RE: derived from `_CMangler_GetLinkName` (0x16f7f0) and from
        probing the live compiler (see locate/README.md).  The record is not
        the GC/1.1-2.6 shape: the source name is at +0x0C, the *mangled* name
        cache is at +0x48 (datatype 3/4 only) and is empty at the point
        CodeGen_Generator starts, which is why `load_linkname` is a no-op here
        (upstream's trick of calling CMangler_GetLinkName aborts this build
        with an internal error, CMangler.c:1128).

        The caller therefore matches functions by source name; for the plain C
        functions this decomp produces, source name == link name.
        """
        datatype = read_u8(addr + 0x2)
        name = read_string(read_u32(addr + 0x0C) + 0xA)
        linkname = None
        if load_linkname and datatype in (3, 4):
            mangled = read_u32(addr + 0x48)
            if mangled:
                linkname = read_string(mangled + 0xA)
        size = 0
        type_ = read_u32(addr + 0x24)
        if type_:
            size = read_s32(type_ + 0x2)
        return cls(name=name, stack_offset=0, size=size, linkname=linkname)
'''

PCODE_OLD = '''    def load(cls, addr: int) -> MwccPcode:
        if MWCC_VERSION.name == "GC/1.1":
'''
PCODE_NEW = '''    def load(cls, addr: int) -> MwccPcode:
        if MWCC_VERSION.layout == "wii13":
            return load_pcode_wii13(addr)
        if MWCC_VERSION.name == "GC/1.1":
'''

PCODE_HELPER = '''

# PORT/RE: Wii/1.3 PCode instruction record, derived from `_vformatpcode`
# (0x5ae160: `mov word ptr [eax+0x28], op`, `[eax+0x2a] = arg count`, the args
# at +0x2C with a 0x0E stride and a header of 0x2C bytes) and from
# `_appendpcode` (0x5a0ba0: next @0, prev @4, owning block @8).
WII13_PCODE_HEAD = 0x2C
WII13_PCODE_OP = 0x28
WII13_PCODE_ARG_COUNT = 0x2A
WII13_PCODE_ARGS = 0x2C
WII13_ARG_SIZE = 0x0E


def decode_arg_wii13(kind_byte: int, regclass: int, field2: int,
                     field4: int) -> MwccPCodeArg:
    """One Wii/1.3 PCode operand.

    PORT/RE: the 0x0E-byte record, read off a live dump of what
    `_vformatpcode` (0x5ae160) produces:

        +0x0  kind    0 = register, 1/2 = immediate, 4 = fixup,
                      6 = label, 8 = placeholder
        +0x1  class   register file, only meaningful when kind == 0
        +0x2  u16     operand value for kinds 1/2/4/6; use/def flags for a
                      register (1 = use, 2 = def)
        +0x4  s16     register number (virtual, >= 32, before regalloc); for
                      kind 6 a label id

    The register-file numbers are the compiler's own enum, the same one the
    colouring loop uses (4 = GPR, 3 = FPR), plus 1 = condition-register field
    and 0 = a special register (1 = LR, 2 = CTR, attested by MFLR/MTCTR).
    Anything not established is printed raw instead of guessed.
    """
    if kind_byte == 0:
        if regclass == 4:
            return MwccPCodeArg(MwccPCodeArg.Kind.GPR, reg=field4 & 0xFFFF)
        if regclass == 3:
            return MwccPCodeArg(MwccPCodeArg.Kind.FPR, reg=field4 & 0xFFFF)
        if regclass == 1:
            return MwccPCodeArg(MwccPCodeArg.Kind.CRFIELD, reg=field4 & 0xFFFF)
        if regclass == 0:
            special = {1: "lr", 2: "ctr"}.get(field4)
            return MwccPCodeArg(special or f"spr{field4}")
        return MwccPCodeArg(f"k0.{regclass}:{field4}")
    if kind_byte in (1, 2):
        value = field2
        if value >= 0x8000:
            value -= 0x10000
        return MwccPCodeArg(MwccPCodeArg.Kind.IMMEDIATE, imm=value)
    if kind_byte == 8:
        return MwccPCodeArg(MwccPCodeArg.Kind.PLACEHOLDER)
    if kind_byte == 6:
        # A branch target.  Resolving it needs the block's label list, which
        # is not derived yet, so show the record's label id.
        return MwccPCodeArg(f"@L{field4 & 0xFFFF}")
    if kind_byte == 4:
        # A relocation the assembler patches later (TOC offset, call address);
        # the record names the fixup kind in +2.
        return MwccPCodeArg(f"fixup[{field2:#06x}]")
    return MwccPCodeArg(f"k{kind_byte}.{regclass}:{field4}")


def load_pcode_wii13(addr: int) -> MwccPcode:
    mem = read_mem(addr, WII13_PCODE_HEAD)
    next_addr = parse_u32(mem, 0x0)
    op = parse_u16(mem, WII13_PCODE_OP)
    arg_count = parse_u16(mem, WII13_PCODE_ARG_COUNT)
    args = []
    if 0 < arg_count <= 24:
        arg_mem = read_mem(addr + WII13_PCODE_ARGS, arg_count * WII13_ARG_SIZE)
        for i in range(arg_count):
            off = i * WII13_ARG_SIZE
            args.append(
                decode_arg_wii13(
                    parse_u8(arg_mem, off + 0x0),
                    parse_u8(arg_mem, off + 0x1),
                    parse_u16(arg_mem, off + 0x2),
                    parse_s16(arg_mem, off + 0x4),
                )
            )
    return MwccPcode(next_addr=next_addr, line=None, op=op, args=args)
'''

BLOCK_OLD = '''    def load(cls, addr: int) -> MwccBlock:
        if MWCC_VERSION.name == "GC/1.1":
'''
BLOCK_NEW = '''    def load(cls, addr: int) -> MwccBlock:
        if MWCC_VERSION.layout == "wii13":
            return load_block_wii13(addr)
        if MWCC_VERSION.name == "GC/1.1":
'''

BLOCK_HELPER = '''

# PORT/RE: Wii/1.3 basic-block record.  `_makepcblock` (0x5a0e20) allocates
# 0x2E bytes and stores the current source line at +0x24 and the block index at
# +0x1C; `_appendpcode` (0x5a0ba0) keeps the instruction list head at +0x14,
# the tail at +0x18 and bumps the instruction count at +0x28.  The successor /
# predecessor / label fields are *not* derived yet (see locate/README.md), so
# the dump says so rather than printing zeros.
def load_block_wii13(addr: int) -> MwccBlock:
    mem = read_mem(addr, 0x2E)
    line = parse_s32(mem, 0x24)
    if line == -1:
        line = None
    return MwccBlock(
        next_addr=parse_u32(mem, 0x0),
        prev_addr=0,
        label_addr=0,
        predecessors_addr=0,
        successors_addr=0,
        instr_addr=parse_u32(mem, 0x14),
        index=parse_s32(mem, 0x1C),
        line=line,
        loop_weight=0,
        pcode_count=parse_u16(mem, 0x28),
        flags=0,
    )
'''

PRINT_BLOCK_OLD = '''def print_block(f, block: MwccBlock):
    print(
'''
PRINT_BLOCK_NEW = '''def print_block(f, block: MwccBlock):
    if MWCC_VERSION.layout == "wii13":
        print_block_wii13(f, block)
        return
    print(
'''

PRINT_BLOCK_HELPER = '''

def print_block_wii13(f, block: MwccBlock):
    """PORT: keep the instruction stream; say plainly what is not derived."""
    print(f"B{block.index}: ", end="", file=f)
    line_str = f"line={block.line} " if block.line is not None else ""
    print(
        f"{line_str}pcode={block.pcode_count} "
        "(successors/predecessors/labels not dumped: record layout not derived)",
        file=f,
    )
    instr_addr = block.instr_addr
    count = 0
    while instr_addr != 0 and count < 20000:
        instr = MwccPcode.load(instr_addr)
        print_instruction(f, instr, block.line)
        instr_addr = instr.next_addr
        count += 1
    if count >= 20000:
        print("  ... (instruction chain did not terminate)", file=f)
    print("", file=f)
'''

IGNODE_OLD = '''    def load(cls, addr: int) -> MwccIGNode:
        if MWCC_VERSION.name == "GC/1.1":
'''
IGNODE_NEW = '''    def load(cls, addr: int) -> MwccIGNode:
        if MWCC_VERSION.layout == "wii13":
            return load_ignode_wii13(addr)
        if MWCC_VERSION.name == "GC/1.1":
'''

IGNODE_HELPER = '''

# PORT/RE: Wii/1.3 interference-graph node.  The graph is a *flat array* of
# 0x20-byte records indexed by virtual register (`[0x77814c] + reg*0x20` in
# `_colorinstructions`/`_pickcolor`), not an array of pointers as in the GC
# builds.  Fields: next @0, object @4, cost @0xC, virtual reg @0x10, physical
# reg @0x12, flags @0x14, degree @0x16, second counter @0x18, interference
# pair-list head @0x1C.
WII13_IGNODE_STRIDE = 0x20


def load_ignode_wii13(addr: int) -> MwccIGNode:
    mem = read_mem(addr, WII13_IGNODE_STRIDE)
    virtual_reg = parse_s16(mem, 0x10)
    physical_reg = parse_s16(mem, 0x12)
    flags_value = parse_u16(mem, 0x14)
    return MwccIGNode(
        next_addr=parse_u32(mem, 0x0),
        virtual_reg=virtual_reg,
        physical_reg=physical_reg,
        cost=parse_s32(mem, 0xC),
        flags=_ignode_flags(flags_value),
        obj_addr=parse_u32(mem, 0x4),
        neighbors=neighbors_wii13(addr, virtual_reg),
    )


def _ignode_flags(flags_value: int) -> list:
    flags = []
    if flags_value & 0x01:
        flags.append(MwccIGNode.Flag.fSpilled)
    if flags_value & 0x04:
        flags.append(MwccIGNode.Flag.fCoalesced)
    if flags_value & 0x08:
        flags.append(MwccIGNode.Flag.fCoalescedInto)
    if flags_value & 0x10:
        flags.append(MwccIGNode.Flag.fPairHigh)
    if flags_value & 0x20:
        flags.append(MwccIGNode.Flag.fPairLow)
    return flags


def neighbors_wii13(node_addr: int, virtual_reg: int) -> list:
    """Interference neighbours of a Wii/1.3 node.

    `_makeinterfere` (0x23fc60) threads every interference pair into *two*
    singly-linked lists at once: the pair record's +4 is the next pointer for
    endpoint A and +8 the next pointer for endpoint B.  `_pickcolor`'s caller
    (0x5b9bc0) walks them by comparing the node's own virtual register against
    the record's second endpoint: if own >= b it takes `a` and follows +8,
    otherwise it takes `b` and follows +4.
    """
    own = virtual_reg & 0xFFFF
    out = []
    rec = read_u32(node_addr + 0x1C)
    seen = 0
    while rec != 0 and seen < 4096:
        pair = read_mem(rec, 0x10)
        a = parse_u16(pair, 0xC)
        b = parse_u16(pair, 0xE)
        if own >= b:
            out.append(a)
            rec = parse_u32(pair, 0x8)
        else:
            out.append(b)
            rec = parse_u32(pair, 0x4)
        seen += 1
    return out
'''

OPCODE_OLD = '''    if MWCC_VERSION.name == "GC/1.1":
        size = 0x10
    elif MWCC_VERSION.name == "GC/2.6":
        size = 0x12
    else:
        raise ValueError(f"Unsupported MWCC version: {MWCC_VERSION.name}")
'''
OPCODE_NEW = '''    # PORT: the entry size is version data, not code (0x10 for GC/1.1, 0x12
    # for GC/2.6, 0x16 for Wii/1.3 where each entry also carries the encoder's
    # instruction word).
    size = MWCC_VERSION.opcodeinfo_stride
'''

GRAPH_OLD = '''    # Read all interference graph nodes
    graph_addr = read_u32(MWCC_VERSION.interferencegraph_addr)
    nodes = OrderedDict()
    if num_regs > 0:
        mem = gdb.selected_inferior().read_memory(graph_addr, num_regs * 0x4)
        for i in range(32, num_regs):
            node_addr = parse_u32(mem, i * 0x4)
            if node_addr == 0:
                continue
            node = MwccIGNode.load(node_addr)
'''
GRAPH_NEW = '''    # Read all interference graph nodes
    graph_addr = read_u32(MWCC_VERSION.interferencegraph_addr)
    nodes = OrderedDict()
    if num_regs > 0:
        if MWCC_VERSION.layout == "wii13":
            # PORT: flat array of records, not an array of pointers.
            addresses = [
                graph_addr + i * WII13_IGNODE_STRIDE for i in range(32, num_regs)
            ]
        else:
            mem = gdb.selected_inferior().read_memory(graph_addr, num_regs * 0x4)
            addresses = [parse_u32(mem, i * 0x4) for i in range(32, num_regs)]
        for i, node_addr in zip(range(32, num_regs), addresses):
            if node_addr == 0:
                continue
            node = MwccIGNode.load(node_addr)
            nodes[node_addr] = node
            i = i  # noqa: keep the index available for debug output
'''
GRAPH_TAIL_OLD = '''            nodes[node_addr] = node
            if node.obj_addr != 0:
'''
GRAPH_TAIL_NEW = '''            if node.obj_addr != 0:
'''

ASSIGNED_OLD = '''    sp = int(gdb.parse_and_eval("$esp"))
    if MWCC_VERSION.name == "GC/1.1":
        # For GC/1.1, coloring class is first argument, assigned variables is second argument
        coloring_class = read_u32(sp + 0x4)
        assigned_nodes_addr = read_u32(sp + 0x8)
        if coloring_class == 0:
            reg_type = RegType.GPR
        elif coloring_class == 1:
            reg_type = RegType.FPR
        else:
            raise ValueError(f"Unexpected coloring class: {coloring_class}")
    else:
        # For other versions, coloring class is in a global variable, assigned variables is first argument
        coloring_class = read_u8(MWCC_VERSION.coloring_class_addr)
        assigned_nodes_addr = read_u32(sp + 0x4)
        if coloring_class == 4:
            reg_type = RegType.GPR
        elif coloring_class == 3:
            reg_type = RegType.FPR
        else:
            raise ValueError(f"Unexpected coloring class: {coloring_class}")
'''
ASSIGNED_NEW = '''    sp = int(gdb.parse_and_eval("$esp"))
    if MWCC_VERSION.name == "GC/1.1":
        # For GC/1.1, coloring class is first argument, assigned variables is second argument
        coloring_class = read_u32(sp + 0x4)
        assigned_nodes_addr = read_u32(sp + 0x8)
    else:
        # For other versions, coloring class is in a global variable, assigned
        # variables is the first argument.
        coloring_class = read_u8(MWCC_VERSION.coloring_class_addr)
        # PORT: *which* stack slot holds the assigned list is build data.  The
        # GC builds leave it at [esp+4]; Wii/1.3's _colorinstructions (0x1b9941)
        # pushes the priority list right before the call into the colorer, so
        # the breakpoint there has it at [esp+0].
        assigned_nodes_addr = read_u32(sp + MWCC_VERSION.regalloc_assigned_stack)
    # PORT: which class number means GPR and which FPR is build data.  The
    # compiler colours five register classes; this tool models the two upstream
    # had and says so instead of raising on the others.
    kind = MWCC_VERSION.coloring_classes.get(coloring_class)
    if kind is None:
        print(
            f"regalloc: skipping coloring class {coloring_class} "
            "(not modelled by this tool)"
        )
        return
    reg_type = RegType[kind]
'''

FIND_FUNC_OLD = '''        func = find_current_function()
        if func.linkname == FUNCTION_NAME:
            break
        print(f"Skipping function {func.linkname}")
'''
FIND_FUNC_NEW = '''        func = find_current_function()
        # PORT: match the mangled name when the build exposes one, else the
        # source name (Wii/1.3 - see MwccObject._load_wii13).
        if FUNCTION_NAME in (func.linkname, func.name):
            break
        print(f"Skipping function {func.linkname or func.name}")
'''

FORMAT_OPERANDS_OLD = '''def format_operands(instr) -> str:
    out = ""
    arg_count = len(instr.args)
    for i in range(min(arg_count, 6)):
        arg = instr.args[i]

        if arg.kind == MwccPCodeArg.Kind.PLACEHOLDER:
            continue

        if i != 0:
            out += ","

        if arg.kind == MwccPCodeArg.Kind.GPR:
            out += f"r{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.FPR:
            out += f"f{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.SPR:
            if arg.reg == 0:
                out += "zero"
            elif arg.reg == 1:
                out += "ctr"
            elif arg.reg == 2:
                out += "lr"
            else:
                out += f"spr{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.CRFIELD:
            out += f"cr{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.VR:
            out += f"vr{arg.reg}"
        elif arg.kind in (MwccPCodeArg.Kind.IMMEDIATE, MwccPCodeArg.Kind.MEMORY):
            if arg.imm < 0:
                out += f"-0x{-arg.imm:x}"
            elif arg.imm < 10:
                out += str(arg.imm)
            else:
                out += f"0x{arg.imm:x}"
            if arg.obj_addr != 0:
                obj = MwccObject.load(arg.obj_addr)
                out += f"({obj.name})"
'''
FORMAT_OPERANDS_NEW = '''def format_operands(instr) -> str:
    out = ""
    arg_count = len(instr.args)
    for i in range(min(arg_count, 6)):
        arg = instr.args[i]

        if arg.kind == MwccPCodeArg.Kind.PLACEHOLDER:
            continue

        # PORT: Wii/1.3 operands whose kind is not derived yet are carried as a
        # raw marker string so the dump shows them instead of raising.
        if isinstance(arg.kind, str):
            if i != 0:
                out += ","
            out += arg.kind
            continue

        if i != 0:
            out += ","

        if arg.kind == MwccPCodeArg.Kind.GPR:
            out += f"r{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.FPR:
            out += f"f{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.SPR:
            if arg.reg == 0:
                out += "zero"
            elif arg.reg == 1:
                out += "ctr"
            elif arg.reg == 2:
                out += "lr"
            else:
                out += f"spr{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.CRFIELD:
            out += f"cr{arg.reg}"
        elif arg.kind == MwccPCodeArg.Kind.VR:
            out += f"vr{arg.reg}"
        elif arg.kind in (MwccPCodeArg.Kind.IMMEDIATE, MwccPCodeArg.Kind.MEMORY):
            if arg.imm < 0:
                out += f"-0x{-arg.imm:x}"
            elif arg.imm < 10:
                out += str(arg.imm)
            else:
                out += f"0x{arg.imm:x}"
            # PORT: truthiness, not `!= 0` - a Wii operand that carries no
            # object leaves the field unset (None) rather than 0.
            if arg.obj_addr:
                obj = MwccObject.load(arg.obj_addr)
                out += f"({obj.name})"
'''


VARIABLES_OLD = '''def print_variables():
    if not MWCC_VERSION.name == "GC/1.1":
        # TODO: implement other versions
        return
'''
VARIABLES_NEW = '''def print_variables():
    # PORT: upstream supported this for GC/1.1 only; make that a property of
    # the version table so adding a build is data.
    if not MWCC_VERSION.supports_variables:
        print("variables.txt: not supported for this compiler build - skipped")
        return
'''

RUN_OLD_RE = re.compile(r"\ndef run_compiler\(\):.*\Z", re.S)

RUN_NEW = '''

def run_compiler():
    """gdb-side entry point (this file sourced as a gdb command script)."""
    gdb.execute("set python print-stack full")
    gdb.execute("set confirm off")
    gdb.execute("set pagination off")

    # PORT: identify the build first - the native path needs the executable
    # path and argument string that come with it.  (Upstream sniffed the
    # version from inferior memory, so it had to happen after `target remote`.)
    init_mwcc_version()

    if MWCC_EMULATOR:
        # Upstream behaviour: attach to the retrowin32 gdb stub.
        gdb.execute("set architecture i386")
        gdb.execute("set osabi none")
        gdb.execute(f"target remote localhost:{MWCC_GDB_PORT}")
    else:
        # PORT: native Windows.  `file`/`set args` are issued *here* rather
        # than on the gdb command line, because gdb runs `-x` scripts at an
        # order that is not guaranteed relative to `-ex` and because
        # `startup-with-shell` is a target setting that does not exist until
        # an executable has been loaded.  We must keep the shell out of the
        # launch or cmd.exe would re-split the -pragma "cats off" arguments.
        gdb.execute("file " + MWCC_EXE_GDB)
        gdb.execute("set args " + MWCC_ARGS_GDB)
        try:
            gdb.execute("set startup-with-shell off")
        except gdb.error:
            pass

    # PORT: the node-name and opcode tables are read out of the *inferior's*
    # memory, so they can only be read once the process exists.  Upstream got
    # them here because its `target remote` had already started the emulator;
    # natively the tables are loaded after the first stop, below.
    tables_loaded = False

    gdb.execute(f"break *{MWCC_VERSION.codegen_start_addr:#x}")

    first = True
    func = None
    while True:
        if first and not MWCC_EMULATOR:
            gdb.execute("run")
            # The image is only resident once the inferior has started.
            check_native_image()
        else:
            gdb.execute("continue")
        first = False
        if not tables_loaded:
            load_node_names()
            load_opcode_info()
            tables_loaded = True
        func = find_current_function()
        if FUNCTION_NAME in (func.linkname, func.name):
            break
        print(f"Skipping function {func.linkname or func.name}")

    print(f"Found function {func.linkname or func.name}")
    print()

    # Set breakpoints
    for addr in MWCC_VERSION.ast_breakpoints:
        gdb.execute(f"break *{addr:#x}")
    for addr in MWCC_VERSION.pcode_breakpoints:
        gdb.execute(f"break *{addr:#x}")
    if MWCC_VERSION.regalloc_breakpoint_addr:
        gdb.execute(f"break *{MWCC_VERSION.regalloc_breakpoint_addr:#x}")
    gdb.execute(f"break *{MWCC_VERSION.codegen_end_addr:#x}")

    # Loop through breakpoints
    frontend_pass_number = 0
    backend_pass_number = 0
    while True:
        gdb.execute("continue")
        current_addr = int(gdb.parse_and_eval("$pc"))

        if current_addr in MWCC_VERSION.ast_breakpoints:
            pass_name = MWCC_VERSION.ast_breakpoints[current_addr]
            if not MWCC_VERSION.supports_ast:
                print(
                    f"AST dump skipped at {current_addr:#x} ({pass_name}): the "
                    "frontend record layout for this compiler build is not "
                    "derived (see locate/README.md)"
                )
            else:
                sp = int(gdb.parse_and_eval("$esp"))
                statement_addr = read_u32(sp)
                print_ast(statement_addr, frontend_pass_number, pass_name)
                frontend_pass_number += 1

        if current_addr in MWCC_VERSION.pcode_breakpoints:
            pass_name = MWCC_VERSION.pcode_breakpoints[current_addr]
            print_pcode(backend_pass_number, pass_name)
            backend_pass_number += 1

        if (
            MWCC_VERSION.regalloc_breakpoint_addr
            and current_addr == MWCC_VERSION.regalloc_breakpoint_addr
        ):
            print_regalloc()

        if current_addr == MWCC_VERSION.codegen_end_addr:
            print_variables()
            gdb.execute("quit")


def check_native_image():
    """PORT: fail with a clear message instead of dumping garbage.

    Every address in versions.py is image-relative, so the compiler must have
    loaded at its PE ImageBase.  The shipped mwcceppc.exe files have
    DllCharacteristics == 0 (no DYNAMIC_BASE), so they always do; this check is
    here so that a hypothetical relocation shows up as one clear sentence.
    """
    base = MWCC_VERSION.image_base
    try:
        head = bytes(gdb.selected_inferior().read_memory(base, 2))
    except gdb.error:
        head = b""
    if head != b"MZ":
        raise SystemExit(
            f"mwcc-debugger: expected an MZ image at {base:#x} (the PE "
            f"ImageBase the version table is relative to), found {head!r}.\\n"
            "  The compiler was relocated or a different binary is loaded, so "
            "every breakpoint address would be wrong.  Re-derive the table "
            "with locate/dissect.py or disable ASLR for the process."
        )


# ---------------------------------------------------------------------------
# Launcher (PORT: rewritten - native gdb by default, retrowin32 kept on request)
# ---------------------------------------------------------------------------


def split_command_line(text: str) -> list:
    """Split a compiler command line, Windows-style.

    PORT: POSIX `shlex.split` eats the backslashes of Windows paths, turning
    `src\\\\fn_8004C9A0.cpp` into `srcfn_8004C9A0.cpp`.  We split on whitespace,
    keep quoted runs together and leave every backslash alone.
    """
    out = []
    cur = ""
    quote = None
    started = False
    for ch in text:
        if quote:
            if ch == quote:
                quote = None
            else:
                cur += ch
            continue
        if ch in "\\"'":
            quote = ch
            started = True
            continue
        if ch.isspace():
            if started:
                out.append(cur)
                cur = ""
                started = False
            continue
        cur += ch
        started = True
    if started:
        out.append(cur)
    return out


def safe_dir_name(name: str) -> str:
    """PORT: mangled C++ names contain ?@<>:* which Windows rejects in a path."""
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name)[:120] or "function"


def find_gdb(explicit: Optional[str]) -> str:
    """PORT: resolve gdb, and explain how to install one if it is missing."""
    if explicit:
        if Path(explicit).exists():
            return explicit
        raise SystemExit(f"mwcc-debugger: --gdb {explicit!r} does not exist")
    found = shutil.which("gdb") or shutil.which("gdb.exe")
    if found:
        return found
    candidates = [
        Path("/mingw64/bin/gdb.exe"),
        Path(r"C:\\msys64\\mingw64\\bin\\gdb.exe"),
        Path(r"C:\\msys64\\usr\\bin\\gdb.exe"),
    ]
    for cand in candidates:
        try:
            if cand.exists():
                return str(cand)
        except OSError:
            continue
    raise SystemExit(
        "mwcc-debugger: no gdb found.  Windows needs a *native* mingw-w64 gdb\\n"
        "  (a Cygwin/MSYS gdb cannot debug a native mwcceppc.exe).  Either:\\n"
        "    * MSYS2:      pacman -S mingw-w64-x86_64-gdb\\n"
        "                  then --gdb C:\\\\msys64\\\\mingw64\\\\bin\\\\gdb.exe\\n"
        "    * no MSYS2:   python tools/mwcc-debugger/fetch_gdb.py --dest DIR\\n"
        "                  (downloads the MSYS2 package plus its runtime DLLs)\\n"
        "  or put the gdb on PATH."
    )


def gdb_escape_path(path: str) -> str:
    """Escape a path for gdb's command parser (backslash is an escape char).

    Forward slashes are accepted by gdb on Windows and dodge the whole problem,
    so use them and only quote if something odd is left.
    """
    posix = path.replace("\\\\", "/")
    if any(c.isspace() for c in posix):
        return '"' + posix + '"'
    return posix


def gdb_quote_args(argv: list) -> str:
    """Quote a compiler argument list for gdb's `set args` parser (which
    processes backslash escapes inside double quotes)."""
    out = []
    for a in argv:
        a = a.replace("\\\\", "\\\\\\\\").replace('"', '\\\\"')
        if a == "" or any(c.isspace() for c in a):
            out.append(f'"{a}"')
        else:
            out.append(a)
    return " ".join(out)


def start_gdb():
    parser = argparse.ArgumentParser(
        description="Dump MWCC compiler internals while compiling a file."
    )
    parser.add_argument(
        "--args",
        "-a",
        required=True,
        help="compiler command line (in quotes), starting with mwcceppc.exe",
    )
    parser.add_argument(
        "--exe",
        "-E",
        default=None,
        help="path to mwcceppc.exe (default: first word of --args)",
    )
    parser.add_argument(
        "--emulator",
        "-e",
        default=None,
        help="POSIX hosts only: run the compiler under this retrowin32 binary "
        "(upstream behaviour) instead of running it natively",
    )
    parser.add_argument(
        "--gdb",
        "-g",
        default=None,
        help="path to the gdb to use (default: gdb on PATH)",
    )
    parser.add_argument(
        "--gdb-port",
        type=int,
        default=9001,
        help="port of the retrowin32 gdb stub (default: 9001)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=900.0,
        help="seconds to allow the whole debug session (default: 900)",
    )
    parser.add_argument("FUNCTION_NAME", help="name of the function to analyze")
    parser.add_argument(
        "OUTPUT_DIR",
        nargs="?",
        help="output directory for the dump files "
        "(default: debug-FUNCTION_NAME, sanitized)",
    )
    args = parser.parse_args()

    output_dir = args.OUTPUT_DIR or f"debug-{safe_dir_name(args.FUNCTION_NAME)}"

    compiler_argv = split_command_line(args.args)
    if not compiler_argv:
        raise SystemExit("mwcc-debugger: --args is empty")
    if args.exe:
        compiler_argv[0] = args.exe
    exe_path = Path(compiler_argv[0])
    if not exe_path.exists():
        raise SystemExit(
            f"mwcc-debugger: compiler {compiler_argv[0]!r} not found.  Pass the "
            "path to mwcceppc.exe as the first word of --args (or with --exe)."
        )

    info = versions.detect(exe_path)
    if info is None:
        raise SystemExit(
            f"mwcc-debugger: {exe_path} is not a compiler build this tool "
            "knows.  Known builds: "
            + ", ".join(versions.known())
            + ".  Add one in tools/mwcc-debugger/versions.py; "
            "locate/README.md records how the Wii/1.3 entry was derived."
        )
    print(f"Detected MWCC version: {info.name}", file=sys.stderr)

    os.makedirs(output_dir, exist_ok=True)
    gdb_path = find_gdb(args.gdb)

    params = {
        "function_name": args.FUNCTION_NAME,
        "output_dir": str(Path(output_dir).resolve()),
        "version": info.name,
        "exe": str(exe_path.resolve()),
        "emulator": args.emulator,
        "gdb_port": args.gdb_port,
        "gdb_file": gdb_escape_path(str(exe_path.resolve())),
        "gdb_args": gdb_quote_args(compiler_argv[1:]),
    }
    env = dict(os.environ)
    # PORT: state crosses into gdb through the environment, not through a
    # `-ex py ...` string, so paths with quotes/spaces cannot break it.
    env["MWCC_DEBUGGER_PARAMS"] = json.dumps(params)

    gdb_command = [
        gdb_path,
        "-batch",
        "-nx",
        "-ex",
        "set python print-stack full",
    ]

    emulator_process = None
    if args.emulator:
        emulator_command = [args.emulator, "--gdb-stub", *compiler_argv]
        print(f"Emulator command: {shlex.join(emulator_command)}", file=sys.stderr)
        emulator_process = subprocess.Popen(emulator_command)

    gdb_command += ["-x", str(Path(__file__).resolve())]
    print("GDB command: " + " ".join(gdb_command), file=sys.stderr)

    try:
        gdb_result = subprocess.run(
            gdb_command, check=False, env=env, timeout=args.timeout
        )
    except subprocess.TimeoutExpired:
        if emulator_process is not None:
            emulator_process.kill()
        raise SystemExit(
            f"mwcc-debugger: the debug session did not finish within "
            f"{args.timeout:.0f}s (raise --timeout)"
        )
    finally:
        if emulator_process is not None:
            try:
                emulator_process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                emulator_process.kill()

    if gdb_result.returncode != 0:
        raise SystemExit(
            f"mwcc-debugger: gdb exited with code {gdb_result.returncode}"
        )
    print(f"Dumps written to {output_dir}")


if __name__ == "__main__":
    # If we're running under GDB, proceed with the GDB script. If we're not,
    # run start_gdb() to invoke ourselves.
    if IN_GDB:
        run_compiler()
    else:
        start_gdb()
'''


def main():
    src = open(sys.argv[1], encoding="utf-8").read()

    src = sub_once(src, HEADER_OLD, HEADER_NEW, "header")
    src, n = VERSION_OLD_RE.subn(lambda m: VERSION_NEW, src)
    assert n == 1, f"version table region matched {n} times"

    src = sub_once(src, OBJ_OLD, OBJ_NEW, "object load")
    src = sub_once(src, OBJ_TAIL_OLD, OBJ_TAIL_NEW, "object tail")
    src = sub_once(src, "MWCC_OPCODE_INFO: list[MwccOpcodeInfo] = []",
                   PCODE_HELPER.strip() + "\n\n\nMWCC_OPCODE_INFO: list[MwccOpcodeInfo] = []",
                   "pcode helper")
    src = sub_once(src, PCODE_OLD, PCODE_NEW, "pcode load")
    src = sub_once(src, BLOCK_OLD, BLOCK_NEW, "block load")
    src = sub_once(src, "def print_block(f, block: MwccBlock):",
                   BLOCK_HELPER.strip() + "\n\n\ndef print_block(f, block: MwccBlock):",
                   "block helper")
    src = sub_once(src, PRINT_BLOCK_OLD, PRINT_BLOCK_NEW, "print_block")
    src = sub_once(src, "def print_pcode(pass_number: int, pass_name: str):",
                   PRINT_BLOCK_HELPER.strip() + "\n\n\ndef print_pcode(pass_number: int, pass_name: str):",
                   "print_block helper")
    src = sub_once(src, IGNODE_OLD, IGNODE_NEW, "ignode load")
    src = sub_once(src, "MWCC_OPCODE_INFO: list[MwccOpcodeInfo] = []\n\n\ndef load_opcode_info",
                   IGNODE_HELPER.strip() + "\n\n\nMWCC_OPCODE_INFO: list[MwccOpcodeInfo] = []\n\n\ndef load_opcode_info",
                   "ignode helper")
    src = sub_once(src, OPCODE_OLD, OPCODE_NEW, "opcode stride")
    src = sub_once(src, GRAPH_OLD, GRAPH_NEW, "graph nodes")
    src = sub_once(src, GRAPH_TAIL_OLD, GRAPH_TAIL_NEW, "graph tail")
    src = sub_once(src, ASSIGNED_OLD, ASSIGNED_NEW, "assigned list")
    src = sub_once(src, FIND_FUNC_OLD, FIND_FUNC_NEW, "function match")
    src = sub_once(src, VARIABLES_OLD, VARIABLES_NEW, "variables gate")
    src = sub_once(src, FORMAT_OPERANDS_OLD, FORMAT_OPERANDS_NEW, "operands")
    src, n = RUN_OLD_RE.subn(lambda m: RUN_NEW, src)
    assert n == 1, f"run_compiler region matched {n} times"

    open(sys.argv[2], "w", encoding="utf-8", newline="\n").write(src)
    print(f"wrote {sys.argv[2]} ({len(src.splitlines())} lines)")


if __name__ == "__main__":
    main()
