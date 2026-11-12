"""mwcc-debugger funcspec: which function a mangled name, Class::method, an address or a plain name selects."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import tempfile
from pathlib import Path

from tools.lib import funcspec, testing

TIER = "fixture"

ROWS = [
    ("create__17HkbCellphoneInputFP12MEMAllocator", 0x8053E808, 0xB0),
    ("create__18HkbCellphoneKeypadFP12MEMAllocator", 0x8053F7BC, 0x488),
    ("create__9HbmKbListFP12MEMAllocator", 0x80554834, 0x2A4),
    ("BindPltt__Q34nw4r3g3d14ResTexPlttInfoFQ34nw4r3g3d7ResPlttQ34nw4r3g3d10ResTlutObj", 0x8009792C, 0xD8),
    ("__ct__9HbmKbListFv", 0x80554900, 0x20),
    ("plain_c_function", 0x80001000, 0x40),
]


def test_parse(c):
    s = funcspec.parse("create__9HbmKbListFP12MEMAllocator", ROWS)
    c.check("a mangled name is the source name, the whole scope and the link name",
            (s.name, s.scope, s.exact, s.mangled), ("create", ("HbmKbList",), True, "create__9HbmKbListFP12MEMAllocator"))
    q = funcspec.parse("nw4r::g3d::ResTexPlttInfo::BindPltt", ROWS)
    c.check("Class::method resolves to the one map row", q.mangled, ROWS[3][0])
    c.check("... with the row's size", q.size, 0xD8)
    c.check("a qualifier tail is enough", funcspec.parse("ResTexPlttInfo::BindPltt", ROWS).mangled, ROWS[3][0])
    a = funcspec.parse("0x8009792C", ROWS)
    c.check("an address names its row", (a.mangled, a.address, a.size), (ROWS[3][0], 0x8009792C, 0xD8))
    c.check("an 8-digit address needs no 0x", funcspec.parse("8053F7BC", ROWS).name, "create")
    c.check("... and its scope", funcspec.parse("8053F7BC", ROWS).scope, ("HkbCellphoneKeypad",))
    c.check("a plain name with a size picks the row of that size", funcspec.parse("create", ROWS, 0x488).scope,
            ("HkbCellphoneKeypad",))
    p = funcspec.parse("create", ROWS)
    c.check("a plain name stays the first function of that name", (p.mangled, p.scope, p.exact), ("", (), False))
    c.check("... and says that three symbols share it", len(p.notes), 1)
    c.contains("... naming them", p.notes[0], "3 symbols share the name create")
    c.check("a plain name no row shares has no note", funcspec.parse("BindPltt", ROWS).notes, ())
    c.check("a destructor spelling is the compiler's __dt", funcspec.parse("HbmKbList::~HbmKbList").name, "__dt")
    c.check("a constructor spelling is __ct and resolves", funcspec.parse("HbmKbList::HbmKbList", ROWS).mangled,
            "__ct__9HbmKbListFv")
    c.check("a C function is not a member", funcspec.parse("plain_c_function", ROWS).scope, ())
    c.check("without a map a mangled name still parses", funcspec.parse("fn__Fv").scope, ())


def test_refusals(c):
    c.raises("an address with no row", funcspec.SelectorError, funcspec.parse, "0x80001234", ROWS)
    c.raises("an address with no map at all", funcspec.SelectorError, funcspec.parse, "0x80001234")
    c.raises("a size no function of that name has", funcspec.SelectorError, funcspec.parse, "create", ROWS, 0x999)
    c.raises("an empty selector", funcspec.SelectorError, funcspec.parse, "  ")
    two = ROWS + [("create__9HbmKbListFP12MEMAllocatorUl", 0x80555000, 0x10)]
    err = c.raises("two overloads of Class::method", funcspec.SelectorError, funcspec.parse, "HbmKbList::create", two)
    c.contains("... listing both", str(err), "create__9HbmKbListFP12MEMAllocatorUl")
    c.check("--size separates them", funcspec.parse("HbmKbList::create", two, 0x10).mangled,
            "create__9HbmKbListFP12MEMAllocatorUl")


def test_matches(c):
    keypad = funcspec.parse("create__18HkbCellphoneKeypadFP12MEMAllocator", ROWS)
    c.check("the class chain must be the mangled name's", keypad.matches("create", None, ("HkbCellphoneKeypad",)), True)
    c.check("another class's function of the same name is not it", keypad.matches("create", None, ("HbmKbList",)), False)
    c.check("a global function of the same name is not it", keypad.matches("create", None, ()), False)
    nested = funcspec.parse(ROWS[3][0])
    c.check("a nested chain must be whole", nested.matches("BindPltt", None, ("g3d", "ResTexPlttInfo")), False)
    c.check("... and equal", nested.matches("BindPltt", None, ("nw4r", "g3d", "ResTexPlttInfo")), True)
    c.check("a cached link name decides: equal", keypad.matches("create", keypad.mangled, ("X",)), True)
    c.check("a cached link name decides: another overload",
            keypad.matches("create", "create__18HkbCellphoneKeypadFv", ("HkbCellphoneKeypad",)), False)
    tail = funcspec.parse("ResTexPlttInfo::BindPltt")
    c.check("an unresolved Class::method matches the chain's tail",
            tail.matches("BindPltt", None, ("nw4r", "g3d", "ResTexPlttInfo")), True)
    c.check("... and not a chain it does not end in", tail.matches("BindPltt", None, ("ResTexPlttInfo", "x")), False)
    plain = funcspec.parse("create")
    c.check("a plain name matches any scope", plain.matches("create", None, ("A",)), True)
    c.check("... but not another name", plain.matches("create2", None, ()), False)
    c.check("a build that hides the scope matches by name", keypad.matches("create", None, None), True)
    c.check("the selector survives the JSON hop to gdb", funcspec.Selector.from_dict(keypad.to_dict()), keypad)


def test_scope_chain(c):
    words = {0x100: 0x200, 0x104: 0x300, 0x200: 0x400, 0x204: 0x310, 0x400: 0, 0x404: 0}
    strings = {0x300 + 0xA: "Klass", 0x310 + 0xA: "ns"}
    chain = funcspec.scope_chain(lambda a: words[a], lambda a: strings[a], 0x100)
    c.check("the parent chain is read outermost first, the unnamed global scope skipped", chain, ("ns", "Klass"))
    c.check("no scope is no chain", funcspec.scope_chain(lambda a: 1 // 0, lambda a: "", 0), ())
    cyc = {0x10: 0x10, 0x14: 0x20}
    c.check("a cycle ends at the limit", len(funcspec.scope_chain(lambda a: cyc[a], lambda a: "x", 0x10, limit=5)), 5)


def test_function_rows(c):
    with tempfile.TemporaryDirectory() as td:
        path = Path(td) / "symbols.txt"
        path.write_text("create__3FooFv = .text:0x80001000; // type:function size:0x10 scope:global\n"
                        "lbl_80002000 = .data:0x80002000; // size:0x4\n", encoding="utf-8")
        c.check("only .text functions are rows", funcspec.function_rows(str(path)), [("create__3FooFv", 0x80001000, 0x10)])
        c.check("a missing map is no rows", funcspec.function_rows(str(Path(td) / "none.txt")), [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
