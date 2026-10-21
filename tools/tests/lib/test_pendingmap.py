"""lib.pendingmap: the pending-map file format and its edits applied to a split target's symbol table (renames that
relocations follow, a row split, the refusals), with every section's bytes left as they were."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import tempfile

from tools.lib import pendingmap, testing
from tools.lib.binary.build import ElfBuilder
from tools.lib.binary.elf import STB_GLOBAL, STB_LOCAL, STT_FUNC, Elf

TIER = "fixture"


def _target() -> bytes:
    """A split-target-shaped object: two functions, a data label, an undefined callee the first one calls."""
    b = ElfBuilder()
    b.section(".text", bytes(range(0x30)), align=4)
    b.section(".data", b"\x11\x22\x33\x44", align=8)
    b.symbol("fn_80001000", ".text", 0x0, 0x20, bind="global", type="func")
    b.symbol("fn_80001020", ".text", 0x20, 0x10, bind="local", type="func")
    b.symbol("lbl_80500000", ".data", 0x0, 0x4, bind="global", type="object")
    b.symbol("fn_80009000")
    b.reloc(".text", 0x4, "fn_80009000", "R_PPC_REL24")
    b.reloc(".text", 0x24, "lbl_80500000", "R_PPC_ADDR16_HA")
    return b.comment(14).build()


def test_parse(c):
    ops = pendingmap.parse("# renames\nfn_80001000 obj_ctor\n\nrename lbl_80500000 obj_table  # data\n"
                           "split obj_ctor 0x10 obj_ctor_tail --scope local\n")
    c.check("old new, rename old new and split lines, comments and blanks skipped",
            [(o.kind, o.old, o.new, o.offset, o.scope, o.line) for o in ops],
            [("rename", "fn_80001000", "obj_ctor", 0, None, 2), ("rename", "lbl_80500000", "obj_table", 0, None, 4),
             ("split", "obj_ctor", "obj_ctor_tail", 0x10, "local", 5)])
    c.raises("a one-word line is refused", ValueError, pendingmap.parse, "fn_80001000\n")
    c.raises("a split without its offset is refused", ValueError, pendingmap.parse, "split a b\n")
    c.raises("an unknown scope is refused", ValueError, pendingmap.parse, "split a 4 b --scope odd\n")
    c.raises("a non-numeric offset is refused", ValueError, pendingmap.parse, "split a x b\n")


def test_apply_renames_and_split(c):
    raw = _target()
    before = Elf.read(raw)
    ops = pendingmap.parse("fn_80001000 obj_ctor\nfn_80009000 obj_callee\nfn_80001234 nowhere\n"
                           "split obj_ctor 0x18 obj_ctor_tail\nsplit fn_80001020 0x8 helper_tail --scope global\n")
    blob, out = pendingmap.apply(raw, ops)
    after = Elf.read(blob)
    names = {s.name: s for s in after.symbols}
    c.check("renamed: the defined row and the undefined callee", [(o, n) for o, n, _k in out.renamed],
            [("fn_80001000", "obj_ctor"), ("fn_80009000", "obj_callee")])
    c.check("a name the object neither defines nor references is absent, not refused", (out.absent, out.refused),
            (["fn_80001234"], []))
    c.check("the old names are gone", ("fn_80001000" in names, "fn_80009000" in names), (False, False))
    c.check("a relocation follows its renamed symbol", [r.symbol_name for r in after.relocs(".text")],
            ["obj_callee", "lbl_80500000"])
    c.check("split: the row shrinks to the offset", (names["obj_ctor"].value, names["obj_ctor"].size), (0, 0x18))
    tail = names["obj_ctor_tail"]
    c.check("split: the new row takes the rest, same section and type, the row's binding",
            (tail.value, tail.size, tail.section, tail.type, tail.bind), (0x18, 0x8, ".text", STT_FUNC, STB_GLOBAL))
    c.check("--scope overrides the binding of the new row",
            (names["helper_tail"].bind, names["fn_80001020"].bind, names["helper_tail"].value), (STB_GLOBAL, STB_LOCAL, 0x28))
    for sec in before.sections[1:]:
        if sec.name not in (".symtab", ".strtab"):
            c.check("%s bytes unchanged" % sec.name, after.section(sec.name).data, sec.data)
    c.check("existing symbol indices keep their place", [s.name for s in after.symbols[:len(before.symbols)]][-1],
            before.symbols[-1].name.replace("fn_80009000", "obj_callee"))
    same, nothing = pendingmap.apply(raw, pendingmap.parse("fn_80001234 nowhere\n"))
    c.check("no op that touches the object: the input bytes, unchanged", (same == raw, nothing.absent),
            (True, ["fn_80001234"]))


def test_refusals(c):
    raw = _target()
    _blob, out = pendingmap.apply(raw, pendingmap.parse(
        "fn_80001000 lbl_80500000\nsplit fn_80001020 0x10 past_end\nsplit fn_80001000 0x8 lbl_80500000\n"))
    c.check("a rename onto a name already defined, a split at or past the end, a split onto a defined name",
            [line for line, _why in out.refused], [1, 2, 3])
    c.check("refused ops change nothing", (out.renamed, out.split), ([], []))
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "unit.o")
        with open(src, "wb") as fh:
            fh.write(raw)
        c.raises("apply_file never rewrites the split object itself", ValueError, pendingmap.apply_file, src, src, [])
        dst = os.path.join(tmp, "out", "unit.pending.o")
        res = pendingmap.apply_file(src, dst, pendingmap.parse("fn_80001000 obj_ctor\n"))
        c.check("apply_file writes the copy and leaves the source", (os.path.exists(dst), open(src, "rb").read() == raw,
                                                                     res.summary().startswith("1 rename(s)")),
                (True, True, True))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
