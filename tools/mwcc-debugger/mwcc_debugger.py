#!/usr/bin/env python3
"""Dump MWCC compiler internals while it compiles a file: the Windows-native port of cadmic/mwcc-debugger.
Spec: docs/tools/spec/mwcc-debugger.md. CLI: mwcc_debugger.py -a ARGS [--exe EXE] [-e EMULATOR] [-g GDB] [--gdb-port P] [--timeout S] FUNCTION [OUTPUT_DIR]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

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
# PORT: the version tables live in a data module (upstream hard-coded them
# inside init_mwcc_version()).  versions.py depends only on the repository's
# tools/lib (the prologue above puts the root on sys.path), so it loads both
# in the launcher and inside gdb's embedded Python; `tools/mwcc-debugger/` is
# not an identifier but is a namespace package of `tools`, so it is imported by name.
import importlib  # noqa: E402

versions = importlib.import_module("tools.mwcc-debugger.versions")
from tools.lib import proc as lib_proc  # noqa: E402


def read_mem(addr: int, size: int) -> memoryview:
    """PORT: one read helper (upstream spelled read_memory out at each site)."""
    return gdb.selected_inferior().read_memory(addr, size)


# Helpers for parsing structs
def parse_s8(mem: memoryview, offset: int) -> int:
    return int.from_bytes(mem[offset : offset + 1], "little", signed=True)


def parse_u8(mem: memoryview, offset: int) -> int:
    return int.from_bytes(mem[offset : offset + 1], "little", signed=False)


def parse_s16(mem: memoryview, offset: int) -> int:
    return int.from_bytes(mem[offset : offset + 2], "little", signed=True)


def parse_u16(mem: memoryview, offset: int) -> int:
    return int.from_bytes(mem[offset : offset + 2], "little", signed=False)


def parse_s32(mem: memoryview, offset: int) -> int:
    return int.from_bytes(mem[offset : offset + 4], "little", signed=True)


def parse_u32(mem: memoryview, offset: int) -> int:
    return int.from_bytes(mem[offset : offset + 4], "little", signed=False)


def parse_f32(mem: memoryview, offset: int) -> float:
    return struct.unpack("<f", mem[offset : offset + 4])[0]


def parse_f64(mem: memoryview, offset: int) -> float:
    return struct.unpack("<d", mem[offset : offset + 8])[0]


# Helpers for reading memory
def read_s8(addr: int) -> int:
    return parse_s8(gdb.selected_inferior().read_memory(addr, 1), 0)


def read_u8(addr: int) -> int:
    return parse_u8(gdb.selected_inferior().read_memory(addr, 1), 0)


def read_s16(addr: int) -> int:
    return parse_s16(gdb.selected_inferior().read_memory(addr, 2), 0)


def read_u16(addr: int) -> int:
    return parse_u16(gdb.selected_inferior().read_memory(addr, 2), 0)


def read_s32(addr: int) -> int:
    return parse_s32(gdb.selected_inferior().read_memory(addr, 4), 0)


def read_u32(addr: int) -> int:
    return parse_u32(gdb.selected_inferior().read_memory(addr, 4), 0)


# Read a C string
def read_string(addr: int) -> str:
    # TODO: We should probably use raw bytes and not a Unicode string
    return gdb.Value(addr).cast(gdb.lookup_type("char").pointer()).string("latin-1")


MWCC_VERSION: "versions.MwccVersion" = None
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


@dataclass
class MwccObject:
    name: str
    stack_offset: int
    size: int
    linkname: Optional[str]

    @classmethod
    def load(cls, addr: int, load_linkname=False) -> MwccObject:
        if MWCC_VERSION.layout == "wii13":
            return cls._load_wii13(addr, load_linkname)
        if load_linkname:
            # Force the linkname to be evaluated by calling CMangler_GetLinkName. Hopefully this
            # doesn't cause any side effects.
            gdb.execute(
                f"call ((void (*) (void *)) {MWCC_VERSION.cmangler_getlinkname_addr:#x})({addr:#x})"
            )
        mem = gdb.selected_inferior().read_memory(addr, 0x36)
        datatype = parse_u8(mem, 0x2)
        name = read_string(parse_u32(mem, 0xA) + 0xA)
        stack_offset = parse_s32(mem, 0x2A)
        type_ = parse_u32(mem, 0xE)
        if type_ != 0:
            size = read_s32(type_ + 0x2)
        else:
            size = 0
        if load_linkname and datatype in (3, 4):  # FUNC, VFUNC
            if MWCC_VERSION.name == "GC/1.1":
                offset = 0x2E
            elif MWCC_VERSION.name == "GC/2.6":
                offset = 0x32
            else:
                raise ValueError(f"Unsupported MWCC version: {MWCC_VERSION.name}")
            linkname = read_string(parse_u32(mem, offset) + 0xA)
        else:
            linkname = None
        return cls(
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


TYPE_CACHE: dict[str, MwccType] = {}


@dataclass
class MwccType:
    type_type: int
    size: int
    integral: Optional[int] = None  # For TYPEINT, TYPEFLOAT
    name: Optional[str] = None
    target: Optional[MwccType] = None
    offset: Optional[int] = None  # For TYPEBITFIELD
    bitlength: Optional[int] = None  # For TYPEBITFIELD
    scope: Optional[MwccType] = None  # For TYPEMEMBERPOINTER
    # TODO
    pass

    @classmethod
    def load(cls, addr: int) -> MwccType:
        if addr in TYPE_CACHE:
            return TYPE_CACHE[addr]

        mem = gdb.selected_inferior().read_memory(addr, 0x16)
        type_type = parse_s8(mem, 0x0)
        size = parse_u32(mem, 0x2)

        if type_type in (-1, 0, 8):  # TYPEILLEGAL, TYPEVOID, TYPELABEL
            rtype = cls(
                type_type=type_type,
                size=size,
            )
        elif type_type in (1, 2):  # TYPEINT, TYPEFLOAT
            integral = parse_u8(mem, 0x6)
            rtype = cls(
                type_type=type_type,
                size=size,
                integral=integral,
            )
        elif type_type in (3, 4, 5):  # TYPEENUM, TYPESTRUCT, TYPECLASS
            if type_type == 3:  # TYPEENUM
                name_addr = parse_u32(mem, 0x12)
            elif type_type == 4:  # TYPESTRUCT
                name_addr = parse_u32(mem, 0x6)
            elif type_type == 5:  # TYPECLASS
                name_addr = parse_u32(mem, 0xA)
            if name_addr == 0:
                name = None
            else:
                name = read_string(name_addr + 0xA)
            rtype = cls(
                type_type=type_type,
                size=size,
                name=name,
            )
        elif type_type == 6:  # TYPEFUNC
            target = MwccType.load(parse_u32(mem, 0xE))
            rtype = cls(
                type_type=type_type,
                size=size,
                target=target,
            )
        elif type_type == 7:  # TYPEBITFIELD
            target = MwccType.load(parse_u32(mem, 0x6))
            offset = parse_s8(mem, 0xA)
            bitlength = parse_s8(mem, 0xB)
            rtype = cls(
                type_type=type_type,
                size=size,
                target=target,
                offset=offset,
                bitlength=bitlength,
            )
        elif type_type == 10:  # TYPEMEMBERPOINTER
            scope = MwccType.load(parse_u32(mem, 0xA))
            target = MwccType.load(parse_u32(mem, 0x6))
            rtype = cls(
                type_type=type_type,
                size=size,
                scope=scope,
                target=target,
            )
        elif type_type in (11, 12):  # TYPEPOINTER, TYPEARRAY
            target = MwccType.load(parse_u32(mem, 0x6))
            rtype = cls(
                type_type=type_type,
                size=size,
                target=target,
            )
        else:
            raise ValueError(f"Unknown type: {type_type}")

        TYPE_CACHE[addr] = rtype
        return rtype


INTEGRAL_NAMES: list[str] = [
    "bool",
    "char",
    "signed char",
    "unsigned char",
    "wchar_t",
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
    "float",
    "short double",
    "double",
    "long double",
]


def format_type(rtype: MwccType) -> str:
    type_type = rtype.type_type
    if type_type == -1:  # TYPEILLEGAL
        return "illegal"
    elif type_type == 0:  # TYPEVOID
        return "void"
    elif type_type in (1, 2):  # TYPEINT, TYPEFLOAT
        return INTEGRAL_NAMES[rtype.integral]
    elif type_type == 3:  # TYPEENUM
        name = rtype.name if rtype.name else "<anonymous>"
        return f"enum {name}"
    elif type_type == 4:  # TYPESTRUCT
        name = rtype.name if rtype.name else "<anonymous>"
        return f"struct {name}"
    elif type_type == 5:  # TYPECLASS
        name = rtype.name if rtype.name else "<anonymous>"
        return f"class {name}"
    elif type_type == 6:  # TYPEFUNC
        return f"freturns({format_type(rtype.target)})"
    elif type_type == 7:  # TYPEBITFIELD
        return (
            f"bitfield({format_type(rtype.target)}){{{rtype.offset}:{rtype.bitlength}}}"
        )
    elif type_type == 8:  # TYPELABEL
        return "label"
    elif type_type == 10:  # TYPEMEMBERPOINTER
        return f"memberpointer({format_type(rtype.scope)},{format_type(rtype.target)})"
    elif type_type == 11:  # TYPEPOINTER
        return f"pointer({format_type(rtype.target)})"
    elif type_type == 12:  # TYPEARRAY
        return f"array({format_type(rtype.target)})"
    else:
        raise ValueError(f"Unknown type: {type_type}")


NODE_NAMES: list[str] = []


def load_node_names():
    if NODE_NAMES:
        return

    mem = gdb.selected_inferior().read_memory(
        MWCC_VERSION.nodenames_addr, MWCC_VERSION.nodenames_size * 0x4
    )
    for i in range(MWCC_VERSION.nodenames_size):
        str_addr = parse_u32(mem, i * 0x4)
        NODE_NAMES.append(read_string(str_addr))


@dataclass
class MwccENode:
    expr_type: str
    rtype: MwccType
    # Child expressions
    children: list[MwccENode]
    # Constants
    int_const: Optional[int] = None  # For EINTCONST
    float_const: Optional[float] = None  # For EFLOATCONST
    string_const: Optional[str] = None  # For ESTRINGCONST
    name: Optional[str] = None  # For EOBJREF, ELABEL

    @classmethod
    def load(cls, addr: int) -> MwccENode:
        mem = gdb.selected_inferior().read_memory(addr, 0x1A)
        expr_type = NODE_NAMES[parse_u8(mem, 0x0)]
        rtype = MwccType.load(parse_u32(mem, 0x6))
        if MWCC_VERSION.name == "GC/1.1":
            data_offset = 0xA
        elif MWCC_VERSION.name == "GC/2.6":
            data_offset = 0xE
        else:
            raise ValueError(f"Unsupported MWCC version: {MWCC_VERSION.name}")
        if expr_type == "EINTCONST":
            hi = parse_s32(mem, data_offset + 0)
            lo = parse_u32(mem, data_offset + 4)
            int_const = (hi << 32) | lo
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[],
                int_const=int_const,
            )
        elif expr_type == "EFLOATCONST":
            float_const = parse_f64(mem, data_offset)
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[],
                float_const=float_const,
            )
        elif expr_type == "ESTRINGCONST":
            string_const = read_string(parse_u32(mem, data_offset + 4))
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[],
                string_const=string_const,
            )
        elif expr_type == "EOBJREF":
            obj = MwccObject.load(parse_u32(mem, data_offset))
            name = obj.name
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[],
                name=name,
            )
        elif expr_type == "ELABEL":
            label_addr = parse_u32(mem, data_offset)
            label_name = read_u32(label_addr + 0x8)
            label_name = read_string(label_name + 0xA)
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=children,
            )
        elif expr_type in (
            "EPOSTINC",
            "EPOSTDEC",
            "EPREINC",
            "EPREDEC",
            "EINDIRECT",
            "EMONMIN",
            "EBINNOT",
            "ELOGNOT",
            "EFORCELOAD",
            "ETYPCON",
            "EBITFIELD",
        ):
            # Unary operators
            expr = cls.load(parse_u32(mem, data_offset))
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[expr],
            )
        elif expr_type in (
            "EMUL",
            "EMULV",
            "EDIV",
            "EMODULO",
            "EADDV",
            "ESUBV",
            "EADD",
            "ESUB",
            "ESHL",
            "ESHR",
            "ELESS",
            "EGREATER",
            "ELESSEQU",
            "EGREATEREQU",
            "EEQU",
            "ENOTEQU",
            "EAND",
            "EXOR",
            "EOR",
            "ELAND",
            "ELOR",
            "EASS",
            "EMULASS",
            "EDIVASS",
            "EMODASS",
            "EADDASS",
            "ESUBASS",
            "ESHLASS",
            "ESHRASS",
            "EANDASS",
            "EXORASS",
            "EORASS",
            "EBCLR",
            "EBSET",
            "ECOMMA",
            "EPMODULO",
            "EROTL",
            "EROTR",
            "EBTST",
            # TODO: ENULLCHECK has a unique id too
            "ENULLCHECK",
        ):
            # Binary operators
            left = cls.load(parse_u32(mem, data_offset))
            right = cls.load(parse_u32(mem, data_offset + 4))
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[left, right],
            )
        elif expr_type in ("ECOND", "ECONDASS"):
            # Ternary operators
            cond = cls.load(parse_u32(mem, data_offset))
            expr1 = cls.load(parse_u32(mem, data_offset + 4))
            expr2 = cls.load(parse_u32(mem, data_offset + 8))
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[cond, expr1, expr2],
            )
        elif expr_type in ("EFUNCCALL", "EFUNCCALLP"):
            # Function call
            funcref = cls.load(parse_u32(mem, data_offset))
            args_addr = parse_u32(mem, data_offset + 4)
            children = [funcref]
            while args_addr != 0:
                args_mem = gdb.selected_inferior().read_memory(args_addr, 0x8)
                arg = MwccENode.load(parse_u32(args_mem, 0x4))
                children.append(arg)
                args_addr = parse_u32(args_mem, 0x0)
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=children,
            )
        elif expr_type in (
            "EVECTORCONST",
            "EPRECOMP",
            "ETEMP",
            "EINITTRYCATCH",
            "EDEFINE",
            "EREUSE",
        ):
            # TODO
            return cls(
                expr_type=expr_type,
                rtype=rtype,
                children=[],
            )
        else:
            raise ValueError(f"Unsupported expression type: {expr_type}")


@dataclass
class MwccStatement:
    next_addr: int
    stmt_type: int
    expr_addr: int
    line: Optional[int]
    label_name: Optional[str]
    # For ST_SWITCH
    switch_cases: Optional[Tuple[int, str]] = None
    default_label_name: Optional[str] = None

    @classmethod
    def load(cls, addr: int) -> MwccStatement:
        mem = gdb.selected_inferior().read_memory(addr, 0x1A)
        next_addr = parse_u32(mem, 0x0)
        stmt_type = parse_u8(mem, 0x4)
        expr_addr = parse_u32(mem, 0xA)
        line = parse_s32(mem, 0x16)
        if line == -1:
            line = None
        if stmt_type in (2, 3, 6, 7):  # ST_LABEL, ST_GOTO, ST_IFGOTO, ST_IFNGOTO
            label_addr = parse_u32(mem, 0xE)
            label_name = read_u32(label_addr + 0x8)
            label_name = read_string(label_name + 0xA)
        else:
            label_name = None
        if stmt_type == 5:  # ST_SWITCH
            switch_info_addr = parse_u32(mem, 0xE)
            switch_mem = gdb.selected_inferior().read_memory(switch_info_addr, 0x8)

            case_addr = parse_u32(switch_mem, 0x0)
            switch_cases = []
            while case_addr != 0:
                case_mem = gdb.selected_inferior().read_memory(case_addr, 0x10)
                label_addr = parse_u32(case_mem, 0x4)
                label_name = read_u32(label_addr + 0x8)
                label_name = read_string(label_name + 0xA)
                hi = parse_s32(case_mem, 0x8)
                lo = parse_u32(case_mem, 0xC)
                value = (hi << 32) | lo
                switch_cases.append((value, label_name))
                case_addr = parse_u32(case_mem, 0x0)
            default_label_addr = parse_u32(switch_mem, 0x4)
            default_label_name = read_u32(default_label_addr + 0x8)
            default_label_name = read_string(default_label_name + 0xA)
        else:
            switch_cases = None
            default_label_name = None
        return cls(
            next_addr=next_addr,
            stmt_type=stmt_type,
            expr_addr=expr_addr,
            line=line,
            label_name=label_name,
            switch_cases=switch_cases,
            default_label_name=default_label_name,
        )


def print_type(f, rtype: MwccType):
    print(f" {format_type(rtype)}", file=f)


def print_expression(f, expr: MwccENode, indent):
    print("  " * indent, end="", file=f)

    # TODO: print flags
    expr_type = expr.expr_type
    rtype = expr.rtype
    print(f"{expr_type}", end="", file=f)

    if expr_type == "EINTCONST":
        print(f" [{expr.int_const:#x}]", end="", file=f)
        print_type(f, rtype)
    elif expr_type == "EFLOATCONST":
        # 17 significant figures is enough for any double, but we try 15 first
        # to get a shorter representation if possible
        float_str = f"{expr.float_const:.15g}"
        if float(float_str) != expr.float_const:
            float_str = f"{expr.float_const:.17g}"
        print(f" [{float_str}]", end="", file=f)
        print_type(f, rtype)
    elif expr_type == "ESTRINGCONST":
        encoding = ""
        for c in expr.string_const:
            if c == "\x00":
                encoding += "\\0"
            elif c == "\t":
                encoding += "\\t"
            elif c == "\n":
                encoding += "\\n"
            elif c == "\r":
                encoding += "\\r"
            elif c == "\\":
                encoding += "\\\\"
            elif c == '"':
                encoding += '\\"'
            elif c >= "\x20" and c <= "\x7e":
                encoding += c
            else:
                encoding += f"\\x{ord(c):02x}"
        print(f' ["{encoding}"]', end="", file=f)
        print_type(f, rtype)
    elif expr_type == "EVECTORCONST":
        print(" <unimplemented>", file=f)
    elif expr_type in ("EOBJREF", "ELABEL"):
        print(f" [{expr.name}]", end="", file=f)
        print_type(f, rtype)
    elif expr_type in (
        "EVECTORCONST",
        "EPRECOMP",
        "ETEMP",
        "EINITTRYCATCH",
        "EDEFINE",
        "EREUSE",
    ):
        print(" <unimplemented>", file=f)
    else:
        print_type(f, rtype)
        for child in expr.children:
            print_expression(f, child, indent + 1)


def print_statement(f, stmt: MwccStatement):
    stmt_type = stmt.stmt_type
    if stmt.line:
        print(f"{stmt.line} ", end="", file=f)

    if stmt_type == 1:
        print(f"ST_NOP", file=f)
    elif stmt_type == 2:
        print(f"ST_LABEL L{stmt.label_name}", file=f)
    elif stmt_type == 3:
        print(f"ST_GOTO L{stmt.label_name}", file=f)
    elif stmt_type == 4:
        print(f"ST_EXPRESSION", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
    elif stmt_type == 5:
        print(f"ST_SWITCH", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
        for value, label_name in stmt.switch_cases:
            print(f"  CASE {value:#x}: L{label_name}", file=f)
        print(f"  DEFAULT: L{stmt.default_label_name}", file=f)
    elif stmt_type == 6:
        print(f"ST_IFGOTO L{stmt.label_name}", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
    elif stmt_type == 7:
        print(f"ST_IFNGOTO L{stmt.label_name}", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
    elif stmt_type == 8:
        print(f"ST_RETURN", file=f)
        if stmt.expr_addr != 0:
            expr = MwccENode.load(stmt.expr_addr)
            print_expression(f, expr, 1)
    elif stmt_type == 12:
        print(f"ST_BEGINCATCH", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
    elif stmt_type == 13:
        print(f"ST_ENDCATCH", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
    elif stmt_type == 14:
        print(f"ST_ENDCATCHDTOR", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
    elif stmt_type == 15:
        print(f"ST_GOTOEXPR", file=f)
        expr = MwccENode.load(stmt.expr_addr)
        print_expression(f, expr, 1)
    elif stmt_type == 16:
        print(f"ST_ASM", file=f)
        print("  ...", file=f)
    else:
        raise ValueError(f"Unknown statement type: {stmt_type}")


def print_ast(stmt_addr: int, pass_number: int, pass_name: str):
    output_path = Path(OUTPUT_DIR) / f"frontend-{pass_number:02}-ast-{pass_name}.txt"
    print(f"Dumping AST to {output_path}")
    with open(output_path, "w") as f:
        while stmt_addr != 0:
            stmt = MwccStatement.load(stmt_addr)
            print_statement(f, stmt)
            stmt_addr = stmt.next_addr


@dataclass
class MwccOpcodeInfo:
    mnemonic: str
    format_str: str


@dataclass
class MwccPCodeArg:
    class Kind(Enum):
        GPR = 0  # General Purpose Register
        FPR = 1  # Floating Point Register
        SPR = 2  # Special Purpose Register
        CRFIELD = 3  # Condition Register Field
        VR = 4  # Vector Register
        IMMEDIATE = 5  # Immediate Value
        MEMORY = 6  # Memory Address
        LABEL = 7  # Label
        PLACEHOLDER = 8  # Placeholder for unused arguments

    kind: Kind
    reg: Optional[int] = None  # For GPR, FPR, SPR, crfield, vector register
    imm: Optional[int] = None  # For immediate, memory
    obj_addr: Optional[int] = None  # For memory
    block_index: Optional[int] = None  # For label


@dataclass
class MwccPcode:
    next_addr: int
    line: Optional[int]
    op: int
    # TODO: flags
    args: list[MwccPCodeArg]

    @classmethod
    def load(cls, addr: int) -> MwccPcode:
        if MWCC_VERSION.layout == "wii13":
            return load_pcode_wii13(addr)
        if MWCC_VERSION.name == "GC/1.1":
            mem = gdb.selected_inferior().read_memory(addr, 0x1C)
            # flags = parse_u32(mem, 0x16)
            arg_count = parse_s16(mem, 0x1A)
            if arg_count > 0:
                arg_mem = gdb.selected_inferior().read_memory(
                    addr + 0x1C, arg_count * 0xC
                )

            args = []
            for i in range(arg_count):
                arg_offset = i * 0xC
                kind = parse_u8(arg_mem, arg_offset)
                if kind == 0:  # GPR
                    reg = parse_u16(arg_mem, arg_offset + 2)
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.GPR, reg=reg))
                elif kind == 1:  # FPR
                    reg = parse_u16(arg_mem, arg_offset + 2)
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.FPR, reg=reg))
                elif kind == 2:  # SPR
                    reg = parse_u16(arg_mem, arg_offset + 2)
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.SPR, reg=reg))
                elif kind == 3:  # crfield
                    reg = parse_u16(arg_mem, arg_offset + 2)
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.CRFIELD, reg=reg))
                elif kind == 4:  # immediate
                    imm = parse_s32(arg_mem, arg_offset + 2)
                    obj_addr = parse_u32(arg_mem, arg_offset + 6)
                    args.append(
                        MwccPCodeArg(
                            MwccPCodeArg.Kind.IMMEDIATE, imm=imm, obj_addr=obj_addr
                        )
                    )
                elif kind == 5:  # memory
                    imm = parse_s32(arg_mem, arg_offset + 2)
                    obj_addr = parse_u32(arg_mem, arg_offset + 6)
                    args.append(
                        MwccPCodeArg(
                            MwccPCodeArg.Kind.MEMORY, imm=imm, obj_addr=obj_addr
                        )
                    )
                elif kind == 6:  # label
                    label_addr = parse_u32(arg_mem, arg_offset + 2)
                    label = MwccPCodeLabel.load(label_addr)
                    block = MwccBlock.load(label.block_addr)
                    block_index = block.index
                    args.append(
                        MwccPCodeArg(MwccPCodeArg.Kind.LABEL, block_index=block_index)
                    )
                elif kind == 9:  # vector register
                    reg = parse_u16(arg_mem, arg_offset + 2)
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.VR, reg=reg))
                elif kind == 10:  # placeholder
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.PLACEHOLDER))
                else:
                    raise ValueError(f"Unknown operand kind: {kind}")

            return cls(
                next_addr=parse_u32(mem, 0x0),
                line=None,
                op=parse_s16(mem, 0x14),
                args=args,
            )
        elif MWCC_VERSION.name == "GC/2.6":
            mem = gdb.selected_inferior().read_memory(addr, 0x24)
            line = parse_s32(mem, 0x1C)
            if line == -1:
                line = None

            arg_count = parse_s16(mem, 0x22)
            if arg_count > 0:
                arg_mem = gdb.selected_inferior().read_memory(
                    addr + 0x24, arg_count * 0xC
                )

            args = []
            for i in range(arg_count):
                arg_offset = i * 0xC
                kind = parse_u8(arg_mem, arg_offset)
                if kind == 0:  # Register
                    regclass = parse_u8(arg_mem, arg_offset + 1)
                    reg = parse_u16(arg_mem, arg_offset + 4)
                    if regclass == 0:  # SPR
                        kind = MwccPCodeArg.Kind.SPR
                    elif regclass == 1:  # crfield
                        kind = MwccPCodeArg.Kind.CRFIELD
                    elif regclass == 2:  # vector register
                        kind = MwccPCodeArg.Kind.VR
                    elif regclass == 3:  # FPR
                        kind = MwccPCodeArg.Kind.FPR
                    elif regclass == 4:  # GPR
                        kind = MwccPCodeArg.Kind.GPR
                    else:
                        raise ValueError(f"Unknown register class: {regclass}")
                    args.append(MwccPCodeArg(kind, reg=reg))
                elif kind == 1:  # sysreg
                    reg = parse_u16(arg_mem, arg_offset + 4)
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.SPR, reg=reg))
                elif kind == 2:  # immediate
                    imm = parse_s32(arg_mem, arg_offset + 2)
                    obj_addr = parse_u32(arg_mem, arg_offset + 6)
                    args.append(
                        MwccPCodeArg(
                            MwccPCodeArg.Kind.IMMEDIATE, imm=imm, obj_addr=obj_addr
                        )
                    )
                elif kind == 3:  # memory
                    imm = parse_s32(arg_mem, arg_offset + 2)
                    obj_addr = parse_u32(arg_mem, arg_offset + 6)
                    args.append(
                        MwccPCodeArg(
                            MwccPCodeArg.Kind.MEMORY, imm=imm, obj_addr=obj_addr
                        )
                    )
                elif kind == 4:  # label
                    label_addr = parse_u32(arg_mem, arg_offset + 2)
                    label = MwccPCodeLabel.load(label_addr)
                    block = MwccBlock.load(label.block_addr)
                    block_index = block.index
                    args.append(
                        MwccPCodeArg(MwccPCodeArg.Kind.LABEL, block_index=block_index)
                    )
                elif kind == 6:  # placeholder
                    args.append(MwccPCodeArg(MwccPCodeArg.Kind.PLACEHOLDER))
                else:
                    raise ValueError(f"Unknown operand kind: {kind}")

            return cls(
                next_addr=parse_u32(mem, 0x0),
                line=line,
                op=parse_s16(mem, 0x20),
                args=args,
            )
        else:
            raise ValueError(f"Unsupported MWCC version: {MWCC_VERSION.name}")


@dataclass
class MwccBlock:
    next_addr: int
    prev_addr: int
    label_addr: int
    predecessors_addr: int
    successors_addr: int
    instr_addr: int
    index: int
    line: Optional[int]
    loop_weight: int
    pcode_count: int
    flags: int  # TODO: parse flags

    @classmethod
    def load(cls, addr: int) -> MwccBlock:
        if MWCC_VERSION.layout == "wii13":
            return load_block_wii13(addr)
        if MWCC_VERSION.name == "GC/1.1":
            mem = gdb.selected_inferior().read_memory(addr, 0x30)
            line = parse_s32(mem, 0x20)
            if line == -1:
                line = None
            return cls(
                next_addr=parse_u32(mem, 0x0),
                prev_addr=parse_u32(mem, 0x4),
                label_addr=parse_u32(mem, 0x8),
                predecessors_addr=parse_u32(mem, 0xC),
                successors_addr=parse_u32(mem, 0x10),
                instr_addr=parse_u32(mem, 0x14),
                index=parse_s32(mem, 0x1C),
                line=line,
                loop_weight=parse_s32(mem, 0x28),
                pcode_count=parse_s16(mem, 0x2C),
                flags=parse_u16(mem, 0x2E),
            )
        elif MWCC_VERSION.name == "GC/2.6":
            mem = gdb.selected_inferior().read_memory(addr, 0x2C)
            return cls(
                next_addr=parse_u32(mem, 0x0),
                prev_addr=parse_u32(mem, 0x4),
                label_addr=parse_u32(mem, 0x8),
                predecessors_addr=parse_u32(mem, 0xC),
                successors_addr=parse_u32(mem, 0x10),
                instr_addr=parse_u32(mem, 0x14),
                index=parse_s32(mem, 0x1C),
                line=None,
                loop_weight=parse_s32(mem, 0x24),
                pcode_count=parse_s16(mem, 0x28),
                flags=parse_u16(mem, 0x2A),
            )
        else:
            raise ValueError(f"Unsupported MWCC version: {MWCC_VERSION.name}")


@dataclass
class MwccPCodeLabel:
    next_addr: int
    block_addr: int
    index: int

    @classmethod
    def load(cls, addr: int) -> MwccPCodeLabel:
        mem = gdb.selected_inferior().read_memory(addr, 0xC)
        return cls(
            next_addr=parse_u32(mem, 0x0),
            block_addr=parse_u32(mem, 0x4),
            index=parse_u16(mem, 0xA),
        )


@dataclass
class MwccIGNode:
    class Flag(Enum):
        fSpilled = 0
        fCoalesced = 1
        fCoalescedInto = 2
        fPairHigh = 3
        fPairLow = 4
        fRematerialized = 5

    next_addr: int
    virtual_reg: int
    physical_reg: int
    cost: int
    flags: list[Flag]
    obj_addr: int
    neighbors: list[int]

    @classmethod
    def load(cls, addr: int) -> MwccIGNode:
        if MWCC_VERSION.layout == "wii13":
            return load_ignode_wii13(addr)
        if MWCC_VERSION.name == "GC/1.1":
            mem = gdb.selected_inferior().read_memory(addr, 0x16)
            next_addr = parse_u32(mem, 0x0)
            obj_addr = parse_u32(mem, 0x4)
            cost = parse_s32(mem, 0x8)
            virtual_reg = parse_s16(mem, 0xC)
            physical_reg = parse_s16(mem, 0x10)
            flags_value = parse_u8(mem, 0x12)
            num_neighbors = parse_s16(mem, 0x14)
            neighbors_addr = addr + 0x16
        elif MWCC_VERSION.name == "GC/2.6":
            mem = gdb.selected_inferior().read_memory(addr, 0x1A)
            next_addr = parse_u32(mem, 0x0)
            obj_addr = parse_u32(mem, 0x4)
            cost = parse_s32(mem, 0xC)
            virtual_reg = parse_s16(mem, 0x10)
            physical_reg = parse_s16(mem, 0x14)
            flags_value = parse_u8(mem, 0x16)
            num_neighbors = parse_s16(mem, 0x18)
            neighbors_addr = addr + 0x1A
        else:
            raise ValueError(f"Unsupported MWCC version: {MWCC_VERSION.name}")

        neighbors = []
        if num_neighbors > 0:
            mem = gdb.selected_inferior().read_memory(
                neighbors_addr, num_neighbors * 0x2
            )
            for i in range(num_neighbors):
                neighbor = parse_s16(mem, i * 0x2)
                neighbors.append(neighbor)

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

        return cls(
            next_addr=next_addr,
            virtual_reg=virtual_reg,
            physical_reg=physical_reg,
            cost=cost,
            flags=flags,
            obj_addr=obj_addr,
            neighbors=neighbors,
        )


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


MWCC_OPCODE_INFO: list[MwccOpcodeInfo] = []


def load_opcode_info():
    if MWCC_OPCODE_INFO:
        return

    # PORT: the entry size is version data, not code (0x10 for GC/1.1, 0x12
    # for GC/2.6, 0x16 for Wii/1.3 where each entry also carries the encoder's
    # instruction word).
    size = MWCC_VERSION.opcodeinfo_stride

    # Load opcode info from the binary
    mem = gdb.selected_inferior().read_memory(
        MWCC_VERSION.opcodeinfo_addr, MWCC_VERSION.opcodeinfo_size * size
    )
    for i in range(MWCC_VERSION.opcodeinfo_size):
        offset = i * size
        mnemonic = read_string(parse_u32(mem, offset))
        format_str = read_string(parse_u32(mem, offset + 4))
        MWCC_OPCODE_INFO.append(
            MwccOpcodeInfo(mnemonic=mnemonic, format_str=format_str)
        )


def format_operands(instr) -> str:
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
        elif arg.kind == MwccPCodeArg.Kind.LABEL:
            out += f"B{arg.block_index}"
        else:
            raise ValueError(f"Unknown operand kind: {arg.kind}")

    if arg_count > 6:
        out += ",..."

    return out


def print_instruction(f, instr: MwccPcode, block_line_number: Optional[int]):
    operands = format_operands(instr)
    # TODO: show "record" bit as dot
    mnemonic = MWCC_OPCODE_INFO[instr.op].mnemonic.lower()

    line_number = instr.line or block_line_number
    line_number_str = f"{line_number:>5}" if line_number else "     "
    print(f" {line_number_str}  {mnemonic:<8} {operands}", file=f)


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


def print_block(f, block: MwccBlock):
    if MWCC_VERSION.layout == "wii13":
        print_block_wii13(f, block)
        return
    print(
        f":{{{block.flags:04x}}}::::::::::::::::::::::::::::::::::::::::LOOPWEIGHT={block.loop_weight}",
        file=f,
    )
    print(f"B{block.index}: ", end="", file=f)
    print("Successors = { ", end="", file=f)
    link_addr = block.successors_addr
    while link_addr != 0:
        link_block = MwccBlock.load(read_u32(link_addr + 0x4))
        print(f"B{link_block.index} ", end="", file=f)
        link_addr = read_u32(link_addr + 0x0)
    print("}  Predecessors = { ", end="", file=f)
    link_addr = block.predecessors_addr
    while link_addr != 0:
        link_block = MwccBlock.load(read_u32(link_addr + 0x4))
        print(f"B{link_block.index} ", end="", file=f)
        link_addr = read_u32(link_addr + 0x0)
    print("}  Labels = { ", end="", file=f)
    label_addr = block.label_addr
    while label_addr != 0:
        label = MwccPCodeLabel.load(label_addr)
        print(f"L{label.index} ", end="", file=f)
        label_addr = label.next_addr
    print("}", file=f)

    instr_addr = block.instr_addr
    while instr_addr != 0:
        instr = MwccPcode.load(instr_addr)
        print_instruction(f, instr, block.line)
        instr_addr = instr.next_addr

    print("", file=f)


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


def print_pcode(pass_number: int, pass_name: str):
    output_path = Path(OUTPUT_DIR) / f"backend-{pass_number:02}-{pass_name}.txt"
    print(f"Dumping PCode to {output_path}")
    with open(output_path, "w") as f:
        block_addr = read_u32(MWCC_VERSION.pcbasicblocks_addr)
        while block_addr != 0:
            block = MwccBlock.load(block_addr)
            print_block(f, block)
            block_addr = block.next_addr


class RegType(Enum):
    GPR = 0
    FPR = 1


def reg_name(reg: int, reg_type: RegType) -> str:
    if reg == -1:
        return "none"
    elif reg_type == RegType.GPR:
        return f"r{reg}"
    elif reg_type == RegType.FPR:
        return f"f{reg}"
    else:
        raise ValueError(f"Unknown register type: {reg_type}")


@dataclass
class RegallocObject:
    reg_type: RegType
    virtual_reg: int
    physical_reg: int


REGALLOC_PASS = defaultdict[RegType, int](lambda: 0)
# Objects from regalloc nodes. New objects from regalloc spills don't seem to be stored
# anywhere else so we collect them here.
POTENTIAL_SPILLS = []
# Variable information collected during regalloc
REGALLOC_OBJECTS: dict[int, RegallocObject] = {}


def print_regalloc():
    assigned_nodes_addr = None
    reg_type = None

    # Find out which register class we are processing (GPR or FPR)
    sp = int(gdb.parse_and_eval("$esp"))
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

    REGALLOC_PASS[reg_type] += 1
    pass_number = REGALLOC_PASS[reg_type]
    pass_name = reg_type.name.lower()
    if reg_type == RegType.GPR:
        num_regs = read_s16(MWCC_VERSION.used_virtual_registers_gpr_addr)
    elif reg_type == RegType.FPR:
        num_regs = read_s16(MWCC_VERSION.used_virtual_registers_fpr_addr)

    # Read all interference graph nodes
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
            if node.obj_addr != 0:
                POTENTIAL_SPILLS.append(node.obj_addr)
                REGALLOC_OBJECTS[node.obj_addr] = RegallocObject(
                    reg_type=reg_type,
                    virtual_reg=node.virtual_reg,
                    physical_reg=node.physical_reg,
                )

    output_path = Path(OUTPUT_DIR) / f"regalloc-{pass_name}-pass-{pass_number}-all.txt"
    print(f"Dumping all registers to {output_path}")
    with open(output_path, "w") as f:
        for node in nodes.values():
            print(
                f"{reg_name(node.virtual_reg, reg_type)} -> {reg_name(node.physical_reg, reg_type)}",
                file=f,
                end="",
            )
            if node.obj_addr:
                obj = MwccObject.load(node.obj_addr)
                print(f" { obj.name}", file=f, end="")
            print("", file=f)
            print(f"  flags:", file=f, end="")
            for flag in node.flags:
                print(f" {flag.name}", file=f, end="")
            print("", file=f)
            print(f"  cost: {node.cost}", file=f)
            if node.neighbors:
                neighbors_str = " ".join(
                    reg_name(i, reg_type) for i in sorted(node.neighbors)
                )
                print(
                    f"  neighbors: {len(node.neighbors)} ({neighbors_str})",
                    file=f,
                )
            else:
                print("  neighbors: 0", file=f)

    output_path = (
        Path(OUTPUT_DIR) / f"regalloc-{pass_name}-pass-{pass_number}-assigned.txt"
    )
    print(f"Dumping assigned registers to {output_path}")
    with open(output_path, "w") as f:
        # TODO: print free registers
        assigned_nodes = []
        while assigned_nodes_addr != 0:
            assigned_nodes.append(nodes[assigned_nodes_addr])
            assigned_nodes_addr = nodes[assigned_nodes_addr].next_addr

        for i, node in enumerate(assigned_nodes):
            prev_neighbors = set(node.neighbors)
            for j in range(i, len(assigned_nodes)):
                prev_neighbors.discard(assigned_nodes[j].virtual_reg)
            print(
                f"{reg_name(node.virtual_reg, reg_type)} -> {reg_name(node.physical_reg, reg_type)}",
                file=f,
                end="",
            )
            if node.obj_addr:
                obj = MwccObject.load(node.obj_addr)
                print(f" {obj.name}", file=f, end="")
            print("", file=f)
            print(f"  flags:", file=f, end="")
            for flag in node.flags:
                print(f" {flag.name}", file=f, end="")
            print("", file=f)
            print(f"  cost: {node.cost}", file=f)
            print(
                f"  adjusted cost: {node.cost / len(prev_neighbors) if prev_neighbors else 0:.2f}",
                file=f,
            )
            if prev_neighbors:
                prev_neighbors_str = " ".join(
                    reg_name(i, reg_type) for i in sorted(prev_neighbors)
                )
                print(
                    f"  previous neighbors: {len(prev_neighbors)} ({prev_neighbors_str})",
                    file=f,
                )
            else:
                print("  previous neighbors: 0", file=f)
            if node.neighbors:
                neighbors_str = " ".join(
                    reg_name(i, reg_type) for i in sorted(node.neighbors)
                )
                print(
                    f"  neighbors: {len(node.neighbors)} ({neighbors_str})",
                    file=f,
                )
            else:
                print("  neighbors: 0", file=f)


def load_object_list(list_addr: int) -> list[int]:
    node_addr = read_u32(list_addr)
    obj_addrs = []
    while node_addr != 0:
        mem = gdb.selected_inferior().read_memory(node_addr, 0x8)
        obj_addrs.append(parse_u32(mem, 0x4))
        node_addr = parse_u32(mem, 0x0)
    return obj_addrs


def print_objects(f, stack_base: int, objects: list[int], is_locals=False):
    for obj_addr in reversed(objects):
        obj = MwccObject.load(obj_addr)

        if MWCC_VERSION.name == "GC/1.1":
            # On GC/1.1, everything is allocated a stack address, except for locals who have a register
            if is_locals and obj_addr in REGALLOC_OBJECTS:
                has_stack = False
            else:
                has_stack = True
        else:
            raise ValueError(f"Unsupported MWCC version: {MWCC_VERSION.name}")

        if has_stack:
            stack_str = f"r1+0x{stack_base + obj.stack_offset:04x}-0x{stack_base + obj.stack_offset + obj.size:04x} "
        else:
            stack_str = ""

        if obj_addr in REGALLOC_OBJECTS:
            regalloc_obj = REGALLOC_OBJECTS[obj_addr]
            reg_type = regalloc_obj.reg_type
            virtual_reg = regalloc_obj.virtual_reg
            physical_reg = regalloc_obj.physical_reg
            register_str = f"  {reg_name(virtual_reg, reg_type).rjust(4)} -> {reg_name(physical_reg, reg_type).rjust(4)}   "
        else:
            register_str = ""

        print(
            f"  {stack_str}{register_str}{obj.name}",
            file=f,
        )


def print_variables():
    # PORT: upstream supported this for GC/1.1 only; make that a property of
    # the version table so adding a build is data.
    if not MWCC_VERSION.supports_variables:
        print("variables.txt: not supported for this compiler build - skipped")
        return

    output_file = "variables.txt"
    output_path = Path(OUTPUT_DIR) / output_file
    print(f"Dumping variables to {output_path}")
    with open(output_path, "w") as f:
        stack_base = read_u32(MWCC_VERSION.frame_base_size_addr) + read_u32(
            MWCC_VERSION.frame_call_args_size_addr
        )

        arguments = load_object_list(MWCC_VERSION.arguments_addr)
        locals_ = load_object_list(MWCC_VERSION.locals_addr)
        temps = load_object_list(MWCC_VERSION.temps_addr)

        spills = []
        for obj_addr in POTENTIAL_SPILLS:
            if (
                obj_addr in arguments
                or obj_addr in locals_
                or obj_addr in temps
                or obj_addr in spills
            ):
                continue
            spills.append(obj_addr)

        print("temps:", file=f)
        print_objects(f, stack_base, temps)
        print("spills:", file=f)
        print_objects(f, stack_base, spills)
        print("locals:", file=f)
        print_objects(f, stack_base, locals_, is_locals=True)
        print("arguments:", file=f)
        print_objects(f, stack_base, arguments)


def find_current_function() -> MwccObject:
    # For GC/1.1, there seems to be no global variable for the current function.
    # Instead, we inspect the stack for the second argument to CodeGen_Generator.
    if MWCC_VERSION.name == "GC/1.1":
        sp = int(gdb.parse_and_eval("$esp"))
        current_function_addr = read_u32(sp + 8)
    else:
        current_function_addr = read_u32(MWCC_VERSION.gfunction_addr)
    return MwccObject.load(current_function_addr, load_linkname=True)



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
            f"ImageBase the version table is relative to), found {head!r}.\n"
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
    `src\\fn_8004C9A0.cpp` into `srcfn_8004C9A0.cpp`.  We split on whitespace,
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
        if ch in "\"'":
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
        Path(r"C:\msys64\mingw64\bin\gdb.exe"),
        Path(r"C:\msys64\usr\bin\gdb.exe"),
    ]
    for cand in candidates:
        try:
            if cand.exists():
                return str(cand)
        except OSError:
            continue
    raise SystemExit(
        "mwcc-debugger: no gdb found.  Windows needs a *native* mingw-w64 gdb\n"
        "  (a Cygwin/MSYS gdb cannot debug a native mwcceppc.exe).  Either:\n"
        "    * MSYS2:      pacman -S mingw-w64-x86_64-gdb\n"
        "                  then --gdb C:\\msys64\\mingw64\\bin\\gdb.exe\n"
        "    * no MSYS2:   python tools/mwcc-debugger/fetch_gdb.py --dest DIR\n"
        "                  (downloads the MSYS2 package plus its runtime DLLs)\n"
        "  or put the gdb on PATH."
    )


def gdb_escape_path(path: str) -> str:
    """Escape a path for gdb's command parser (backslash is an escape char).

    Forward slashes are accepted by gdb on Windows and dodge the whole problem,
    so use them and only quote if something odd is left.
    """
    posix = path.replace("\\", "/")
    if any(c.isspace() for c in posix):
        return '"' + posix + '"'
    return posix


def gdb_quote_args(argv: list) -> str:
    """Quote a compiler argument list for gdb's `set args` parser (which
    processes backslash escapes inside double quotes)."""
    out = []
    for a in argv:
        a = a.replace("\\", "\\\\").replace('"', '\\"')
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
        # PORT: the repository's one process runner (lib.proc), with gdb's
        # output left on the terminal.
        gdb_result = lib_proc.run(
            gdb_command, env=env, timeout=args.timeout, stdout=None, stderr=None
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
