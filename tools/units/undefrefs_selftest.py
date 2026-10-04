#!/usr/bin/env python3
"""Deterministic self-test for `tools/units/undefrefs.py` - the flip blocker a score cannot see.

    python tools/units/undefrefs_selftest.py
    python tools/units/undefrefs.py --selftest

No build and no repository state: every object is a fixture ELF32 big-endian image written by this file, so
the contract is pinned on its own means. The two catches of the incident are fixtures here - the wrong
mangled struct tag (same relocation *offset* in the target) and the C-linkage spelling (the target's
mangled name found by the *stem* when the layouts differ) - and both must name the two spellings. The
negative fixture is the Pat unit's shape: wrong slots that point at real functions which *are*
`symbols.txt` rows must stay silent. A clean unit (defined here, mapped, another provider, the target's own
unresolved reference, the linker's own symbol) is silent too. The batch path is pinned end to end: a
candidate decides the cached index is built, a clean batch does not, and the index survives in
`build/tmp/undefrefs/link-symbols.json`.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import shutil
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))

from tools.units import undefrefs as ur

SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4


def _align(n: int, a: int = 4) -> int:
    return (n + a - 1) // a * a


def build_obj(sections, symbols, relocs=()) -> bytes:
    """A minimal ELF32 big-endian object (no `.comment`; this check never reads one).

    sections : [(name, data)]                        PROGBITS sections, in shndx order
    symbols  : [(name, size, section, info, value)]  the null symbol is implicit; `section is None` is an
                                                     undefined reference
    relocs   : [(target_section, offset, name)]      become `.rela<target>` (SHT_RELA) sections
    """
    syms = [(None, 0, None, 0, 0)] + [(s[0], s[1], s[2], s[3], s[4] if len(s) > 4 else 0) for s in symbols]
    rela_targets = []
    for rel in relocs:
        target = rel[0]
        if target not in rela_targets:
            rela_targets.append(target)
    sec_names = ([""] + [n for n, _ in sections] + [".rela" + t for t in rela_targets]
                 + [".symtab", ".strtab", ".shstrtab"])
    index = {n: i for i, n in enumerate(sec_names)}

    strtab = bytearray(b"\0")
    name_off = {None: 0}
    for name, _size, _section, _info, _value in syms:
        if name is not None:
            name_off[name] = len(strtab)
            strtab += name.encode() + b"\0"

    sym_index = {s[0]: i for i, s in enumerate(syms)}
    symtab = bytearray()
    for name, size, section, info, value in syms:
        symtab += struct.pack(">IIIBBH", name_off.get(name, 0), value, size, info, 0,
                              index.get(section, 0) if section else 0)

    rela_data = {t: bytearray() for t in rela_targets}
    for rel in relocs:
        target, offset, name = rel[:3]
        rtype, addend = (rel[3], rel[4]) if len(rel) > 3 else (0, 0)
        rela_data[target] += struct.pack(">IIi", offset, (sym_index[name] << 8) | rtype, addend)

    data = {n: d for n, d in sections}
    for t in rela_targets:
        data[".rela" + t] = bytes(rela_data[t])
    data[".symtab"] = bytes(symtab)
    data[".strtab"] = bytes(strtab)
    shstr = bytearray(b"\0")
    sh_name = {}
    for n in sec_names:
        sh_name[n] = len(shstr)
        shstr += n.encode() + b"\0"
    data[".shstrtab"] = bytes(shstr)

    placed = []
    off = 52
    for n in sec_names:
        if n == "":
            placed.append((n, 0, 0))
            continue
        off = _align(off)
        placed.append((n, off, len(data[n])))
        off += len(data[n])
    shoff = _align(off)
    buf = bytearray(shoff + 40 * len(sec_names))
    struct.pack_into(">4sBBBBB7s", buf, 0, b"\x7fELF", 1, 2, 1, 0, 0, b"\0" * 7)
    struct.pack_into(">HHIIIIIHHHHHH", buf, 0x10, 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0, 40,
                     len(sec_names), len(sec_names) - 1)
    for n, o, size in placed:
        if n:
            buf[o:o + size] = data[n]
    for i, (n, o, size) in enumerate(placed):
        if n == "":
            continue
        if n.startswith(".rela"):
            typ, link, info = SHT_RELA, 0, index[n[5:]]      # sh_info = the section the relocs apply to
        elif n == ".symtab":
            typ, link, info = SHT_SYMTAB, index[".strtab"], 0  # sh_link = its string table
        elif n in (".strtab", ".shstrtab"):
            typ, link, info = SHT_STRTAB, 0, 0
        else:
            typ, link, info = SHT_PROGBITS, 0, 0
        entsize = 12 if typ == SHT_RELA else 16 if typ == SHT_SYMTAB else 0
        struct.pack_into(">IIIIIIIIII", buf, shoff + i * 40, sh_name[n], typ, 0, 0, o, size, link, info, 4,
                         entsize)
    return bytes(buf)


def write(root: str, rel: str, body: bytes) -> str:
    path = os.path.join(root, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(body)
    return path


# --- the checks --------------------------------------------------------------------------------------

FAILURES: list[str] = []


def expect(label: str, got, want) -> None:
    if got != want:
        FAILURES.append("%s\n      got:  %r\n      want: %r" % (label, got, want))
        print("FAIL  %s" % label)
    else:
        print("ok    %s" % label)


TARGET_REL = os.path.normpath(os.path.join("build", "RMHE08", "obj", "quest", "arenatask.o"))
FUNC = 0x12          # global FUNC
UNDEF = 0x10         # global NOTYPE, undefined


def obj(path: str):
    return ur.load_object(path)


def check(our, target, map_set, providers=None, ref_count=None, linker=None, target_rel=TARGET_REL,
          base_names=frozenset()):
    return ur.check_object("quest/arenatask", our, target, map_set=map_set, providers=providers or {},
                           ref_count=ref_count or {}, target_rel=target_rel, linker_set=linker or set(),
                           base_names=base_names)[0]


def check_pair(our, target, map_set, base_names=frozenset(), providers=None, ref_count=None,
               linker=None, target_rel=TARGET_REL):
    return ur.check_object("quest/arenatask", our, target, map_set=map_set, providers=providers or {},
                           ref_count=ref_count or {}, target_rel=target_rel, linker_set=linker or set(),
                           base_names=base_names)


def selftest() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        # 1. The wrong mangled struct tag: `ArenaEqData` (11) vs `_arena_eq_data` (14). Our object
        #    relocates the wrong spelling at the same `.text` offset the target relocates the right one;
        #    the map carries only the target's spelling, nothing provides either. The refusal must name
        #    both spellings - that is the fix - and the "same offset" path is the one that finds it.
        wrong = "dl_acdata_to_ar_eqdata__FP11ArenaEqDataUc"
        right = "dl_acdata_to_ar_eqdata__FP14_arena_eq_dataUc"
        ours = write(tmp, "wrong_tag_ours.o", build_obj(
            [(".text", b"\0" * 0x40)],
            [("arena_eqdata_from_userdata", 0x2FC, ".text", FUNC, 0), (wrong, 0, None, UNDEF)],
            relocs=[(".text", 0x20, wrong)]))
        tgt = write(tmp, "wrong_tag_tgt.o", build_obj(
            [(".text", b"\0" * 0x40)],
            [("arena_eqdata_from_userdata", 0x2FC, ".text", FUNC, 0), (right, 0, None, UNDEF)],
            relocs=[(".text", 0x20, right)]))
        found = check(obj(ours), obj(tgt), {right})
        expect("wrong tag: one refusal", len(found), 1)
        expect("wrong tag: names our wrong spelling", wrong in found[0], True)
        expect("wrong tag: names the target's spelling", right in found[0], True)
        expect("wrong tag: says how it found it", "same offset" in found[0], True)
        expect("wrong tag: a name the map carries is silent",
               check(obj(tgt), obj(tgt), {right}), [])   # switching to the right spelling clears it
        # "before -> after" is the shape both fixtures have: the defective object is refused and the
        # corrected one is silent. (Before this row existed both objects scored the same and neither was
        # seen; the fixture is the regression test for that.)

        # 2. The C-linkage spelling: our object calls `get_move_work_adrs`, the target's relocation is
        #    `get_move_work_adrs__FUc` and - like `hud/cockpit_quest`'s partial object against the full
        #    original TU - at a different offset, so only the linkage-stem path can name it.
        c_wrong, c_right = "get_move_work_adrs", "get_move_work_adrs__FUc"
        ours2 = write(tmp, "clink_ours.o", build_obj(
            [(".text", b"\0" * 0x40)],
            [("cockpit_body", 0x30, ".text", FUNC, 0), (c_wrong, 0, None, UNDEF)],
            relocs=[(".text", 0x20, c_wrong)]))
        tgt2 = write(tmp, "clink_tgt.o", build_obj(
            [(".text", b"\0" * 0x60)],
            [("cockpit_body", 0x30, ".text", FUNC, 0), (c_right, 0, None, UNDEF)],
            relocs=[(".text", 0x44, c_right)]))          # different offset on purpose
        found2 = check(obj(ours2), obj(tgt2), {c_right})
        expect("C linkage: one refusal", len(found2), 1)
        expect("C linkage: names both spellings",
               (c_wrong in found2[0], c_right in found2[0]), (True, True))
        expect("C linkage: found by the stem", "same stem" in found2[0], True)
        expect("C linkage: the corrected spelling passes", check(obj(tgt2), obj(tgt2), {c_right}), [])

        # 2a. ADD-ONLY (a): a unit whose ONLY wrong reference is pre-existing must PASS. This is the
        #     regression test for the whole change - the C-linkage object above, with the base snapshot
        #     carrying its wrong spelling, is debt the batch did not create.
        problems_a, line_a = check_pair(obj(ours2), obj(tgt2), {c_right}, base_names={c_wrong})
        expect("add-only (a): a pre-existing-only unit passes", problems_a, [])
        expect("add-only (a): the debt is reported, naming the unit and the count",
               (line_a is not None, "1 pre-existing" in (line_a or ""), c_wrong in (line_a or "")),
               (True, True, True))

        # 2b. ADD-ONLY (b): one pre-existing and one newly-wrong reference -> refuse, naming ONLY the new.
        new_wrong = "brand_new_undefined"
        ours2b = write(tmp, "clink_ours_b.o", build_obj(
            [(".text", b"\0" * 0x40)],
            [("cockpit_body", 0x30, ".text", FUNC, 0), (c_wrong, 0, None, UNDEF), (new_wrong, 0, None, UNDEF)],
            relocs=[(".text", 0x20, c_wrong), (".text", 0x24, new_wrong)]))
        problems_b, line_b = check_pair(obj(ours2b), obj(tgt2), {c_right}, base_names={c_wrong})
        expect("add-only (b): one refusal", len(problems_b), 1)
        expect("add-only (b): names the new one", new_wrong in problems_b[0], True)
        expect("add-only (b): never names the pre-existing one in the refusal", c_wrong in problems_b[0], False)
        expect("add-only (b): reports the pre-existing one separately",
               (line_b is not None, c_wrong in (line_b or ""), "1 pre-existing" in (line_b or "")),
               (True, True, True))
        # (c) the existing wrong-tag and C-linkage fixtures above still refuse when the batch *introduces*
        #     them: their `check` calls pass no base, so every reference is new.

        # 3. The negative fixture - the Pat vtable's shape. 62 wrong slots point at real functions that ARE
        #    `symbols.txt` rows; the bytes are identical and the relocation *names* are real, so this must
        #    stay silent. (The vtable case is a `.data` relocation to a function, not `.text` code.)
        real = ["NetworkSessionManagerPat_dtor", "__ct__12PatInterfaceFv", "fn_803D3CE8", "Pat_GetKey"]
        ours3 = write(tmp, "pat_ours.o", build_obj(
            [(".data", b"\0" * 0x10)],
            [("Pat_vtable", 0x10, ".data", 0x11, 0)] + [(n, 0, None, UNDEF) for n in real],
            relocs=[(".data", 0, real[0]), (".data", 4, real[1]),
                    (".data", 8, real[2]), (".data", 0xC, real[3])]))
        tgt3 = write(tmp, "pat_tgt.o", build_obj(
            [(".data", b"\0" * 0x10)],
            [("Pat_vtable", 0x10, ".data", 0x11, 0)] + [(n, 0, None, UNDEF) for n in real],
            relocs=[(".data", 0, real[0]), (".data", 8, real[1]),
                    (".data", 4, real[2]), (".data", 0xC, real[3])]))    # same set, different slots
        expect("Pat negative fixture: wrong slots to mapped names pass", check(obj(ours3), obj(tgt3), set(real)), [])

        # 4. A clean unit: a name our object defines, a map row, another input's provider, the target's own
        #    unresolved reference, the linker's symbol, and a name the link already references unresolved.
        ours4 = write(tmp, "clean_ours.o", build_obj(
            [(".text", b"\0" * 0x20)],
            [("body", 0x20, ".text", FUNC, 0), ("defined_here", 4, ".text", FUNC, 0x1C),
             ("mapped_name", 0, None, UNDEF), ("provided_elsewhere", 0, None, UNDEF),
             ("target_only", 0, None, UNDEF), ("linker_name", 0, None, UNDEF),
             ("relay_only", 0, None, UNDEF)],
            relocs=[(".text", 0, "defined_here"), (".text", 4, "mapped_name"),
                    (".text", 8, "provided_elsewhere"), (".text", 0xC, "target_only"),
                    (".text", 0x10, "linker_name"), (".text", 0x14, "relay_only")]))
        tgt4 = write(tmp, "clean_tgt.o", build_obj(
            [(".text", b"\0" * 0x20)],
            [("body", 0x20, ".text", FUNC, 0), ("target_only", 0, None, UNDEF)],
            relocs=[(".text", 4, "target_only")]))
        expect("clean unit is silent", check(
            obj(ours4), obj(tgt4), {"mapped_name"},
            providers={"provided_elsewhere": ["build/RMHE08/obj/other.o"]},
            ref_count={"relay_only": 1}, linker={"linker_name"}), [])

        # 5. The pure helpers.
        expect("linkage_stem strips the mangling", ur.linkage_stem(c_right), c_wrong)
        expect("linkage_stem is identity for C names", ur.linkage_stem("get_move_work_adrs"), "get_move_work_adrs")
        expect("provides_global: global", ur.provides_global((".text", 0x12)), True)
        expect("provides_global: local", ur.provides_global((".text", 0x02)), False)
        expect("provides_global: absent", ur.provides_global(None), False)

        sym = write(tmp, "symbols.txt", b"foo = .text:0x80004000; // type:function size:0x10\n"
                                         b"// comment\n\nbar = .text:0x80004010;\n")
        saved_symbols = ur.SYMBOLS_REL
        ur.SYMBOLS_REL = os.path.relpath(sym, tmp)
        try:
            expect("map_rows reads the map's rows", ur.map_rows(tmp), {"foo", "bar"})
        finally:
            ur.SYMBOLS_REL = saved_symbols
        lcf = write(tmp, "ldscript.lcf", b"SECTIONS\n{\n    _stack_addr = 0x80004000;\n}\n")
        saved_lcf = ur.LDSCRIPT_REL
        ur.LDSCRIPT_REL = os.path.relpath(lcf, tmp)
        try:
            expect("linker_symbols reads assignments and the EABI/entry set",
                   ur.linker_symbols(tmp) >= {"_stack_addr", "_SDA_BASE_", "_SDA2_BASE_", "__start"}, True)
        finally:
            ur.LDSCRIPT_REL = saved_lcf

        # 6. The batch path, end to end, in a throwaway tree: the target object is a link input, another
        #    input provides a name, and `check_units` returns the refusal for the wrong-tag unit and
        #    nothing for the clean one. The cached index lands in build/tmp/undefrefs/link-symbols.json.
        tree = os.path.join(tmp, "tree")
        write(tree, os.path.join("build", "RMHE08", "obj", "U.o"), open(tgt, "rb").read())
        write(tree, os.path.join("build", "RMHE08", "src", "U.o"), open(ours, "rb").read())
        write(tree, os.path.join("build", "RMHE08", "obj", "provided.o"), build_obj(
            [(".text", b"\0" * 4)],
            [("provided_elsewhere", 4, ".text", FUNC, 0)]))
        write(tree, os.path.join("build", "RMHE08", "src", "V.o"), build_obj(
            [(".text", b"\0" * 8)],
            [("body", 8, ".text", FUNC, 0), ("mapped_name", 0, None, UNDEF)],
            relocs=[(".text", 4, "mapped_name")]))
        write(tree, os.path.join("config", "RMHE08", "symbols.txt"),
              ("%s = .text:0x80445DBC;\nmapped_name = .text:0x80000000;\n" % right).encode())
        write(tree, "build.ninja",
              (b"rule link\n  command = x\n\n"
               b"build build\\RMHE08\\main.elf: link build\\RMHE08\\obj\\U.o $\n"
               b"    build\\RMHE08\\obj\\provided.o | $\n"
               b"    build\\RMHE08\\ldscript.lcf\n"))
        expect("check_units names the wrong-tag unit",
               len(ur.check_units(tree, ["U"])["problems"]), 1)
        expect("check_units names the clean unit's absence",
               ur.check_units(tree, ["V"])["problems"], [])
        # add-only end to end: with the wrong spelling in the base snapshot, the same tree is not refused,
        # and the debt is reported.
        pre = ur.check_units(tree, ["U"], base_snapshot={"U": {"refs": [wrong]}})
        expect("check_units: a pre-existing-only unit passes", pre["problems"], [])
        expect("check_units: ... and is reported as debt",
               (len(pre["pre_existing"]), wrong in pre["pre_existing"][0]), (1, True))
        # a base snapshot taken from the same tree makes the wrong tag pre-existing and the row silent
        snap = ur.snapshot_base(tree, ["U"])
        expect("snapshot_base records the tree's own unresolved set",
               snap.get("U", {}).get("refs"), [wrong])
        expect("snapshot_base keys the entry by the source sha", "source" in snap.get("U", {}), True)
        expect("a snapshot of this tree makes the row silent",
               ur.check_units(tree, ["U"], base_snapshot=snap)["problems"], [])
        cache = os.path.join(tree, "build", "tmp", "undefrefs", "link-symbols.json")
        expect("the link-symbol cache was written", os.path.exists(cache), True)
        index = ur.link_symbol_index(tree)
        expect("the cache carries the other input's provider",
               index["providers"].get("provided_elsewhere", []),
               [os.path.normpath(os.path.join("build", "RMHE08", "obj", "provided.o"))])

        # 6b. `--base <rev>`: the snapshot is reconstructed from the base tree's own objects (a temporary
        #     worktree of the revision, the units compiled there), so a refusal can be judged pre-existing
        #     from one command instead of reverting the working tree. The hooks stand in for git and the
        #     compiler; the comparison that runs is the real `unresolved_names` path.
        base_tree = os.path.join(tmp, "basetree")
        write(base_tree, os.path.join("src", "U.cpp"), b"// the base source\n")
        write(base_tree, os.path.join("config", "RMHE08", "symbols.txt"),
              b"mapped_name = .text:0x80000000;\n")
        seen = {}

        def fake_compile(unit, main, wt):
            obj = write(wt, os.path.join("build", "RMHE08", "src", unit + ".o"), open(ours, "rb").read())
            return {"object": obj, "compiled": True}

        def fake_add(main, rev, path):
            seen["rev"] = rev
            shutil.copytree(base_tree, path)

        def fake_remove(main, path):
            seen["removed"] = path

        snap_at = ur.snapshot_base_at("MAIN", "BASE", ["U"], add_worktree=fake_add,
                                      remove_worktree=fake_remove, compiler=fake_compile)
        expect("--base: the revision asked for is the one checked out", seen.get("rev"), "BASE")
        expect("--base: the temporary worktree is removed after", seen.get("removed") is not None, True)
        expect("--base: the base object's unresolved set is captured", snap_at.get("U", {}).get("refs"), [wrong])
        fresh_at = ur.snapshot_base_at("MAIN", "BASE", ["new/unit"], add_worktree=fake_add,
                                       remove_worktree=fake_remove, compiler=fake_compile)
        expect("--base: a unit the base never had is recorded with no refs", fresh_at.get("new/unit"),
               {"source": None, "refs": []})
        pre_at = ur.check_units(tree, ["U"], base_snapshot=snap_at)
        expect("--base: the pre-existing refusal is reported, never refused", pre_at["problems"], [])
        expect("--base: and the debt is named with its count",
               "1 pre-existing" in (pre_at["pre_existing"][0] if pre_at["pre_existing"] else ""), True)
        unread = ur.check_units(tree, ["U"], base_snapshot={"U": {"refs": None}})
        expect("--base: an unreadable base is unjudged, never a false refusal",
               (unread["missing"], unread["problems"]), (["U"], []))

        # 7. A batch whose units carry only map rows has no candidate, so `check_units` never builds the
        #    index: the patched index raises, and no cache file appears.
        mapped_only = os.path.join(tmp, "mapped_only")
        write(mapped_only, os.path.join("build", "RMHE08", "src", "X.o"), build_obj(
            [(".text", b"\0" * 8)],
            [("body", 8, ".text", FUNC, 0), ("mapped_name", 0, None, UNDEF)],
            relocs=[(".text", 4, "mapped_name")]))
        write(mapped_only, os.path.join("config", "RMHE08", "symbols.txt"), b"mapped_name = .text:0x0;\n")
        saved_index = ur.link_symbol_index
        ur.link_symbol_index = lambda *a, **k: (_ for _ in ()).throw(AssertionError("index built"))
        try:
            expect("no candidate -> no index, no problem", ur.check_units(mapped_only, ["X"])["problems"], [])
        finally:
            ur.link_symbol_index = saved_index
        expect("no cache written for a candidate-free batch",
               os.path.exists(os.path.join(mapped_only, "build", "tmp", "undefrefs", "link-symbols.json")),
               False)

        # 8. `link_inputs` reads the link edge and stops at the implicit deps (`|`).
        saved_ninja = ur.NINJA_REL
        ur.NINJA_REL = os.path.join(tree, "build.ninja")
        try:
            got = ur.link_inputs(tree)
        finally:
            ur.NINJA_REL = saved_ninja
        expect("link_inputs reads the objects", got, [os.path.normpath(os.path.join("build", "RMHE08", "obj", "U.o")),
                                                      os.path.normpath(os.path.join("build", "RMHE08", "obj", "provided.o"))])
        expect("link_inputs stops at the implicit deps", [x for x in got if "ldscript" in x], [])

    if FAILURES:
        print("\n%d check(s) FAILED" % len(FAILURES))
        return 1
    print("\nall checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
