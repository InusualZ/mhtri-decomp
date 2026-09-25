#!/usr/bin/env python3
"""Self-test for `tools/units/relocaudit.py`.

Three things here can silently make the sweep lie, so each gets its own block of checks:

* the **linkage rule** - `linkage_stem` must see MWCC's `__F`/`__Q` argument list and nothing else
  (`__start`, `_savegpr_20`, `lbl_8058B290` are C/EABI spellings and must not be split);
* the **classifier** - a name our object emits that the target spells differently must come back as a
  **linkage** row, a name the target only ever references must not be a mismatch at all, and a target
  definition must satisfy one of our references (a partial unit may call a sibling in the same TU);
* the **object reader** - against synthetic ELF32 objects written here, so the exclusions (`STT_FILE`,
  `@NNN`, locals) are tested directly, and the whole sweep is driven end to end over a scratch tree
  whose fixture emits `drawSpr2TF` where the target has `drawSpr2TF__FUcP9fltSpr2TFUc` - the exact
  shape of the landed `fn_80059550` defect, constructed fresh rather than reintroduced anywhere.

The real `src/`, `configure.py`, `symbols.txt` and `build/` are never read or written: every fixture
lives in a temp directory, so the selftest is green on a tree with no build.

    python tools/units/relocaudit_selftest.py
    python tools/units/relocaudit.py --selftest
"""

from __future__ import annotations

import os
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import relocaudit as ra  # noqa: E402

# info byte = (bind << 4) | type
GLOBAL_FUNC = (ra.STB_GLOBAL << 4) | 2
GLOBAL_OBJ = (ra.STB_GLOBAL << 4) | 1
GLOBAL_NOTYPE = (ra.STB_GLOBAL << 4) | 0
LOCAL_NOTYPE = (ra.STB_LOCAL << 4) | 0
LOCAL_FILE = (ra.STB_LOCAL << 4) | ra.STT_FILE
SHN_ABS = 0xFFF1


# --------------------------------------------------------------------------------------------------
# a minimal ELF32 big-endian object with the sections the reader needs
# --------------------------------------------------------------------------------------------------
def write_elf(path: str, symbols: list[tuple], text: bytes = b"\x10\x00\x00\x00" * 4) -> str:
    """Write an ELF32 BE object with `.text`, `.symtab`, `.strtab`, `.shstrtab`.

    `symbols` is `[(name, value, size, info, shndx)]`; an empty first tuple is the required null row.
    """
    strtab, offsets = b"\x00", []
    for name, *_ in symbols:
        offsets.append(len(strtab))
        strtab += name.encode("latin-1") + b"\x00"
    symtab = b""
    for (name, value, size, info, shndx), off in zip(symbols, offsets):
        symtab += struct.pack(">IIIBBH", off if name else 0, value, size, info, 0, shndx)
    sections = [(".text", 1, text), (".symtab", 2, symtab), (".strtab", 3, strtab)]
    shstr, shname_off = b"\x00", []
    for name, *_ in sections:
        shname_off.append(len(shstr))
        shstr += name.encode("latin-1") + b"\x00"
    shstrtab_off = len(shstr)
    shstr += b".shstrtab\x00"
    shnum = len(sections) + 2
    ehsize, shentsize = 52, 40
    body, body_off = b"", []
    for _name, _type, data in sections:
        body_off.append(len(body))
        body += data
    shstr_off = len(body)
    body += shstr
    shoff = ehsize + len(body)
    ehdr = (b"\x7fELF\x01\x02\x01" + b"\x00" * 9 +
            struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, ehsize, 0, 0,
                        shentsize, shnum, shnum - 1))
    shdrs = struct.pack(">10I", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    for i, (name, typ, data) in enumerate(sections):
        link = 3 if name == ".symtab" else 0
        shdrs += struct.pack(">10I", shname_off[i], typ, 0, 0, ehsize + body_off[i], len(data),
                             link, 0, 1, 0)
    shdrs += struct.pack(">10I", shstrtab_off, 3, 0, 0, ehsize + shstr_off, len(shstr), 0, 0, 1, 0)
    with open(path, "wb") as fh:
        fh.write(ehdr + body + shdrs)
    return path


def fixture(our_undefined, tgt_undefined, our_defined=(), tgt_defined=()):
    """Synthetic symbol rows for an "our" object and a "target" object.

    A `STT_FILE` row, an `@176` compiler label and a local `...data.0` row ride along in every object
    so the reader's exclusions are exercised on both sides.
    """
    def rows(undefined, defined):
        syms = [("", 0, 0, 0, 0),
                ("unit.c", 0, 0, LOCAL_FILE, SHN_ABS),
                ("@176", 0, 0, LOCAL_NOTYPE, 1),
                ("...data.0", 0, 4, LOCAL_NOTYPE, 1)]
        for name in defined:
            syms.append((name, 0, 16, GLOBAL_FUNC, 1))
        for name in undefined:
            syms.append((name, 0, 0, GLOBAL_NOTYPE, ra.SHN_UNDEF))
        return syms
    return rows(our_undefined, our_defined), rows(tgt_undefined, tgt_defined)


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # -- the linkage rule ----------------------------------------------------------------------
    check("stem strips a C++ argument list",
          ra.linkage_stem("drawSpr2TF__FUcP9fltSpr2TFUc"), "drawSpr2TF")
    check("stem strips a class-qualified argument list",
          ra.linkage_stem("Panic__Q24nw4r2dbFPCciPCce"), "Panic")
    check("stem strips a long EF spelling",
          ra.linkage_stem("get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3"),
          "get_joint_wpos_em")
    check("a plain map name is its own stem", ra.linkage_stem("fn_80059550"), "fn_80059550")
    check("a lbl_ name is its own stem", ra.linkage_stem("lbl_8058B290"), "lbl_8058B290")
    for plain in ("__start", "_savegpr_20", "__init_data", "@176"):
        check("not a mangling: %s" % plain, ra.linkage_stem(plain), plain)

    # EABI register-save helpers encode register allocation, not linkage, so they are tagged and kept
    # out of the linkage repair list
    for helper in ("_savegpr_20", "_restgpr_17", "_savefpr_31", "_restfpr_14"):
        check("compiler helper: %s" % helper, ra.mismatch_kind(helper), "compiler-helper")
    for extra in ("GXSetTexCoordGen2", "fn_80501EE0", "lbl_80629B90"):
        check("not a compiler helper: %s" % extra, ra.mismatch_kind(extra), "extra")

    # -- the classifier ------------------------------------------------------------------------
    # clean: our reference is a target reference; the target's extra references are a partial unit
    r = ra.audit_sets(set(), {"a"}, set(), {"a", "b"})
    check("subset of the target's references is clean",
          (r["linkage_undefined"], r["other_undefined"], r["linkage_defined"], r["other_defined"]),
          ([], [], [], []))
    # the landed defect seen from our side: ours plain, target mangled -> linkage
    r = ra.audit_sets(set(), {"drawSpr2TF"}, set(), {"drawSpr2TF__FUcP9fltSpr2TFUc"})
    check("plain where the target is mangled is a linkage row",
          r["linkage_undefined"], [{"our": "drawSpr2TF", "target": ["drawSpr2TF__FUcP9fltSpr2TFUc"]}])
    check("and is not an other row", r["other_undefined"], [])
    # the reverse: ours mangled, target plain -> linkage
    r = ra.audit_sets(set(), {"fn_80043EA8__FP4Vec3"}, set(), {"fn_80043EA8"})
    check("mangled where the target is plain is a linkage row",
          r["linkage_undefined"], [{"our": "fn_80043EA8__FP4Vec3", "target": ["fn_80043EA8"]}])
    # a target *definition* satisfies one of our references (a sibling in the same original TU)
    r = ra.audit_sets(set(), {"helper"}, {"helper"}, set())
    check("a target definition satisfies our reference",
          (r["linkage_undefined"], r["other_undefined"]), ([], []))
    # a name the target never mentions is an other row, not a linkage one
    r = ra.audit_sets(set(), {"mystery_fn"}, set(), {"other_fn"})
    check("a name the target never mentions is an other row", r["other_undefined"],
          [{"our": "mystery_fn", "target": []}])
    check("and is not a linkage row", r["linkage_undefined"], [])
    # defined direction
    r = ra.audit_sets({"TPLtexLoad"}, set(), {"TPLtexLoad__FPvP9_tex_info"}, set())
    check("a definition with the wrong linkage is a defined linkage row",
          r["linkage_defined"], [{"our": "TPLtexLoad", "target": ["TPLtexLoad__FPvP9_tex_info"]}])
    r = ra.audit_sets({"brand_new"}, set(), set(), set())
    check("a definition the target never names is an other defined row", r["other_defined"],
          [{"our": "brand_new", "target": []}])

    # -- the object reader + the whole sweep, over a scratch tree ------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        # 1) the reader's exclusions, on a single fixture
        p = write_elf(os.path.join(tmp, "one.o"), fixture(
            ["drawSpr2TF", "get_ScreenSize__FP8_MH_VEC2"],
            ["drawSpr2TF__FUcP9fltSpr2TFUc", "get_ScreenSize__FP8_MH_VEC2"],
            our_defined=["main_fn"], tgt_defined=["main_fn"])[0])
        defined, undefined = ra.object_sets(p)
        check("reader keeps global definitions", "main_fn" in defined, True)
        check("reader drops the STT_FILE row", "unit.c" in defined or "unit.c" in undefined, False)
        check("reader drops @NNN labels", "@176" in defined or "@176" in undefined, False)
        check("reader drops local data rows", "...data.0" in defined or "...data.0" in undefined, False)
        check("reader reports undefined names", sorted(undefined),
              ["drawSpr2TF", "get_ScreenSize__FP8_MH_VEC2"])
        check("reader is None on a non-ELF", ra.object_sets(os.path.join(tmp, "nope.o")), None)
        with open(os.path.join(tmp, "junk.o"), "wb") as fh:
            fh.write(b"not an ELF\n")
        check("reader is None on junk", ra.object_sets(os.path.join(tmp, "junk.o")), None)
        blob = open(p, "rb").read()
        with open(os.path.join(tmp, "trunc.o"), "wb") as fh:
            fh.write(blob[:60])          # valid magic + header, section table cut off
        check("reader is None on a truncated object", ra.object_sets(os.path.join(tmp, "trunc.o")), None)

        # 2) end to end: a scratch repo whose OUR object emits the wrong linkage. This is the case
        #    that must FAIL - the sweep has to report it as a suspect, not pass it.
        for sub in ("build/RMHE08/src", "build/RMHE08/obj"):
            os.makedirs(os.path.join(tmp, sub))
        with open(os.path.join(tmp, "configure.py"), "w") as fh:
            fh.write('config.libs = [\n    Object(NonMatching, "unit.c"),\n]\n')
        our_syms, tgt_syms = fixture(
            ["drawSpr2TF", "get_ScreenSize__FP8_MH_VEC2"],
            ["drawSpr2TF__FUcP9fltSpr2TFUc", "get_ScreenSize__FP8_MH_VEC2"],
            our_defined=["main_fn"], tgt_defined=["main_fn", "helper"])
        write_elf(os.path.join(tmp, "build/RMHE08/src/unit.o"), our_syms)
        write_elf(os.path.join(tmp, "build/RMHE08/obj/unit.o"), tgt_syms)
        s = ra.sweep(tmp, with_decls=False)
        check("sweep sees the one registered unit", s["units_total"], 1)
        check("the wrong-linkage fixture is a suspect", len(s["suspects"]), 1)
        rec = s["suspects"][0]
        check("the mismatch is a linkage row, not an other row",
              (len(rec["linkage_undefined"]), len(rec["other_undefined"])), (1, 0))
        check("the linkage row names both spellings",
              (rec["linkage_undefined"][0]["our"], rec["linkage_undefined"][0]["target"]),
              ("drawSpr2TF", ["drawSpr2TF__FUcP9fltSpr2TFUc"]))
        check("the target definition of helper is not reported",
              [m["our"] for m in rec["linkage_undefined"] + rec["other_undefined"]], ["drawSpr2TF"])
        check("the clean counts are right", (s["clean"], s["undefined_suspects"]), (0, 1))

        # 3) the same tree with the linkage repaired is clean - proves the failure above is the
        #    symbol, not the fixture
        our_fixed, _ = fixture(
            ["drawSpr2TF__FUcP9fltSpr2TFUc", "get_ScreenSize__FP8_MH_VEC2"],
            [], our_defined=["main_fn"])
        write_elf(os.path.join(tmp, "build/RMHE08/src/unit.o"), our_fixed)
        s = ra.sweep(tmp, with_decls=False)
        check("the repaired fixture is clean", (s["clean"], len(s["suspects"])), (1, 0))

        # 4) an unbuilt unit is named, never silently passed
        os.remove(os.path.join(tmp, "build/RMHE08/src/unit.o"))
        s = ra.sweep(tmp, with_decls=False)
        check("a missing object is unbuilt, not clean",
              (s["units_built"], s["clean"], len(s["unbuilt"])), (0, 0, 1))
        check("the missing path is named", s["unbuilt"][0]["missing"],
              ["build/RMHE08/src/unit.o"])

        # 5) the declaration lookup finds the owner header the repair has to edit
        os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
        os.makedirs(os.path.join(tmp, "include"), exist_ok=True)
        with open(os.path.join(tmp, "src", "unit.c"), "w") as fh:
            fh.write("void f(void) {}\n")
        with open(os.path.join(tmp, "include", "owner.h"), "w") as fh:
            fh.write("extern \"C\" void drawSpr2TF(int a);\n")
        idx = ra.declaration_index(tmp)
        decls = ra.declarations_for(idx, "drawSpr2TF__FUcP9fltSpr2TFUc")
        check("declaration lookup uses the linkage stem",
              [(d["file"], d["line"]) for d in decls], [("include/owner.h", 1)])

    print("relocaudit selftest: %d checks, %d failed" % (checks, len(fails)))
    for f in fails:
        print("  FAIL %s" % f)
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(selftest())
