"""lib.binary.pe: the one PE32 reader (mwlink and mwcc-debugger), on PeBuilder images; nothing reads the live tree."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import struct
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.binary.build import PeBuilder
from tools.lib.binary.pe import MACHINE_I386, Pe

TIER = "fixture"
#: One scratch directory for the whole module, removed when the interpreter exits.
_TMP = tempfile.TemporaryDirectory()


def image(builder: PeBuilder) -> tuple[Pe, PeBuilder]:
    path = Path(tempfile.mkdtemp(dir=_TMP.name)) / "tool.exe"
    builder.write(path)
    return Pe(path), builder


def full_builder() -> PeBuilder:
    b = PeBuilder(entry=0x1010)
    b.section(".text", b"\x90" * 0x20 + b"\xcc" * 0x10)
    b.section(".rdata", b"Linking\0*fill*\0CodeWarrior\0")
    b.section(".bss", b"", vsize=0x400)
    b.imports({"KERNEL32.dll": ["GetProcAddress", "ExitProcess"], "USER32.dll": ["LoadStringA"]})
    b.strings({1: ["Link", "", "Linking: '%c'"], 3: ["Optimizing: '%c'"]})
    b.codeview([(".text", 0x10, "_CodeGen_Generator"), (".rdata", 0x8, "_fill_literal")])
    return b


def test_headers_and_sections(c):
    pe, b = image(full_builder())
    c.check("machine", pe.machine, MACHINE_I386)
    c.check("image base", pe.image_base, 0x400000)
    c.check("entry", pe.entry, 0x1010)
    c.check("section names, in order, generated ones last", [s.name for s in pe.sections],
            [".text", ".rdata", ".bss", ".idata", ".rsrc", ".debug"])
    c.check("sections are numbered from 1 (the CodeView convention)", [s.index for s in pe.sections][:3], [1, 2, 3])
    c.check("section() by name", pe.section(".rdata").va, b.rva(".rdata"))
    c.check("section() of an absent name", pe.section(".nope"), None)
    c.check("read_rva reads file-backed bytes", pe.read_rva(b.rva(".rdata", 8), 6), b"*fill*")
    c.check("cstring at an rva", pe.cstring(b.rva(".rdata", 15)), "CodeWarrior")
    c.check("a .bss tail is not file-backed: rva2off None", pe.rva2off(b.rva(".bss", 0x10)), None)
    c.check("... read() answers None there", pe.read(b.rva(".bss", 0x10), 4), None)
    c.raises("... and read_rva refuses it", ValueError, pe.read_rva, b.rva(".bss", 0x10), 4)
    c.check("an rva past every section maps nowhere", pe.rva2off(0x900000), None)


def test_imports_and_iat(c):
    pe, b = image(full_builder())
    c.check("imports in directory order", pe.imports(),
            [("KERNEL32.dll", ["GetProcAddress", "ExitProcess"]), ("USER32.dll", ["LoadStringA"])])
    slots = pe.iat_slots()
    c.check("every by-name import has a slot", sorted(slots), ["ExitProcess", "GetProcAddress", "LoadStringA"])
    c.check("a slot is a VA (image base + the FirstThunk array entry)",
            slots["ExitProcess"] - slots["GetProcAddress"], 4)
    c.check("the first DLL's array follows the three descriptors", slots["GetProcAddress"],
            pe.image_base + pe.section(".idata").va + 3 * 20)
    for name, slot in sorted(slots.items()):
        target = int.from_bytes(pe.read_rva(slot - pe.image_base, 4), "little")
        c.check("the slot of %s points at its hint/name entry" % name, pe.cstring(target + 2), name)
    c.expect("slots lie inside .idata", pe.section(".idata").va + pe.image_base <= slots["LoadStringA"]
             < pe.section(".idata").va + pe.image_base + pe.section(".idata").vsize)
    empty, _ = image(PeBuilder().section(".text", b"\x90"))
    c.check("no import directory: no imports, no slots", (empty.imports(), empty.iat_slots()), ([], {}))


def test_resources(c):
    pe, _b = image(full_builder())
    blocks = pe.string_blocks()
    c.check("RT_STRING blocks by name id", [bid for bid, _rva, _size in blocks], [1, 3])
    rva, size = blocks[0][1], blocks[0][2]
    raw = pe.read_rva(rva, size)
    c.check("a block is length-prefixed UTF-16LE (first slot)", raw[:2 + 8], struct.pack("<H", 4) + "Link".encode("utf-16le"))
    c.check("resources carry (type, name, lang, rva, size, codepage)", [r[:3] for r in pe.resources()],
            [(6, 1, 0x409), (6, 3, 0x409)])


def test_codeview(c):
    pe, b = image(full_builder())
    c.check("CodeView symbols: (rva, name, section), sorted", pe.codeview_symbols(),
            [(b.rva(".text", 0x10), "_CodeGen_Generator", ".text"), (b.rva(".rdata", 8), "_fill_literal", ".rdata")])
    c.check("symbol_map", pe.symbol_map(), {b.rva(".text", 0x10): "_CodeGen_Generator", b.rva(".rdata", 8): "_fill_literal"})
    c.check("one debug-directory entry, type 2 (CodeView)", [e["type"] for e in pe.debug_entries()], [2])
    c.check("the blob is NB11", (pe.debug_blob() or b"")[:4], b"NB11")
    bare, _ = image(PeBuilder().section(".text", b"\x90"))
    c.check("no debug directory (every mwldeppc.exe): no entries, no symbols",
            (bare.debug_entries(), bare.debug_blob(), bare.codeview_symbols()), ([], None, []))


def test_machine(c):
    pe, _b = image(PeBuilder(machine=0x01C0, image_base=0x10000000).section(".text", b"\x90"))
    c.check("the COFF machine is read, not assumed", pe.machine, 0x01C0)
    c.check("the image base is read, not assumed", pe.image_base, 0x10000000)


def test_refusals(c):
    tmp = Path(tempfile.mkdtemp(dir=_TMP.name))
    (tmp / "a.bin").write_bytes(b"\x7fELF" + bytes(60))
    c.raises("no MZ is refused", ValueError, Pe, tmp / "a.bin")
    raw = bytearray(PeBuilder().section(".text", b"\x90").build())
    raw[0x40:0x44] = b"XX\0\0"
    (tmp / "b.exe").write_bytes(bytes(raw))
    c.raises("no PE signature is refused", ValueError, Pe, tmp / "b.exe")
    raw = bytearray(PeBuilder().section(".text", b"\x90").build())
    struct.pack_into("<H", raw, 0x58, 0x20B)
    (tmp / "c.exe").write_bytes(bytes(raw))
    c.raises("a PE32+ optional header is refused", ValueError, Pe, tmp / "c.exe")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
