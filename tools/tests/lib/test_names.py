"""lib.names: the generated-name schemes, the mangling predicates, the stem and the member-mangling estimate."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
from tools.lib import names, testing

TIER = "fixture"

#: name -> (default, rule7, map, ledger)
TABLE = {
    "fn_8018B3B8": (True, True, True, True),
    "lbl_80594EE0": (True, True, True, True),
    "loc_805113B0": (True, True, True, True),
    "jumptable_805CEF78": (True, False, False, True),
    "@etb_800066E0": (True, False, True, False),
    "@123": (True, False, False, False),
    "dtor_8005E5E8": (False, False, True, False),
    "FUN_80001234": (False, False, True, False),
    "unk50": (False, False, True, True),
    "fn_": (False, False, False, True),
    "lbl_not_a_label": (False, False, False, False),
    "resetNetworkState3": (False, False, False, False),
    "kbd_init__FUc": (False, False, False, False),
    "main": (False, False, False, False),
}


def test_generated_schemes(c):
    for name, want in TABLE.items():
        got = tuple(names.is_generated(name, s) for s in ("default", "rule7", "map", "ledger"))
        c.check("is_generated(%s) default/rule7/map/ledger" % name, got, want)
    c.check("an empty name is never generated", names.is_generated(""), False)
    c.raises("an unknown scheme is refused", KeyError, names.is_generated, "fn_80000000", "nope")
    c.check("address_of reads the _XXXXXXXX tail", (names.address_of("fn_8018B3B8"), names.address_of("main")),
            (0x8018B3B8, None))


def test_generated_name_kind(c):
    """Rule 7 exact (owner, 2026-10-05): a stem anywhere, or a DOL address in the name; the boundaries are the
    false-positive cases measured on the tree."""
    table = {
        "fn_80041234": "generated", "fn_80041234__FPv": "generated", "view_fn_80041234": "generated",
        "fn_800FD864_fx": "generated", "dtor_8005E5E8": "generated", "zz_80123456_": "generated",
        "lbl_8059DCF8": "generated", "loc_805113B0": "generated", "fn_8004cad8": "generated",
        "Panel805482CC": "address", "s_80276B58": "address", "Helper_80147CE0": "address",
        "MHTRI_ENEMY_FN_80128204_H": "address", "tbl_0x80276B58": "address",
        "quest_flag_80000000_ck": None,      # 0x80000000 is below the DOL image: a flag value
        "Eft8030EffectSlot": None,           # `8030Effe` is followed by another hex digit
        "abc180123456": None,                # nine digits: a number, not an address
        "addr_80900000": None,               # past the DOL span
        "fn_1234": None, "main": None, "": None,
    }
    for name, want in table.items():
        c.check("generated_name_kind(%r)" % name, names.generated_name_kind(name), want)
    c.check("the DOL span covers the live map's lowest and highest symbol", names.DOL_SPAN[0] <= 0x80004000 and
            0x8079D7F0 < names.DOL_SPAN[1], True)


def test_mangling(c):
    c.check("is_mangled", [names.is_mangled(n) for n in ("fn__Fv", "Panic__Q24nw4r2dbFPCciPCce", "__start",
                                                         "_savegpr_14", "lbl_8058B290")],
            [True, True, False, False, False])
    stems = {
        "get_move_work_adrs__FUc": "get_move_work_adrs",
        "dl_acdata_to_ar_eqdata__FP11ArenaEqDataUc": "dl_acdata_to_ar_eqdata",
        "drawSpr2TF__FUcP9fltSpr2TFUc": "drawSpr2TF",
        "Panic__Q24nw4r2dbFPCciPCce": "Panic",
        "fn_80059550": "fn_80059550",
        "__start": "__start",
        "_savegpr_14": "_savegpr_14",
        "__FileWrite": "__FileWrite",
        "__ct__13NetPlStateMsgFv": "__ct__13NetPlStateMsgFv",
        "__ct__Q44nw4r3g3d6ScnMdl15CopiedMatAccessFv": "__ct",
        "send__16NetworkSingleTcpFPCUcl": "send__16NetworkSingleTcpFPCUcl",   # a known gap (spec): a class member
    }
    for name, want in stems.items():
        c.check("linkage_stem(%s)" % name, names.linkage_stem(name), want)
    c.check("peel_tokens finds the length-prefixed components",
            names.peel_tokens("Pl_get_gunner_pos__FP4_PLWQ34nw4r4math4VEC3l"), {"_PLW", "nw4r", "math", "VEC3"})


def test_member_mangling_estimate(c):
    c.check("param codes", [names.param_code(p) for p in ("const u8* data", "s32 size", "unsigned char c", "Foo& f",
                                                          "const char* const* argv", "int (*fn)(void)", "u8 buf[4]")],
            ["PCUc", "l", "Uc", "R3Foo", "PCPCc", None, None])
    c.check("a member", names.estimate_member_mangling("NetworkSingleTcp", "send", ["const u8* data", "s32 size"]),
            "send__16NetworkSingleTcpFPCUcl")
    c.check("a const member with no parameters", names.estimate_member_mangling("T", "get", ["void"], const_self=True),
            "get__1TCFv")
    c.check("a repeated class parameter is refused", names.estimate_member_mangling("T", "f", ["Foo* a", "Foo* b"]), None)
    c.check("a static member", names.estimate_static_mangling("GameSpyInterfaceThread", "getInstance", []),
            "getInstance__22GameSpyInterfaceThreadFv")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
