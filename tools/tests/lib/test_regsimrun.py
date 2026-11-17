"""lib.regsimrun and lib.localedit: the debugger command line, gdb discovery, the stand-in comparison, the plan's edit text,
the field walker, the hoisted call result and their refusals."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import importlib
from pathlib import Path

from tools.lib import localedit, regsim, regsimrun, testing
from tools.lib.binary.build import ElfBuilder

TIER = "fixture"
debugger = importlib.import_module("tools.mwcc-debugger.mwcc_debugger")

WORDS = bytes.fromhex("38600000" "4e800020")


def obj(words: bytes, call: str = "callee"):
    b = ElfBuilder().section(".text", words).symbol("fn", ".text", 0, len(words), bind="global", type="func")
    b.symbol(call)
    return b.reloc(".text", 0, call, "R_PPC_REL24").build()


def test_quoting(c):
    c.check("a plain token stays bare", regsimrun.quote_arg("-O4,p"), "-O4,p")
    c.check("a pragma with a space is quoted", regsimrun.quote_arg("cats off"), '"cats off"')
    line = regsimrun.command_line(["mwcceppc.exe", "-pragma", "cats off", "-i", "C:\\a b\\src", "-c", "x.cpp"])
    c.check("the debugger reads the line back as the same tokens", debugger.split_command_line(line),
            ["mwcceppc.exe", "-pragma", "cats off", "-i", "C:\\a b\\src", "-c", "x.cpp"])
    c.check("a token with a double quote takes single quotes", regsimrun.quote_arg('a"b c'), "'a\"b c'")
    c.raises("both quote kinds cannot be spelled", ValueError, regsimrun.quote_arg, "a\"b'c")


def test_split_and_stand_in(c):
    cmd = ["cmd", "/c", "C:\\b\\sjiswrap.exe", "C:\\b\\compilers\\GC\\3.0a5.2\\mwcceppc.exe", "-O4,p", "-pragma",
           "cats off", "-MMD", "-c", "C:\\w\\src\\u.cpp", "-o", "C:\\w\\build", "&&", "python", "objalign.py"]
    comp = regsimrun.split_compile(cmd)
    c.check("the version is the compiler's directory", comp.version, "GC/3.0a5.2")
    c.check("the flags lose -MMD and the wrapper", comp.flags, ["-O4,p", "-pragma", "cats off"])
    c.check("the bare invocation ends in -c SRC -o DIR", comp.tokens("D")[-4:], ["-c", "C:\\w\\src\\u.cpp", "-o", "D"])
    c.check("the stand-in swaps the version directory", regsimrun.stand_in_compiler(comp.compiler),
            "C:\\b\\compilers\\Wii\\1.3\\mwcceppc.exe")
    c.raises("a command with no compiler is refused", SystemExit, regsimrun.split_compile, ["cmd", "/c", "-MMD"])


def test_gdb(c):
    with testing.temp_dir() as tmp:
        root = Path(tmp)
        calls = []

        def fetch(dest):
            calls.append(dest)
            g = Path(dest) / "mingw64" / "bin" / "gdb.exe"
            g.parent.mkdir(parents=True)
            g.write_bytes(b"")

        got, how = regsimrun.locate_gdb(root, which=lambda n: None, fetch=fetch)
        c.check("with none anywhere it is fetched into the slot", (how.startswith("fetched"), len(calls)), (True, 1))
        c.check("... at .pi/tmp/mwcc-dbg", Path(got), regsimrun.slot_gdb(root))
        got, how = regsimrun.locate_gdb(root, which=lambda n: None, fetch=fetch)
        c.check("the second time the slot's copy is used", (how, len(calls)), ("slot copy", 1))
        got, how = regsimrun.locate_gdb(root, which=lambda n: "X")
        c.check("the slot's copy beats PATH", how, "slot copy")
        c.raises("a missing --gdb is refused", SystemExit, regsimrun.locate_gdb, root, str(root / "nope.exe"))
    with testing.temp_dir() as tmp:
        got, how = regsimrun.locate_gdb(tmp, which=lambda n: "C:\\x\\gdb.exe")
        c.check("PATH is used when the slot has none", (got, how), ("C:\\x\\gdb.exe", "PATH"))


def test_compare(c):
    with testing.temp_dir() as tmp:
        a, b, d, e = (Path(tmp) / n for n in "abde")
        a.write_bytes(obj(WORDS))
        b.write_bytes(obj(WORDS))
        d.write_bytes(obj(bytes.fromhex("38600001" "4e800020")))
        e.write_bytes(obj(WORDS, "other"))
        same = regsimrun.compare_function(str(a), str(b), "fn")
        c.check("the same bytes and calls are identical", same.identical, True)
        diff = regsimrun.compare_function(str(a), str(d), "fn")
        c.check("a changed word is not identical, first at byte 3", (diff.identical, diff.first_diff), (False, 3))
        c.check("a different callee is a relocation difference only", (lambda r: (r.same_bytes, r.same_relocs))(
            regsimrun.compare_function(str(a), str(e), "fn")), (True, False))
        c.check("a missing symbol is named", bool(regsimrun.compare_function(str(a), str(b), "zz").notes), True)


GRAPH = regsim.Graph({
    35: regsim.Node(35, 28, "set", neighbors=(1,)), 36: regsim.Node(36, 29, "j", neighbors=(1,)),
    38: regsim.Node(38, 31, "@1999", neighbors=(1,)), 40: regsim.Node(40, 0, "", neighbors=()),
    41: regsim.Node(41, 0, "i", neighbors=())},
    defs={38: "addi r38,r35,2", 40: "add r43,r3,r42"})


def test_edits(c):
    e = localedit.parse_edit(("declare r38(@1999) as a local at declaration position 1",))
    c.check("a declare edit", (e.kind, e.vreg, e.label, e.position), ("declare", 38, "r38(@1999)", 1))
    f = localedit.parse_edit(("fold r43(-) into local r35(i)",))
    c.check("a fold edit", (f.kind, f.vreg, f.target, f.target_label, f.label), ("fold", 43, 35, "i", "r43(-)"))
    c.check("no edit", localedit.parse_edit(()), None)
    g = regsim.Graph({43: regsim.Node(43, 29, ""), 35: regsim.Node(35, 0, "i")}, defs={43: "add r43,r3,r42"})
    try:
        localedit.fold(g, f)
    except localedit.Refused as r:
        c.contains("a fold into a dead local says it is not a source change", r.reason, "not a source change")
        c.contains("... and gives the edit by hand", r.manual, "keep i live")
    else:
        c.check("fold refuses", False, True)
    c.check("a pointer takes the singular of its field", localedit.pointer_name("parts_0x02", set()), "part")
    c.check("a taken name gets a suffix", localedit.pointer_name("parts_0x02", {"part"}), "part_p")
    c.check("a result local drops _get", localedit.local_name_for("hunter_rank_points_get", set()), "hunter_rank_points")


HEADER = """struct Q_Set {
    /* +0x00 */ u16 weapon_0x00;
    /* +0x02 */ u16 parts_0x02[2];
    /* +0x06 */ u8 flag_0x06;
};
"""
SOURCE = HEADER + """extern "C" void swap(u16* p, u16 a);
extern "C" void f(Q_Set* user, u16 a)
{
    int i;
    int j;
    Q_Set* set;
    for (i = 0, set = user; i < 20; set++, i++) {
        for (j = 0; j < 2; j++) {
            swap(&set->parts_0x02[j], a);
        }
    }
}
"""


def test_walker(c):
    with testing.temp_dir() as tmp:
        src = Path(tmp) / "src" / "u.cpp"
        src.parent.mkdir()
        src.write_text(SOURCE, encoding="utf-8", newline="\n")
        flds = localedit.struct_fields([str(src)], "Q_Set")
        c.check("fields are read from the offset comments", [(f.offset, f.type, f.name, f.dims) for f in flds],
                [(0, "u16", "weapon_0x00", ""), (2, "u16", "parts_0x02", "[2]"), (6, "u8", "flag_0x06", "")])
        c.check("a pointer type names its class", localedit.struct_of("const Q_Set *"), "Q_Set")
        c.check("a value type names none", localedit.struct_of("int"), None)
        c.check("a local's type is read from its line", localedit.declared_type("set", SOURCE, "f"), "Q_Set*")
        c.check("a parameter's type too", localedit.declared_type("user", SOURCE, "f"), "Q_Set*")
        locs = regsim.source_locals(SOURCE, "f")
        g = regsim.Graph({33: regsim.Node(33, 31, "user"), 34: regsim.Node(34, 30, "a"), 35: regsim.Node(35, 29, "i"),
                          36: regsim.Node(36, 28, "j"), 37: regsim.Node(37, 27, "set"), 38: regsim.Node(38, 26, "@9")},
                         defs={38: "addi r38,r37,2"})
        mapping = regsim.map_locals(g, 2, locs)
        e = localedit.Edit("declare", 38, "r38(@9)", position=1)
        ch = localedit.walker(g, e, [], SOURCE, "f", str(src), str(Path(tmp) / "src"), mapping, [37, 38, 35, 36])
        c.check("the pointer is a u16*", (ch.new_local.name, ch.new_local.line), ("part", "u16* part;"))
        old, new = ch.blocks[0]
        c.contains("the loop header walks the pointer", new, "for (j = 0, part = set->parts_0x02; j < 2; part++, j++) {")
        c.contains("... and the site reads it", new, "swap(part, a);")
        c.check("the old block is the loop", old.splitlines()[0].strip(), "for (j = 0; j < 2; j++) {")
        bad = SOURCE.replace("&set->parts_0x02[j]", "&set->parts_0x02[0]")
        c.raises("with no site in a counted loop it refuses", localedit.Refused, localedit.walker, g, e, [], bad, "f",
                 str(src), str(Path(tmp) / "src"), mapping, [37, 38, 35, 36])
        g.defs[38] = "lwz r38,r37,2"
        c.raises("a temp that is not base + offset is refused", localedit.Refused, localedit.walker, g, e, [], SOURCE,
                 "f", str(src), str(Path(tmp) / "src"), mapping, [37, 38, 35, 36])


HOIST = """extern "C" u32 pts(u16 n);
extern "C" u32* blk(void);
extern "C" void g(u32* out)
{
    *out = 1;
    if (*blk() >= pts(999)) {
        return;
    }
    *out = pts(2);
}
"""


def test_hoist(c):
    with testing.temp_dir() as tmp:
        src = Path(tmp) / "src" / "h.cpp"
        src.parent.mkdir()
        src.write_text(HOIST, encoding="utf-8", newline="\n")
        words = bytes(4 * 6)
        b = ElfBuilder().section(".text", words).symbol("g", ".text", 0, len(words), bind="global", type="func")
        b.symbol("blk").symbol("pts")
        for off, name in ((0, "blk"), (4, "pts"), (12, "pts")):
            b.reloc(".text", off, name, "R_PPC_REL24")
        objfile = Path(tmp) / "h.o"
        objfile.write_bytes(b.build())
        c.check("the object's calls in code order", localedit.call_symbols(str(objfile), "g"), ["blk", "pts", "pts"])
        g = regsim.Graph({33: regsim.Node(33, 31, ""), 34: regsim.Node(34, 31, "@7")})
        instrs = [("bl", ()), ("mr", ("r34", "r3")), ("bl", ()), ("cmplw", ("cr0", "r34", "r3")), ("bl", ()),
                  ("stw", ("r3", "r33"))]
        ch = localedit.hoist(g, 34, instrs, str(objfile), "g", HOIST, "g", str(src), str(Path(tmp) / "src"))
        old, new = ch.blocks[0]
        c.contains("the call is evaluated before its statement", new, "u32 pts_value = pts(999);")
        c.contains("... and the statement reads the local", new, "if (*blk() >= pts_value) {")
        c.check("the old block is the previous line and the statement", old.splitlines(),
                ['    *out = 1;', '    if (*blk() >= pts(999)) {'])
        named = localedit.hoist(g, 34, instrs, str(objfile), "g", HOIST, "g", str(src), str(Path(tmp) / "src"), "cap")
        c.contains("--local-name picks the name", named.blocks[0][1], "u32 cap = pts(999);")
        two = [("bl", ()), ("mr", ("r34", "r3")), ("bl", ()), ("bl", ()), ("cmplw", ("cr0", "r34", "r3"))]
        c.raises("two calls in the live range are ambiguous", localedit.Refused, localedit.hoist, g, 34, two,
                 str(objfile), "g", HOIST, "g", str(src), str(Path(tmp) / "src"))
        short = ElfBuilder().section(".text", bytes(8)).symbol("g", ".text", 0, 8, bind="global", type="func")
        short.symbol("pts").reloc(".text", 0, "pts", "R_PPC_REL24")
        (Path(tmp) / "s.o").write_bytes(short.build())
        c.raises("object and dump disagreeing on the call count is refused", localedit.Refused, localedit.hoist, g, 34,
                 instrs, str(Path(tmp) / "s.o"), "g", HOIST, "g", str(src), str(Path(tmp) / "src"))
        seq = HOIST.replace("*blk() >= pts(999)", "*blk() && pts(999)")
        c.raises("a call after && is not reordered", localedit.Refused, localedit.hoist, g, 34, instrs, str(objfile),
                 "g", seq, "g", str(src), str(Path(tmp) / "src"))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
