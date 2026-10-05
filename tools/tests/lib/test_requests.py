"""lib.requests: the schema, the legacy loader, static classification, owner resolution, the status sidecar, STOPGAP."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import os
import tempfile

from tools.lib import requests as R
from tools.lib import testing
from tools.lib.project import Ownership

TIER = "fixture"

#: Verbatim lines of the 2026-10-03 pilot request files (the loader's real input).
LEGACY = [
    {"kind": "rename", "symbol": "fn_8051E864", "owner": "SO/soi.cpp", "proposed": "SOInit",
     "evidence": "sNetworkLibraryWii::init calls it"},
    {"kind": "rename", "symbol": "fn_805075B0", "owner": "DWCi/dwc_error.cpp", "proposed": "DWC_CleanupInet (GUESS)",
     "evidence": "called right before SOCleanup"},
    {"kind": "rename", "symbol": "networkStreamWriter_dtor (.text:0x803CB958)", "owner": "lobby/lb_server_sel_trans.cpp",
     "proposed": "__dt__19NetworkStreamWriterFv, with ~NetworkStreamWriter() declared on the class", "evidence": "x"},
    {"kind": "decl", "symbol": "fn_804C2200, fn_804C2380", "owner": "OS/FindContainHeap_.c",
     "proposed": "declare MEMAllocFromExpHeapEx(heap,size,align) / MEMGetAllocatableSizeForExpHeapEx(heap,mode) in "
                 "OS/FindContainHeap_.h", "evidence": "soAlloc"},
    {"kind": "decl", "symbol": "SOGetHostID", "owner": "SO/soi.cpp (include/SO/soi.h)", "proposed": "u32 SOGetHostID(void);",
     "evidence": "start state 30"},
    {"kind": "decl", "symbol": "getErrorInfo654c (PatInterface.cpp)", "owner": "L4 include/Network/PatInterface.h",
     "proposed": "returns s32 (non-zero = an error is pending), not void", "evidence": "postEvent"},
    {"kind": "decl", "symbol": "fn_8007B870 / fn_80065804", "owner": "g3d/fn_80075DCC.cpp",
     "proposed": "they are placement operator new(size, void*) / delete(void*, void*): declare them as such",
     "evidence": "initWorkRecord"},
    {"kind": "move", "symbol": "MH3GetErrorString2", "owner": "Network/network_pat_control.cpp (L3)",
     "proposed": "drop the declaration from include/unsplit/Network.h (network_pat_control.h now declares it)",
     "evidence": "rule 2"},
    {"kind": "move", "symbol": "0x80794CC4 mpMediator__15sNetworkLibrary", "owner": "Network/network_opening.cpp",
     "proposed": "NO MOVE: keep it in network_opening's .sbss claim", "evidence": "the mangling"},
    {"kind": "rename", "symbol": "unit stem Network/constructNetworkLibrary.cpp", "owner": "L4",
     "proposed": "Network/sNetworkLibraryWii.cpp via land.py --unit-rename", "evidence": "x"},
    {"kind": "config", "symbol": "config/RMHE08/config.yml block_relocations", "owner": "orchestrator",
     "proposed": "block_relocations:\n- target: extabindex:0x80020000\n  end: extabindex:0x80020010", "evidence": "x"},
    {"kind": "rename", "symbol": "dtor_803CA338", "owner": "the band", "proposed": "the member-mutex class destructor",
     "evidence": "x"},
    {"kind": "decl", "symbol": "work_mem_alloc (work_mem_alloc__FUl)", "owner": "ef/system_core.cpp",
     "proposed": "declare `void* work_mem_alloc(unsigned long size);` in include/ef/system_core.h", "evidence": "x"},
]


def legacy(i):
    return R.normalise_legacy(LEGACY[i], "net-lx-0000", i + 1)


def test_validate(c):
    good = {"id": "net-l3-ef87#12", "kind": "decl", "symbol": "fn_803DF144", "address": "0x803DF144",
            "proposed_name": "setCircleMode", "confidence": "evidence", "evidence": "the call sites",
            "prototype": "void setCircleMode(NetworkSessionManagerPat* manager, s32 flag);",
            "stopgap": {"file": "src/Network/net_session_close.cpp", "id": "net-l3-ef87#12"}}
    c.check("a complete request is valid", R.validate(good), [])
    c.check("a bad id", R.validate(dict(good, id="L3-12")), ["id 'L3-12' is not `<slug>#<n>`"])
    c.check("an unknown kind", R.validate(dict(good, kind="move"))[0].startswith("kind 'move' is not one of"), True)
    c.check("a decl on a generated name needs proposed_name",
            R.validate({k: v for k, v in good.items() if k not in ("proposed_name", "prototype")}),
            ["decl of the generated name fn_803DF144 needs `proposed_name` (rule 7: the integrator names it)"])
    c.check("a decl on a real name does not",
            R.validate({"id": "a#1", "kind": "decl", "symbol": "OSGetTick", "evidence": "x"}), [])
    c.check("a rename needs proposed_name", "rename: needs `proposed_name`" in
            R.validate({"id": "a#1", "kind": "rename", "symbol": "fn_1", "evidence": "x"}), True)
    c.check("a two-line prototype is refused", "prototype must be one C line ending in `;`" in
            R.validate(dict(good, prototype="void f(void);\nint g;")), True)
    c.check("a prototype of another name is refused", "prototype does not declare setCircleMode" in
            R.validate(dict(good, prototype="void other(void);")), True)
    c.check("confidence is one of three", "confidence 'maybe' is not one of certain/evidence/guess" in
            R.validate(dict(good, confidence="maybe")), True)
    c.check("a stopgap needs file and id", "stopgap must be {file, id}" in R.validate(dict(good, stopgap={"file": "x"})),
            True)


def test_legacy_loader(c):
    r = legacy(0)
    c.check("a rename keeps its kind, address and name", (r.kind, r.targets[0].address, r.targets[0].proposed_name),
            ("rename", 0x8051E864, "SOInit"))
    c.check("the id is <slug>#<line>", r.id, "net-lx-0000#1")
    c.check("GUESS sets the confidence", legacy(1).confidence, "guess")
    c.check("... and the name is still read", legacy(1).targets[0].proposed_name, "DWC_CleanupInet")
    t = legacy(2).targets[0]
    c.check("a `(.text:0x...)` spelling gives address and section", (t.symbol, t.address, t.section),
            ("networkStreamWriter_dtor", 0x803CB958, ".text"))
    two = legacy(3)
    c.check("`a, b` is two targets with their addresses", [(x.symbol, x.address) for x in two.targets],
            [("fn_804C2200", 0x804C2200), ("fn_804C2380", 0x804C2380)])
    c.check("the header the request names", two.header, "include/OS/FindContainHeap_.h")
    c.check("a whole-line prototype is read", legacy(4).targets[0].prototype, "u32 SOGetHostID(void);")
    c.check("a backtick prototype is read", legacy(12).targets[0].prototype, "void* work_mem_alloc(unsigned long size);")
    c.check("... and the mangled alias is kept", legacy(12).targets[0].alias, "work_mem_alloc__FUl")
    c.check("prose with a parenthesis is never a prototype", legacy(5).targets[0].prototype, None)
    c.check("`drop the declaration` is a decl-move", legacy(7).kind, "decl-move")
    c.check("`NO MOVE` is info", legacy(8).kind, "info")
    c.check("a leading address is read", legacy(8).targets[0].symbol.startswith("0x80794CC4"), True)
    c.check("`unit stem` is a unit-rename", legacy(9).kind, "unit-rename")
    c.check("prose is not a name", legacy(11).targets[0].proposed_name, None)
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "net-lx-0000-requests.json")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(json.dumps(x) for x in LEGACY) + "\nnot json\n")
            fh.write(json.dumps({"id": "net-lx-0000#15", "kind": "rename", "symbol": "fn_1", "proposed_name": "a",
                                 "evidence": "x"}) + "\n")
        reqs = R.load_file(path)
        c.check("every line is a request (a bad line becomes an info request)", len(reqs), len(LEGACY) + 2)
        c.check("a new-schema line keeps its own id", reqs[-1].id, "net-lx-0000#15")
        c.check("... and is not legacy", reqs[-1].legacy, False)
        c.check("the slug is the file minus -requests.json", R.slug_of(path), "net-lx-0000")


def test_classify(c):
    got = [R.classify(legacy(i))[0] for i in range(len(LEGACY))]
    c.check("the fixture's classes", got,
            ["mechanical", "mechanical", "judgement", "mechanical", "mechanical", "semi", "judgement", "mechanical",
             "judgement", "judgement", "mechanical", "judgement", "mechanical"])
    c.check("`unsigned` is not the semi word `signed`", R.classify(legacy(12))[0], "mechanical")
    c.check("a destructor with no decision is judgement", R.classify(legacy(11))[1].startswith("a constructor/destructor"),
            True)
    c.check("... a decision with a C name makes it mechanical",
            R.classify(legacy(11), {"dtor_803CA338": "destroyMemberMutex"})[0], "mechanical")
    c.check("a member mangling is judgement", R.classify(legacy(2))[1].startswith("the proposed name is a member"), True)


def ownership():
    symbols = "\n".join([
        "SOInit = .text:0x8051E864; // type:function size:0x40",
        "ck_option_cfg__FUc = .text:0x803BEA28; // type:function size:0x20",
        "setCircleMode__24NetworkSessionManagerPatFUc = .text:0x803DF144; // type:function size:0x10",
        "lobby_state_block = .bss:0x806BF530; // type:object size:0x2EB8",
        "OSGetTick = .text:0x804D4D70; // type:function size:0x8",
        "loose = .text:0x80600000; // type:function size:0x8",
    ]) + "\n"
    splits = ("SO/soi.cpp:\n\t.text       start:0x8051E000 end:0x8051F000\n"
              "menu/get_pop_dat_ptr.cpp:\n\t.text       start:0x803BE000 end:0x803BF000\n"
              "Network/NetworkSessionManagerPat.cpp:\n\t.text       start:0x803D7000 end:0x803E0000\n"
              "enemy/em020_prog.cpp:\n\t.bss        start:0x806BF000 end:0x806C3000\n"
              "NAND/nand.c:\n\t.text       start:0x804D4000 end:0x804D5000\n"
              "Network/a.cpp:\n\t.text       start:0x805F0000 end:0x805F8000\n"
              "Network/b.cpp:\n\t.text       start:0x80608000 end:0x80609000\n")
    return Ownership.from_texts(symbols, splits)


def test_resolve(c):
    own = ownership()
    r = R.resolve_target(R.parse_target("fn_8051E864"), own)
    c.check("a lane's fn_ the map has renamed resolves by address (stale)", (r.current, r.owner, r.header, r.stale),
            ("SOInit", "SO/soi.cpp", "include/SO/soi.h", True))
    r = R.resolve_target(R.parse_target("lbl_806BF530"), own)
    c.check("a data label resolves to the unit claiming its .bss", (r.current, r.owner, r.section),
            ("lobby_state_block", "enemy/em020_prog.cpp", ".bss"))
    r = R.resolve_target(R.parse_target("OSGetTick"), own)
    c.check("a name resolves to the unit whose range covers it", (r.owner, r.header), ("NAND/nand.c", "include/NAND/nand.h"))
    r = R.resolve_target(R.parse_target("ck_option_cfg"), own)
    c.check("a C++ function filed by its plain name finds its one mangled row",
            (r.current, r.cpp_linkage, r.stale), ("ck_option_cfg__FUc", True, False))
    c.check("... whose source name is the stem", R.source_name(r.current), "ck_option_cfg")
    r = R.resolve_target(R.parse_target("fn_803DF144"), own)
    c.check("a member is recognised", R.is_member(r.current), True)
    r = R.resolve_target(R.parse_target("loose"), own)
    c.check("an unowned symbol gets its band header", (r.owner, r.header), (None, "include/unsplit/Network.h"))
    r = R.resolve_target(R.parse_target("nowhere"), own)
    c.check("an absent symbol says so", r.notes, ["not in the map (by name)"])
    req = R.normalise_legacy({"kind": "decl", "symbol": "lbl_806BF530", "owner": "unclaimed (.bss)", "proposed": "x",
                              "evidence": "x"}, "s", 1)
    req.owner_unit = "lobby/lb_npc.cpp"
    c.check("a request's owner_unit is cross-checked (the map wins)",
            R.resolve(req, own)[0].notes, ["the request names owner lobby/lb_npc.cpp; the map says enemy/em020_prog.cpp "
                                          "(the map wins)"])
    c.check("a live lane on the owner defers", R.live_owner(R.resolve(req, own), {"enemy/em020_prog"}),
            "enemy/em020_prog.cpp")
    c.check("decisions are read by spelling and address",
            R.decision_for(R.Target("fn_8051E864", 0x8051E864), {"0x8051E864": "SOInit"}), "SOInit")


def test_status_and_stopgap(c):
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "lane-a-requests.json")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write('{"id": "lane-a#1", "kind": "info"}\n{"id": "lane-a#2", "kind": "info"}\n')
        c.check("every request is open with no sidecar", R.open_ids([tmp]), {"lane-a#1", "lane-a#2"})
        R.write_status(path, {"lane-a#1": {"status": "applied", "note": "x"}}, now="2026-10-04T00:00:00")
        c.check("the sidecar sits beside the file", os.path.exists(os.path.join(tmp, "lane-a-requests.status.json")), True)
        c.check("an applied request is no longer open", R.open_ids([tmp]), {"lane-a#2"})
        c.check("the request file itself is never edited", open(path, encoding="utf-8").read().count("\n"), 2)
        c.raises("a status outside the vocabulary is refused", ValueError, R.write_status, path,
                 {"lane-a#2": {"status": "done"}})
    text = "a\n/* STOPGAP-BEGIN(x#1) */\nvoid f(void);\n/* STOPGAP-END(x#1) */\nb\n/* STOPGAP-BEGIN(x#2) */\n"
    blocks = R.stopgap_blocks(text)
    c.check("a paired block is found", [b[0] for b in blocks], ["x#1"])
    c.check("deleting it leaves the neighbours", text[:blocks[0][1]] + text[blocks[0][2]:],
            "a\nb\n/* STOPGAP-BEGIN(x#2) */\n")
    c.check("an unpaired BEGIN is reported", [u[0] for u in R.unpaired_stopgaps(text)], ["x#2"])


def test_config_items(c):
    l2 = {"kind": "config", "symbol": "updateSession", "address": "0x803D7764", "id": "net2-l2-101a#42",
          "proposed": "block_relocations: 0x803D7764 and 0x803D7768 (lis/addi of the constant 0x80060034)",
          "evidence": "x"}
    req = R.from_entry(l2)
    c.check("the L2 round-2 #42 prose is mechanical now (it read as judgement: `symbol` was glued to `proposed`)",
            R.classify(req)[0], "mechanical")
    c.check("... and becomes one block covering both instructions, the aside's constant ignored",
            R.config_items(l2["proposed"]), ("block_relocations",
                                             ["- source: .text:0x803D7764\n  end: .text:0x803D776C"],
                                             "instruction addresses"))
    c.check("the range form", R.config_items("block_relocations source .text:0x803D7564..0x803D756C")[1],
            ["- source: .text:0x803D7564\n  end: .text:0x803D756C"])
    c.check("the YAML form is taken as written", R.config_items(LEGACY[10]["proposed"])[1],
            ["- target: extabindex:0x80020000\n  end: extabindex:0x80020010"])
    c.check("the add_relocations form",
            R.config_items("add_relocations source .text:0x80001000 type R_PPC_ADDR16_HA target lbl_80500000")[1],
            ["- source: .text:0x80001000\n  type: R_PPC_ADDR16_HA\n  target: lbl_80500000"])
    c.check("an add_relocations without its type stays judgement",
            R.classify(R.from_entry(dict(l2, proposed="add_relocations source 0x80001000")))[0], "judgement")
    c.check("a proposal with no relocation key is judgement",
            R.classify(R.from_entry(dict(l2, proposed="fill_gaps: false")))[0], "judgement")


def test_prototypes(c):
    """One request, several declarations: the SO callees took 11 requests when each needed its own STOPGAP block."""
    many = {"id": "net3-c#4", "kind": "decl", "evidence": "the SO callees",
            "prototypes": ["s32 SOInit(void);", "u32 SOGetHostID(void);",
                           {"address": "0x8051E864", "proposed_name": "SOStartup", "prototype": "s32 SOStartup(void);"}],
            "stopgap": {"file": "src/Lane/lane.cpp", "id": "net3-c#4"}}
    c.check("a prototypes request is valid", R.validate(many), [])
    r = R.from_entry(many)
    c.check("... and loads one target per declaration, the name read from a string item",
            [(t.symbol, t.address, t.proposed_name, t.prototype) for t in r.targets],
            [("SOInit", None, None, "s32 SOInit(void);"), ("SOGetHostID", None, None, "u32 SOGetHostID(void);"),
             ("0x8051E864", 0x8051E864, "SOStartup", "s32 SOStartup(void);")])
    c.check("... applies as one unit, and classifies as one decl", (r.atomic, R.classify(r)[0]), (True, "mechanical"))
    one = {"id": "a#1", "kind": "decl", "symbol": "SOInit", "prototype": "s32 SOInit(void);", "evidence": "x"}
    c.check("`prototype` stays the one-item form", ([t.prototype for t in R.from_entry(one).targets],
            R.from_entry(one).atomic, R.prototype_items(one)[0]["prototype"]),
            (["s32 SOInit(void);"], False, "s32 SOInit(void);"))
    bad = lambda **kw: R.validate(dict(many, **kw))  # noqa: E731
    c.check("both forms at once are refused", "prototype and prototypes are exclusive (`prototype` is the one-item "
            "form)" in bad(prototype="s32 SOInit(void);"), True)
    c.check("a top-level symbol beside prototypes is refused",
            "prototypes: symbol belong in the items (each declaration carries its own)" in bad(symbol="SOInit"), True)
    c.check("only a decl carries several", bad(kind="field", symbol="X"),
            ["prototypes: only a `decl` request carries several declarations"])
    c.check("an empty list is refused", "prototypes must be a non-empty list" in bad(prototypes=[]), True)
    c.check("a two-line item is refused", "prototypes[1]: one C line ending in `;` (a string, or an object with "
            "`prototype`)" in bad(prototypes=["s32 a(void);\ns32 b(void);"]), True)
    c.check("an item that declares another name is refused", "prototypes[1]: does not declare SOStop" in
            bad(prototypes=[{"symbol": "SOStop", "prototype": "s32 SOInit(void);"}]), True)
    c.check("a name declared twice is refused", "prototypes[2]: SOInit is declared twice" in
            bad(prototypes=["s32 SOInit(void);", "s32 SOInit(void);"]), True)
    c.check("a generated item name needs proposed_name", "prototypes[1]: the generated name fn_8051E864 needs "
            "`proposed_name` (rule 7)" in bad(prototypes=[{"symbol": "fn_8051E864",
                                                          "prototype": "s32 fn_8051E864(void);"}]), True)
    c.check("declared_name reads functions, data and arrays",
            [R.declared_name(p) for p in ("s32 SOInit(void);", "extern u8 table[4];", 'extern "C" void* f(u32 x);',
                                          "u32 x;", "nonsense")],
            ["SOInit", "table", "f", "x", None])

if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
