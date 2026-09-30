#!/usr/bin/env python3
"""Fixtures-only selftest for `tools/objdiff/relocdiff.py`.

    python tools/objdiff/relocdiff_selftest.py
    python tools/objdiff/relocdiff.py --selftest

Two halves, both offline:

* the **pure rule** (`diff_relocs`) driven with plain tuples - the four classes named in the tool's
  contract (a target-only, b ours-only, c same-offset-different-symbol, d same-symbol-different-
  type/addend), the identical answer, the moved-offset case that must NOT be read as a symbol change,
  and the multiset identity that keeps an unchanged relocation out of the diff;
* the **reader** (`read_relocs`) against ELF32 big-endian fixtures written by this file's builder
  (`sectiongap_selftest.build_elf`, extended to carry a RELA addend), so a symbol index resolves to a
  name, the `.rela<target>` section maps to the section it relocates, and the addend survives the read.

No build, no `ninja`, no repository state.
"""
from __future__ import annotations

import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for _p in (HERE, os.path.join(ROOT, "tools"), os.path.join(ROOT, "tools", "units")):
    if _p not in sys.path:
        sys.path.insert(0, _p)

import relocdiff as rd             # noqa: E402
import sectiongap_selftest as sgs  # noqa: E402  (the project's ELF32 fixture builder)

CHECKS = 0
FAILURES: list[str] = []


def check(name: str, got, want) -> None:
    global CHECKS
    CHECKS += 1
    if got != want:
        FAILURES.append("%s: got %r, want %r" % (name, got, want))


R_REL24, R_ADDR32 = 10, 1


def test_pure() -> None:
    """The four classes, the identical answer, the moved offset, and the multiset identity."""
    same = [(0x0, "fn_A", R_REL24, 0), (0x10, "fn_B", R_ADDR32, 0)]
    d = rd.diff_relocs(same, list(same))
    check("identical sets are identical", d["identical"], True)
    check("... with every relocation matched", d["matched"], 2)
    check("... and nothing in the four classes",
          [d["only_target"], d["only_ours"], d["different_symbol"], d["different_attr"]],
          [[], [], [], []])

    # (a) the target relocates something we do not
    d = rd.diff_relocs(same, same + [(0x20, "fn_C", R_REL24, 0)])
    check("(a) a target-only relocation is only_target", d["only_target"], [(0x20, "fn_C", R_REL24, 0)])
    check("(a) ... and not ours-only", d["only_ours"], [])
    check("(a) the sets are no longer identical", d["identical"], False)

    # (b) the mirror image
    d = rd.diff_relocs(same + [(0x20, "fn_C", R_REL24, 0)], same)
    check("(b) an ours-only relocation is only_ours", d["only_ours"], [(0x20, "fn_C", R_REL24, 0)])
    check("(b) ... and not target-only", d["only_target"], [])

    # (c) the same offset pointing at a different symbol - the objdiff-invisible class
    d = rd.diff_relocs([(0x0, "@45", 109, 0)], [(0x0, "lbl_80791124", 109, 0)])
    check("(c) a different symbol at one offset is different_symbol",
          [(off, a[0], b[0]) for off, a, b in d["different_symbol"]], [(0x0, "@45", "lbl_80791124")])
    check("(c) ... and is reported once, not as an add plus a remove",
          [d["only_target"], d["only_ours"]], [[], []])

    # (d) same symbol, different type; and same symbol, different addend
    d = rd.diff_relocs([(0x66, "cb", 6, 0)], [(0x66, "cb", 4, 0)])
    check("(d) a changed type is different_attr",
          [(off, a[1], b[1]) for off, a, b in d["different_attr"]], [(0x66, 6, 4)])
    check("(d) a changed type is not a symbol change", d["different_symbol"], [])
    d = rd.diff_relocs([(0x66, "cb", 6, 4)], [(0x66, "cb", 6, 0)])
    check("(d) a changed addend is different_attr",
          [(off, a[2], b[2]) for off, a, b in d["different_attr"]], [(0x66, 4, 0)])

    # a relocation that MOVED must read as a remove plus an add, never as a symbol change
    d = rd.diff_relocs([(0x14, "__dl__FPv", R_ADDR32, 0)], [(0x2C, "__dl__FPv", R_ADDR32, 0)])
    check("a moved relocation is not a (c)", d["different_symbol"], [])
    check("... it is one ours-only and one target-only",
          (d["only_ours"], d["only_target"]),
          ([(0x14, "__dl__FPv", R_ADDR32, 0)], [(0x2C, "__dl__FPv", R_ADDR32, 0)]))

    # multiset identity: a duplicated identical relocation stays out of the diff
    d = rd.diff_relocs([(0x0, "fn_A", R_REL24, 0)] * 2, [(0x0, "fn_A", R_REL24, 0)] * 2)
    check("duplicate identical relocations still count as identical", d["identical"], True)
    check("... with both matched", d["matched"], 2)

    check("type_name names a known kind", rd.type_name(R_REL24), "R_PPC_REL24")
    check("type_name names EMB SDA21", rd.type_name(109), "R_PPC_EMB_SDA21")
    check("type_name stays numeric for an unlisted kind", rd.type_name(250), "R_PPC_250")


def _text_elf(sym_index: int, typ: int = R_REL24, addend: int = 0, offset: int = 0) -> bytes:
    """One `.text` section with a single relocation to symbol `sym_index`."""
    return sgs.build_elf(
        content=[(".text", b"\x48\x00\x00\x01" * 4)],
        symbols=[("fn_A", 1, 16), ("target_callee", 0, 0), ("our_callee", 0, 0)],
        relocs=[(".text", offset, typ, sym_index, addend)])


def test_reader() -> None:
    """`read_relocs` resolves the symbol name, the section it relocates, and the addend."""
    with tempfile.TemporaryDirectory() as tmp:
        ours = os.path.join(tmp, "ours.o")
        target = os.path.join(tmp, "target.o")
        with open(ours, "wb") as fh:
            fh.write(_text_elf(sym_index=3, addend=4))       # our_callee, addend 4
        with open(target, "wb") as fh:
            fh.write(_text_elf(sym_index=100, addend=4))     # an out-of-range index -> "?"
        rels, err = rd.read_relocs(ours)
        check("read_relocs finds the .rela.text section", sorted(rels or {}), [".text"])
        check("... names the symbol by index", rels[".text"], [(0x0, "our_callee", R_REL24, 4)])
        check("... and carries the addend", rels[".text"][0][3], 4)

        rels2, _err = rd.read_relocs(target)
        # an index with no symbol row must not be guessed - it is blank, not a name
        check("an unresolvable symbol index is blank, not guessed", rels2[".text"][0][1], "")

        # the same fixture on both sides is relocation-identical
        with tempfile.TemporaryDirectory() as tmp2:
            a, b = os.path.join(tmp2, "a.o"), os.path.join(tmp2, "b.o")
            for p in (a, b):
                with open(p, "wb") as fh:
                    fh.write(_text_elf(sym_index=3, addend=0))
            ra, _ = rd.read_relocs(a)
            rb, _ = rd.read_relocs(b)
            check("two identical fixtures diff clean",
                  rd.diff_relocs(ra[".text"], rb[".text"])["identical"], True)

        # the fixture pair that differs only in the symbol of one relocation is class (c)
        na, _ = rd.read_relocs(ours)
        tb = os.path.join(tmp, "target2.o")
        with open(tb, "wb") as fh:
            fh.write(_text_elf(sym_index=2, addend=4))       # target_callee
        nt, _ = rd.read_relocs(tb)
        d = rd.diff_relocs(na[".text"], nt[".text"])
        check("a different symbol at one offset is (c) over the fixture",
              [(o, x[0], y[0]) for o, x, y in d["different_symbol"]],
              [(0x0, "our_callee", "target_callee")])

        # a missing file is an error naming it, never an empty (falsely clean) object
        missing, err = rd.read_relocs(os.path.join(tmp, "nope.o"))
        check("a missing object is (None, why)", missing, None)
        check("... and the reason names the path", "nope.o" in (err or ""), True)

        # a non-ELF file is refused, not parsed as empty
        junk = os.path.join(tmp, "junk.o")
        with open(junk, "wb") as fh:
            fh.write(b"not an object")
        got, err = rd.read_relocs(junk)
        check("a non-ELF is refused", got, None)
        check("... with a reason", bool(err), True)


def test_gate() -> None:
    """`unit_identical` / `--check`'s verdict: an error is not 'identical'."""
    ok = {"error": None, "sections": [{"diff": {"identical": True}}]}
    bad = {"error": None, "sections": [{"diff": {"identical": False}}]}
    check("identical sections are identical", rd.unit_identical(ok), True)
    check("one differing section is not", rd.unit_identical(bad), False)
    check("an error is not identical", rd.unit_identical({"error": "boom", "sections": []}), False)


def test_by_owner() -> None:
    """`compare_by_owner`: identical, wrong callee name, wrong addend, shifted function, slid instruction."""
    import undefrefs_selftest as us
    func = (1 << 4) | 2

    def obj(pad=0, callee="callee_a", addend=0, r1=4, r2=24):
        syms = [("f", 16, ".text", func, pad), ("g", 16, ".text", func, pad + 16),
                (callee, 0, None, 0x10, 0), ("callee_b", 0, None, 0x10, 0)]
        return us.build_obj([(".text", b"\0" * (pad + 32))], syms,
                            [(".text", pad + r1, callee, R_REL24, addend),
                             (".text", pad + r2, "callee_b", R_REL24, 0)])

    base = obj()
    check("by-owner: identical", rd.compare_by_owner(obj(), base), (2, 2, []))
    m, _t, lines = rd.compare_by_owner(obj(callee="callee_x"), base)
    check("by-owner: wrong callee is one line naming both",
          (m, len(lines), "callee_x vs callee_a" in lines[0], "f+0x4" in lines[0]), (1, 1, True, True))
    m, _t, lines = rd.compare_by_owner(obj(addend=8), base)
    check("by-owner: wrong addend", (m, len(lines), "addend +8 vs +0" in lines[0]), (1, 1, True))
    check("by-owner: a shifted function still pairs", rd.compare_by_owner(obj(pad=16), base), (2, 2, []))
    m, _t, lines = rd.compare_by_owner(obj(r1=8), base)
    check("by-owner: a slid instruction is a non-failing note",
          (m, len(lines), lines[0].startswith("note")), (2, 1, True))
    one = us.build_obj([(".text", b"\0" * 32)], [("f", 16, ".text", func, 0), ("callee_a", 0, None, 0x10, 0)],
                       [(".text", 4, "callee_a", R_REL24, 0)])
    _m, _t, lines = rd.compare_by_owner(one, base)
    check("by-owner: a symbol absent from ours is one line",
          (len(lines), "only in target" in lines[0]), (1, True))


def selftest() -> int:
    test_pure()
    test_reader()
    test_gate()
    test_by_owner()
    for failure in FAILURES:
        print("FAIL " + failure)
    print("ok - %d checks" % CHECKS)
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(selftest())
