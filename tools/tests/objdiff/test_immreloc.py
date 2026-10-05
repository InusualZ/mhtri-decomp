"""immreloc: a target object whose `lis`/`addi` dtk relocated against `fn_X+N` / `@eti_X+N`, our object building the same
value as a plain immediate, a function pointer and a real mid-function address beside them, and config.yml's
block_relocations entries in both forms."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import io
import json
import os
import struct
from contextlib import redirect_stdout

from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.objdiff import immreloc as im

TIER = "fixture"

NOP = 0x60000000


def lis(rd, hi):
    return (15 << 26) | (rd << 21) | (hi & 0xFFFF)


def addi(rd, ra, lo):
    return (14 << 26) | (rd << 21) | (ra << 16) | (lo & 0xFFFF)


def code(*words):
    return b"".join(struct.pack(">I", w) for w in words)


SYMBOLS = "\n".join([
    "updateSession = .text:0x803D72F4; // type:function size:0x60 scope:global",
    "openFile = .text:0x803F6A00; // type:function size:0x20 scope:global",
    "fn_8005FE68 = .text:0x8005FE68; // type:function size:0x200 scope:global",
    "helper = .text:0x80070000; // type:function size:0x10 scope:global",
    "OSDisableInterrupts = .text:0x804D0C70; // type:function size:0x14 scope:global",
]) + "\n"
CONFIG = ("object: orig/RMHE08/sys/main.dol\nblock_relocations:\n- target: extabindex:0x80030000\n"
          "  end: extabindex:0x80030050\n")


def target_object():
    """updateSession: +0x10 lis/addi of fn_8005FE68+0x1CC (0x80060034); +0x18 a pointer to helper (no addend);
    +0x24 a real address OSDisableInterrupts+0xC; +0x2C and +0x50 two @eti_80030000+2 sites, 0x20 apart (two
    entries, never one range over the code between). openFile: +0x8 the same constant, uncovered."""
    b = ElfBuilder()
    b.section(".text", bytes(0x80))
    b.symbol("updateSession", ".text", 0, 0x60, type="func")
    b.symbol("openFile", ".text", 0x60, 0x20, type="func")
    for name in ("fn_8005FE68", "helper", "OSDisableInterrupts", "@eti_80030000"):
        b.symbol(name)
    for off, sym, addend in ((0x10, "fn_8005FE68", 0x1CC), (0x18, "helper", 0), (0x24, "OSDisableInterrupts", 0xC),
                             (0x2C, "@eti_80030000", 2), (0x50, "@eti_80030000", 2), (0x68, "fn_8005FE68", 0x1CC)):
        b.reloc(".text", off + 2, sym, "R_PPC_ADDR16_HA", addend)
        b.reloc(".text", off + 6, sym, "R_PPC_ADDR16_LO", addend)
    return b


def ours_object(relocated=False):
    words = [NOP] * 0x18
    words[4], words[5] = lis(3, 0x8006), addi(3, 3, 0x34)
    b = ElfBuilder()
    b.section(".text", code(*words))
    b.symbol("updateSession", ".text", 0, 0x60, type="func")
    if relocated:
        b.symbol("fn_8005FE68")
        b.reloc(".text", 0x12, "fn_8005FE68", "R_PPC_ADDR16_HA", 0x1CC)
    return b


def tree(t, relocated=False):
    t.write("config/RMHE08/symbols.txt", SYMBOLS)
    t.write("config/RMHE08/config.yml", CONFIG)
    t.add_object("Network/session.o", target_object())
    t.add_object("Network/session.o", ours_object(relocated), side="src")
    t.write("build/RMHE08/report.json", json.dumps({"measures": {}, "units": [{"name": "main/Network/session",
            "functions": [{"name": "updateSession", "size": "96", "fuzzy_match_percent": 87.5,
                           "metadata": {"virtual_address": str(0x803D72F4)}}]}]}))


def test_scan(c):
    with testing.FixtureTree() as t:
        tree(t)
        res = im.run(str(t.root), str(t.build_dir / "obj"), str(t.build_dir / "src"))
        rows = {(c_.function, hex(c_.start)): c_ for c_ in res["candidates"]}
        c.check("every relocated immediate is found, the function pointer is not, and sites far apart are separate",
                sorted(rows), sorted([("updateSession", "0x803d7304"), ("updateSession", "0x803d7318"),
                                      ("updateSession", "0x803d7320"), ("updateSession", "0x803d7344"),
                                      ("openFile", "0x803f6a08")]))
        k = rows[("updateSession", "0x803d7304")]
        c.check("the constant: its source range, label, value and shape",
                (hex(k.end), k.target, hex(k.value), k.halves, k.shape), ("0x803d730c", "fn_8005FE68", "0x80060034",
                                                                         ["@ha", "@l"], "error-code"))
        c.check("... our object materialises it as an immediate, so it is proposed, with the function's score",
                (k.ours, k.ours_immediate, k.proposed, k.score), ("ours: immediate 0x80060034 at +0x10", True, True, 87.5))
        c.check("... and its entry is the source/end form config.yml uses", k.entry(),
                "- source: .text:0x803D7304\n  end: .text:0x803D730C")
        real = rows[("updateSession", "0x803d7318")]
        c.check("a real mid-function address is listed for review, never proposed", (real.shape, real.proposed),
                ("", False))
        eti = [rows[("updateSession", "0x803d7320")], rows[("updateSession", "0x803d7344")]]
        c.check("an extabindex label is covered by the target entry", [e.covered_by for e in eti],
                ["target extabindex:0x80030000..0x80030050"] * 2)
        c.check("the uncovered copy elsewhere is new", (rows[("openFile", "0x803f6a08")].covered_by,
                rows[("openFile", "0x803f6a08")].ours), (None, "our object defines no openFile"))
        s = im.summary(res)
        c.check("the summary counts each entry's rediscoveries", (s["covered"], s["new_proposed"], s["new_review"],
                s["entries"]), (2, 2, 1, {"target extabindex:0x80030000..0x80030050": 2}))


def test_ours_relocating_is_not_evidence(c):
    with testing.FixtureTree() as t:
        tree(t, relocated=True)
        res = im.run(str(t.root), str(t.build_dir / "obj"), str(t.build_dir / "src"), units=["Network/session"])
        k = next(x for x in res["candidates"] if x.start == 0x803D7304)
        c.check("our object relocating the same site is not an immediate", (k.ours, k.ours_immediate),
                ("ours relocates it too at +0x14", False))
        c.check("... the error-code shape still proposes it", k.proposed, True)


def test_block_entries_and_cli(c):
    text = CONFIG + "- source: .text:0x803D7564\n  end: .text:0x803D756C\nother: 1\n"
    c.check("both entry forms are read", im.block_entries(text),
            [{"form": "target", "section": "extabindex", "start": 0x80030000, "end": 0x80030050},
             {"form": "source", "section": ".text", "start": 0x803D7564, "end": 0x803D756C}])
    with testing.FixtureTree() as t:
        tree(t)
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = im.main(["--root", str(t.root), "--json"])
        doc = json.loads(buf.getvalue())
        c.check("--json: new proposed entries exit 1 and say not ok", (rc, doc["ok"], doc["summary"]["new_proposed"]),
                (1, False, 2))
        buf = io.StringIO()
        with redirect_stdout(buf):
            im.main(["--root", str(t.root)])
        text = buf.getvalue()
        c.contains("the text form prints a paste-ready entry", text, "- source: .text:0x803F6A08\n  end: .text:0x803F6A10")
        c.check("... and never a review row's entry", "0x803D7318" in text.split("# new block_relocations")[1], False)
        t.write("config/RMHE08/config.yml", CONFIG + "- source: .text:0x803D7300\n  end: .text:0x803D7310\n"
                "- source: .text:0x803F6A08\n  end: .text:0x803F6A10\n")
        with redirect_stdout(io.StringIO()):
            rc = im.main(["--root", str(t.root)])
        c.check("once every proposed site is covered the scan exits 0", rc, 0)


def test_target_runs(c):
    mk = lambda v: im.Candidate("u", "f", ".text", 0, 4, "@etb_X", "extab", 0, v)  # noqa: E731
    c.check("unwind values merge into 16-byte-line runs", im.target_runs([mk(0x80010001), mk(0x80010013),
                                                                         mk(0x80010015), mk(0x80010042)]),
            [("extab", 0x80010000, 0x80010020), ("extab", 0x80010040, 0x80010050)])


if __name__ == "__main__":
    sys.exit(testing.run(globals()))
