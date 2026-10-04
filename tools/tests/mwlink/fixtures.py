"""Shared fixtures of the tools/mwlink/ tests: a link map, an output ELF and an input object, all built in memory."""
from __future__ import annotations

import struct

from tools.lib.binary.build import ElfBuilder
from tools.lib.binary.elf import SHF_ALLOC, SHF_EXECINSTR

#: A three-fragment .ctors listing in the linker's map format (offset, size, VA, file offset, align, name, source).
CTORS_MAP = ("\n.ctors section layout\n"
             "  00000000 000000 8056f2c0 0056b4c0  1 .ctors$00 \tLinker Generated Symbol File \n"
             "  00000000 000004 8056f2c0 0056b4c0  1 .ctors$10 \t__init_cpp_exceptions.o \n"
             "  00000004 00000c 8056f2c4 0056b4c4  1 .ctors \tfoo.o \n")

#: Where the fixture object's .text lands in the fixture link.
A = 0x8056F2C0


def output_elf(sections) -> bytes:
    """A linked-image-shaped ELF: `sections` is `[(name, addr, data)]`, each allocated at its address."""
    b = ElfBuilder()
    for name, addr, data in sections:
        b.section(name, data, addr=addr, flags=SHF_ALLOC)
    return b.build()


def input_object() -> bytes:
    """A tiny Metrowerks-shaped object: .text + .ctors$10, `foo` in .text, `bar` in .ctors$10, one ADDR32 -> foo."""
    b = ElfBuilder()
    b.section(".text", b"\0\0\0\0", flags=SHF_ALLOC | SHF_EXECINSTR)
    b.section(".ctors$10", b"\0\0\0\0", flags=SHF_ALLOC)
    b.symbol("", ".text", bind="local", type="section")
    b.symbol("", ".ctors$10", bind="local", type="section")
    b.symbol("foo", ".text", size=4, type="func")
    b.symbol("bar", ".ctors$10", size=4, type="object")
    b.reloc(".text", 0, "foo", "R_PPC_ADDR32")
    return b.build()


def object_map(source: str = "fixture.o") -> str:
    """The map of a link that kept the fixture object: .text at A, .ctors$10 at A + 0x10."""
    text = "\n.text section layout\n"
    text += f"  00000000 000004 {A:08x} 00000200  1 .text \t{source} \n"
    text += f"  00000000 000004 {A:08x} 00000200  4 foo \t{source} \n"
    text += "\n.ctors section layout\n"
    text += f"  00000000 000004 {A + 0x10:08x} 00000210  1 .ctors$10 \t{source} \n"
    text += f"  00000010 000004 {A + 0x10:08x} 00000210  4 bar \t{source} \n"
    return text


def linked_elf(text_word: int) -> bytes:
    """The output ELF of that link, its .text holding `text_word` where the ADDR32 relocation applies."""
    return output_elf([(".text", A, struct.pack(">I", text_word)), (".ctors", A + 0x10, b"\0\0\0\0")])
