"""lib.refs: the dump index (address-keyed, stale copies ranked), the object index, the cache, the census, the scans."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import os
import struct

from tools.lib import refs, testing
from tools.lib.binary.build import ElfBuilder

TIER = "fixture"

#: the unit's own file prints the current callee name; the stale top-level copy prints the old `fn_` label of the
#: same address and also dumps a site only it has; a data file holds a pointer table
UNIT = "\n".join([
    '# 0x80001000..0x80001120 | size: 0x120',
    '.text',
    '# .text:0x0 | 0x80001000 | size: 0x1C',
    '.fn caller, global',
    '/* 80001000 00000000  94 21 FF F0 */\tstwu r1, -0x10(r1)',
    '/* 80001004 00000004  38 60 00 00 */\tli r3, 0x0',
    '/* 80001008 00000008  48 00 02 79 */\tbl quest_init__FUc',
    '/* 8000100C 0000000C  90 7F 00 DC */\tstw r3, 0xdc(r31)',
    '/* 80001010 00000010  4E 80 00 20 */\tblr',
    '.endfn caller',
    '# .text:0x60 | 0x80001060 | size: 0x18',
    '.fn with_data, global',
    '/* 80001060 00000060  3C 60 80 50 */\tlis r3, lbl_80500000@ha',
    '/* 80001064 00000064  38 63 00 00 */\taddi r3, r3, lbl_80500000@l',
    '/* 80001068 00000068  80 A0 00 00 */\tlwz r5, lbl_80500020@sda21(r0)',
    '/* 8000106C 0000006C  90 C0 00 00 */\tstw r6, lbl_80500020@sda21(r0)',
    '/* 80001070 00000070  4E 80 00 20 */\tblr',
    '.endfn with_data',
    '']) + "\n"
STALE = "\n".join([
    '# 0x80001000..0x80001280 | size: 0x280',
    '.text',
    '# .text:0x0 | 0x80001000 | size: 0x1C',
    '.fn fn_80001000, global',
    '/* 80001000 00000000  94 21 FF F0 */\tstwu r1, -0x10(r1)',
    '/* 80001004 00000004  38 60 00 00 */\tli r3, 0x0',
    '/* 80001008 00000008  48 00 02 79 */\tbl fn_80001280',
    '/* 8000100C 0000000C  4E 80 00 20 */\tblr',
    '.endfn fn_80001000',
    '# .text:0x200 | 0x80001200 | size: 0x10',
    '.fn fn_80001200, global',
    '/* 80001200 00000200  38 60 00 07 */\tli r3, 0x7',
    '/* 80001204 00000204  48 00 00 7D */\tbl quest_init__FUc',
    '/* 80001208 00000208  4E 80 00 20 */\tblr',
    '.endfn fn_80001200',
    '# .text:0x280 | 0x80001280 | size: 0x40',
    '.fn fn_80001280, global',
    '/* 80001280 00000280  4E 80 00 20 */\tblr',
    '.endfn fn_80001280',
    '']) + "\n"
DATA = "\n".join([
    '# 0x80500000..0x80500020 | size: 0x20',
    '.data',
    '# .data:0x0 | 0x80500000 | size: 0x10',
    '.obj lbl_80500000, global',
    '\t.4byte quest_init__FUc',
    '\t.4byte 0x00000000',
    '.endobj lbl_80500000',
    '# .data:0x20 | 0x80500020 | size: 0x4',
    '.obj lbl_80500020, global',
    '\t.4byte 0x00000000',
    '.endobj lbl_80500020',
    '']) + "\n"


def dump_tree(tree):
    tree.add_asm("menu/multi_result.s", UNIT)
    tree.add_asm("auto_fn_80001000_text.s", STALE)
    tree.add_asm("auto_07_80500000_data.s", DATA)
    asm = str(tree.build_dir / "asm")
    return asm, refs.dump_files(asm)


def test_dump_index(c):
    with testing.FixtureTree() as tree:
        asm, files = dump_tree(tree)
        c.check("three dump files, sorted", [os.path.relpath(f, asm).replace("\\", "/") for f in files],
                ["auto_07_80500000_data.s", "auto_fn_80001000_text.s", "menu/multi_result.s"])
        index, stats = refs.build_dump_index(asm, files, "RMHE08")
        refs.attach_derived(index)
        rows, retried = refs.rows_at(index, 0x80001280)
        sites = {(hex(r[0]), r[1], r[3], r[4]) for r in rows}
        c.check("the callee renamed since the stale copy still resolves by address, the canonical text wins",
                sorted(sites), sorted({("0x80001008", "call", "bl quest_init__FUc", "0x0"),
                                       ("0x80001204", "call", "bl quest_init__FUc", "0x7")}))
        c.check("the stale copy ranks below the canonical file",
                refs.file_rank(os.path.join(asm, "auto_fn_80001000_text.s"), asm)[0] >
                refs.file_rank(os.path.join(asm, "menu", "multi_result.s"), asm)[0], True)
        c.check("the one site both copies dump is dropped once", stats["duplicates"], 1)
        c.check("no printed label disagrees with its displacement", stats["label_mismatch"], 0)
        data_rows, _ = refs.rows_at(index, 0x80500020)
        c.check("sda21 accesses are read and write", sorted((hex(r[0]), r[1]) for r in data_rows),
                [("0x80001068", "read"), ("0x8000106c", "write")])
        pairs = refs.coalesce(refs.rows_at(index, 0x80500000)[0])
        c.check("a lis/addi pair coalesces into one addr site",
                [(hex(r[0]), r[1]) for r in pairs if r[1] == "addr"], [("0x80001060", "addr")])
        c.check("a .4byte entry is a pointer at the containing object",
                [(hex(r[0]), r[1]) for r in refs.rows_at(index, 0x80001280)[0] if r[1] == "pointer"], [])
        c.check("the pointer table names its target by the label table",
                any(r[1] == "pointer" and r[0] == 0x80500000 for r in index["refs"]["quest_init__FUc"]), True)
        c.check("an unresolved name is retried through the names a query passes",
                (len(refs.rows_at(index, 0x80001280, ("quest_init__FUc",))[0]) > len(rows),
                 refs.rows_at(index, 0x80001280, ("quest_init__FUc",))[1]), (True, ["quest_init__FUc"]))


def test_cache(c):
    with testing.FixtureTree() as tree:
        asm, files = dump_tree(tree)
        cache = str(tree.root / "build" / "tmp" / "refs" / "graph.json")
        built = []

        def build():
            built.append(1)
            return refs.build_dump_index(asm, files, "RMHE08")

        sig = refs.libcache.stat_digest(files, asm)
        _i, info = refs.load_index(cache, sig, len(files), build)
        c.check("first load builds", (info["rebuilt"], info["cached"], len(built)), (True, False, 1))
        _i, info = refs.load_index(cache, sig, len(files), build)
        c.check("second load is a cache hit", (info["cached"], info["reason"], len(built)), (True, "cache hit", 1))
        _i, info = refs.load_index(cache, sig + "x", len(files), build, changed="changed")
        c.check("a different signature rebuilds with the caller's reason", (info["reason"], len(built)), ("changed", 2))
        _i, info = refs.load_index(cache, sig, len(files), build, rebuild=True)
        c.check("--rebuild", info["reason"], "forced rebuild")
        body = json.loads(open(cache, encoding="utf-8").read())
        body["schema"] = 99
        tree.write(cache, json.dumps(body))
        _i, info = refs.load_index(cache, sig, len(files), build)
        c.check("a foreign schema is rebuilt, not trusted", info["reason"].startswith("cache schema 99"), True)


def elf_with(code_syms, relocs, data=b"\0" * 8):
    b = ElfBuilder()
    b.section(".text", b"\x60\x00\x00\x00" * 8)
    b.section(".data", data)
    b.section("extab", bytes(8))
    for name, sec, value, size, kind in code_syms:
        b.symbol(name, sec, value, size, type=kind)
    for sec, off, sym, typ in relocs:
        b.reloc(sec, off, sym, typ)
    return b


def test_object_index_and_census(c):
    with testing.FixtureTree() as tree:
        obj = elf_with([("caller", ".text", 0, 0x20, "func"), ("table", ".data", 0, 8, "object"),
                        ("callee", None, 0, 0, "notype"), ("glob", None, 0, 0, "notype")],
                       [(".text", 4, "callee", "R_PPC_REL24"), (".text", 10, "glob", "R_PPC_ADDR16_HA"),
                        (".text", 14, "glob", "R_PPC_ADDR16_LO"), (".data", 0, "callee", "R_PPC_ADDR32"),
                        (".text", 18, "glob", "R_PPC_EMB_SDA21"), ("extab", 0, "glob", "R_PPC_ADDR32")])
        path = tree.add_object("A/a.o", obj)
        symbols = {"caller": [(".text", 0x80001000, "function")], "table": [(".data", 0x80500000, "object")],
                   "callee": [(".text", 0x80002000, "function")], "glob": [(".sbss", 0x80600000, "object")]}
        index, stats = refs.build_object_index(str(tree.build_dir / "obj"), [str(path)], symbols, "RMHE08")
        refs.attach_derived(index)
        c.check("source: elf", index["source"], "elf")
        calls = refs.rows_at(index, 0x80002000)[0]
        c.check("a REL24 is a call at its absolute site; a data word is a pointer",
                sorted((hex(r[0]), r[1], r[3]) for r in calls),
                [("0x80001004", "call", "bl callee"), ("0x80500000", "pointer", "R_PPC_ADDR32 callee")])
        c.check("non-call code relocations are addr with the relocation's name",
                sorted(r[3] for r in refs.rows_at(index, 0x80600000)[0]),
                ["R_PPC_ADDR16_HA glob", "R_PPC_ADDR16_LO glob", "R_PPC_EMB_SDA21 glob"])
        c.check("the object index counts its rows", stats["refs"], 5)

        c.check("object_refs: undefined names with their relocation counts (extab bookkeeping skipped)",
                refs.object_refs(str(path)),
                {"callee": 2, "glob": 3})
        c.check("an unreadable object has no references", refs.object_refs(str(tree.root / "nope.o")), {})
        ranges = {".sbss": [(0x805F0000, 0x80600000, "B/b"), (0x80600010, 0x80600020, "C/c")]}
        found, read = refs.census(str(tree.build_dir / "obj"), ["A/a", "Z/missing"],
                                  {"glob": {"section": ".sbss", "address": 0x80600000, "size": 4, "type": "object"},
                                   "callee": {"section": ".text", "address": 0x80002000, "size": 4, "type": "function"}},
                                  ranges)
        records, cstats = found
        c.check("census reads only the objects that exist", read, 1)
        c.check("an uncovered data address is an orphan bracketed by its neighbours",
                [(r["unit"], r["name"], r["status"], r["sites"], r["prev"][0], r["next"][0]) for r in records],
                [("A/a", "glob", "orphan", 3, "B/b", "C/c")])
        c.check("a code name is counted, not recorded", cstats, {"unmapped": 0, "code": 1, "other_section": 0})
        c.check("classify: own / other", (refs.classify_address(".sbss", 0x805F0004, ranges, "B/b")["status"],
                                          refs.classify_address(".sbss", 0x805F0004, ranges, "A/a")["owner"]),
                ("own", "B/b"))


def test_runs_and_text_scan(c):
    table = {0: {"A": 1}, 4: {"A": 2}, 8: {"B": 1}, 12: {}}
    got = refs.runs_over(lambda a: table[a], 0, 12, 4)
    c.check("equal reader sets collapse into runs; a set change is a seam",
            ([(r["start"], r["end"], r["readers"], r["sites"]) for r in got["runs"]], got["seams"]),
            ([(0, 4, ["A"], 3), (8, 8, ["B"], 1), (12, 12, [], 0)], [8, 12]))
    base = 0x80004000
    words = [0x3C608050, 0x38630010, 0x48000009, 0x4E800020, 0x3C808000, 0x38844000, 0x4E800020, 0x60000000,
             0x4E800020]
    blob = struct.pack(">%dI" % len(words), *words)
    fns = (base, base + 0x10, base + 0x20)
    found = refs.text_refs([(base, blob)], None, None, lambda t: 0x80500000 <= t < 0x80600000, fns,
                           lambda site, t: False)
    c.check("data references from the code", found.refs, {0x80500010: [base + 4]})
    c.check("a call and a function address taken, in scan order", found.fn_edges,
            [(base + 8, base + 0x10, "call"), (base + 0x14, base, "addr")])
    own = refs.text_refs([(base, blob)], None, None, lambda t: False, fns, lambda site, t: True)
    c.check("a function's own address is not an edge", [e for e in own.fn_edges if e[2] == "addr"], [])


def test_function_graph(c):
    with testing.FixtureTree() as tree:
        asm, _files = dump_tree(tree)
        fns = {"caller": {"addr": 0x80001000}, "with_data": {"addr": 0x80001060},
               "quest_init__FUc": {"addr": 0x80001280}}
        labels = {"lbl_80500000": {"section": ".data", "addr": 0x80500000},
                  "lbl_80500020": {"section": ".data", "addr": 0x80500020}}
        g = refs.function_graph([os.path.join(asm, "menu", "multi_result.s")], fns, labels, asm)
        c.check("per-function calls and data references", (g["funcs"]["caller"]["calls"], g["funcs"]["with_data"]["refs"]),
                (["quest_init__FUc"], ["lbl_80500000", "lbl_80500020"]))
        c.check("the .fn self-check matches both spans", (g["fn_check"]["matched"], g["fn_check"]["mismatched_count"]), (2, 0))
        c.check("resolve_name: modifier dropped, @-pool address checked",
                (refs.resolve_name("lbl_80500000@ha", labels), refs.resolve_name('@12_80500000', {"@12": 1},
                                                                                  {"@12": {"addr": 0x80500000}}),
                 refs.resolve_name('@12_80500004', {"@12": 1}, {"@12": {"addr": 0x80500000}})),
                ("lbl_80500000", "@12", None))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
