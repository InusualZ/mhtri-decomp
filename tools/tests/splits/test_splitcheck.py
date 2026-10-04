"""splitcheck's invariants on in-memory mini DOLs: the decoder, each invariant's PASS/FAIL/UNKNOWN, the ctors closure,
pool order, V->S and zigzag seams, jump-table dispatch, `--readers`, the defect ranking (moved from `splitcheck.py`)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import struct

from tools.lib import testing
from tools.splits.invariants.audit import readers_report, run_checks, top_defects
from tools.splits.invariants.context import (FAIL, NA, PASS, UNKNOWN, Ctx, Dol, dataorder_rows, hx, parse_splits,
                                             parse_symbols)
from tools.splits.invariants.ctors import ctors_detail
from tools.splits.invariants.jumptable import jumptable_reader_sites
from tools.splits.invariants.order import file_order_offenders
from tools.splits.invariants.pool import pool_intervals

TIER = "fixture"


def _w(op, rt, ra, imm):
    return (op << 26) | (rt << 21) | (ra << 16) | (imm & 0xFFFF)


def _lis(rd, hi):
    return _w(15, rd, 0, hi)


def _b(site, target, link=False):
    """`b` / `bl` from `site` to `target`."""
    return (18 << 26) | ((target - site) & 0x03FFFFFC) | (1 if link else 0)


BLR = 0x4E800020
NOP = 0x60000000


def _mini(T, words, fns, units, extra_blobs=(), extra_syms=()):
    """A one-text-blob world: `words` at `T`, `fns` = [(name, offset, size)], `units` = splits text; returns `(Ctx, symbols)`."""
    code = struct.pack(">%dI" % len(words), *words)
    dol = _make_dol([(T, code)] + list(extra_blobs))
    syms = parse_symbols(["%s = .text:0x%X; // type:function size:0x%X scope:global" % (n, T + o, z) for n, o, z in fns] + list(extra_syms))
    return Ctx(parse_splits(units), syms, dol, 0x80500000, 0x80600000), syms


def _make_dol(blobs):
    """A minimal DOL image: `blobs` = [(address, bytes)] as text0.. / data0.. sections."""
    head = bytearray(0x100)
    toff, taddr, tsize = [0] * 7, [0] * 7, [0] * 7
    body = bytearray()
    for i, (a, b) in enumerate(blobs[:7]):
        toff[i], taddr[i], tsize[i] = 0x100 + len(body), a, len(b)
        body += b
    head[0x00:0x1C] = struct.pack(">7I", *toff)
    head[0x48:0x64] = struct.pack(">7I", *taddr)
    head[0x90:0xAC] = struct.pack(">7I", *tsize)
    return Dol(bytes(head) + bytes(body))


def test_invariants(c):
    check = c.check

    # -- fixture: two units, text 0x80100000.., sdata2 pool at 0x80300000, ctors at 0x80200000
    T0, S2 = 0x80100000, 0x80300000
    a_fn1 = [_lis(3, 0x8030), _w(48, 1, 3, 0), _w(48, 2, 3, 4), 0x4E800020]                 # A.fn1: reads lit0, lit1
    a_sinit = [_lis(4, 0x8030), _w(48, 1, 4, 4), 0x4E800020, 0x60000000]                      # A.__sinit: reads lit1 (first use later)
    b_fn = [_lis(3, 0x8030), _w(48, 1, 3, 8), _w(48, 2, 3, 4), 0x4E800020]                   # B.fn: reads lit2, lit1 (shared!)
    code = struct.pack(">16I", *(a_fn1 + a_sinit + b_fn + [0x60000000] * 4))
    pool = struct.pack(">fff", 1.5, 2.5, 3.5)
    ctors = struct.pack(">II", T0 + 0x10, T0 + 0x20)                                          # A -> its sinit, B -> fn in B? (wrong: mid-unit)
    eti = struct.pack(">3I", T0, 0x10, 0x80000100) + struct.pack(">3I", T0 + 0x20, 0x10, 0x80000108)
    dol = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti)])
    syms = parse_symbols([
        "A_fn1 = .text:0x%X; // type:function size:0x10 scope:global" % T0,
        "A_sinit = .text:0x%X; // type:function size:0x10 scope:local" % (T0 + 0x10),
        "B_fn = .text:0x%X; // type:function size:0x10 scope:global" % (T0 + 0x20),
        "B_pad = .text:0x%X; // type:function size:0x10 scope:global" % (T0 + 0x30),
        "lit0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2,
        "lit1 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (S2 + 4),
        "lit2 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (S2 + 8),
        "ct = .ctors:0x80200000; // type:object size:0x8 scope:local",
        "@eti_a = extabindex:0x80400000; // type:object size:0xC scope:local",
        "@eti_b = extabindex:0x8040000C; // type:object size:0xC scope:local",
        "@etb_a = extab:0x80000100; // type:object size:0x8 scope:local",
        "@etb_b = extab:0x80000108; // type:object size:0x8 scope:local",
    ])
    sp_text = """Sections:
\t.text       type:code align:32
\textab       type:rodata align:32
\textabindex  type:rodata align:32
\t.ctors      type:rodata align:16
\t.sdata2     type:rodata align:4

u_a.cpp:
\textab       start:0x80000100 end:0x80000108
\textabindex  start:0x80400000 end:0x8040000C
\t.text       start:0x80100000 end:0x80100020
\t.ctors      start:0x80200000 end:0x80200008
\t.sdata2     start:0x80300000 end:0x80300004

u_b.cpp: comment:0
\textab       start:0x80000108 end:0x80000110
\textabindex  start:0x8040000C end:0x80400018
\t.text       start:0x80100020 end:0x80100038
\t.sdata2     start:0x80300004 end:0x8030000C
"""
    sp = parse_splits(sp_text)
    check("parse units", [u.name for u in sp.units], ["u_a.cpp", "u_b.cpp"])
    check("parse attrs kept", sp.units[1].attrs, "comment:0")
    ctx = Ctx(sp, syms, dol, 0x80500000, 0x80600000)
    i0, _ = ctx.data_sym_at(S2)
    check("ref decode: lis + lfs", sorted(ctx.refs.get(i0, [])), [T0 + 4])
    i1, s1 = ctx.data_sym_at(S2 + 4)
    check("ref decode: lit1 read by both units", sorted(ctx.refs[i1]), [T0 + 8, T0 + 0x14, T0 + 0x28])
    res = run_checks(ctx, None)
    check("order pass", res.units["u_a.cpp"]["order"]["status"], PASS)
    check("pool: A's literal lit1 (in B's range) is read by A and B -> B fails", res.units["u_b.cpp"]["pool"]["status"], FAIL)
    check("pool: A reads lit1 too, so the shared literal fails both", res.units["u_a.cpp"]["pool"]["status"], FAIL)
    check("pool edges both ways", sorted(ctx.pool_edges), [("u_a.cpp", "u_b.cpp"), ("u_b.cpp", "u_a.cpp")])
    check("ctors: A's last function is __sinit", res.units["u_a.cpp"]["ctors"]["status"], FAIL)   # 2 words -> multi-TU
    check("ctors: 2 words finding", "2 .ctors words" in res.units["u_a.cpp"]["ctors"]["finding"], True)
    check("ctors: second word targets B", any("u_b.cpp" in i["finding"] for i in res.units["u_a.cpp"]["ctors"]["items"]), True)
    check("extab pass for A and B", (res.units["u_a.cpp"]["extab"]["status"], res.units["u_b.cpp"]["extab"]["status"]), (PASS, PASS))
    eti_bad = struct.pack(">3I", T0, 0x10, 0x80000100) + struct.pack(">3I", T0 + 0x10, 0x10, 0x80000108)
    dol_bad = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti_bad)])
    rx = run_checks(Ctx(sp, syms, dol_bad, None, None, scan=False), None, ["extab"])
    check("extab FAIL for B (function is A's) and A is named", (rx.units["u_b.cpp"]["extab"]["status"], rx.units["u_a.cpp"]["extab"]["status"]), (FAIL, FAIL))
    check("text-cut pass", res.units["u_a.cpp"]["text-cut"]["status"], PASS)
    # order: overlap + cycle
    bad = parse_splits(sp_text.replace("start:0x80100020 end:0x80100038", "start:0x8010001C end:0x80100038"))
    cb = Ctx(bad, syms, dol, 0x80500000, 0x80600000, scan=False)
    rb = run_checks(cb, None, ["order", "text-cut"])
    check("overlap is an order FAIL", rb.units["u_b.cpp"]["order"]["status"], FAIL)
    check("text cut inside a function", rb.units["u_b.cpp"]["text-cut"]["status"], FAIL)
    cyc = parse_splits(sp_text + "\nu_c.cpp:\n\t.text       start:0x80100038 end:0x80100040\n\t.sdata2     start:0x80300000 end:0x80300000\n")
    cyc.units[0].ranges[".ctors"] = [(0x80200010, 0x80200014, "")]
    cyc.units[2].ranges[".ctors"] = [(0x80200000, 0x80200004, "")]
    rc = run_checks(Ctx(cyc, syms, dol, None, None, scan=False), None, ["order"])
    check("cycle between units is an order FAIL", rc.units["u_c.cpp"]["order"]["status"], FAIL)
    # coverage
    sp2 = parse_splits(sp_text.replace("end:0x8030000C", "end:0x80300008"))
    c2 = Ctx(sp2, syms, dol, None, None, scan=False)
    run_checks(c2, None, ["coverage"])
    check("an uncovered symbol is reported", c2.coverage_gaps[".sdata2"]["symbols"], 1)
    # sinit-not-last: the word targets A_fn1 (ends at the middle of A): boundary evidence
    ct2 = struct.pack(">II", T0 + 0x00, 0)
    dol_s = _make_dol([(T0, code), (0x80200000, ct2), (S2, pool), (0x80400000, eti)])
    rs = run_checks(Ctx(sp, syms, dol_s, None, None, scan=False), None, ["ctors"])
    cuts = [i.get("cut_at") for i in rs.units["u_a.cpp"]["ctors"]["items"] if i.get("cut_at")]
    check("ctors: not-last sinit names the cut", (rs.units["u_a.cpp"]["ctors"]["status"], cuts), (FAIL, [T0 + 0x10]))
    # dtors: the crt chain entry is exempt
    sp_d = parse_splits(sp_text.replace("	.ctors      start:0x80200000 end:0x80200008", "	.dtors      start:0x80200000 end:0x80200004"))
    syms_d = syms + parse_symbols(["__destroy_global_chain = .text:0x%X; // type:function size:0x10 scope:global" % (T0 + 0x30)])
    dol_d = _make_dol([(T0, code), (0x80200000, struct.pack(">I", T0 + 0x30)), (S2, pool), (0x80400000, eti)])
    rd = run_checks(Ctx(sp_d, syms_d, dol_d, None, None, scan=False), None, ["dtors"])
    check("dtors: __destroy_global_chain entry passes", rd.units["u_a.cpp"]["dtors"]["status"], PASS)
    # bss: a local object read only by the other unit
    syms_b = syms + parse_symbols(["bssvar = .bss:0x80700000; // type:object size:0x4 scope:local"])
    sp_b = parse_splits(sp_text.replace("	.sdata2     start:0x80300000 end:0x80300004", "	.sdata2     start:0x80300000 end:0x80300004\n\t.bss        start:0x80700000 end:0x80700004", 1))
    cb2 = Ctx(sp_b, syms_b, dol, 0x80500000, 0x80600000, scan=False)
    cb2.refs[cb2.data_sym_at(0x80700000)[0]] = [T0 + 0x24]
    rb2 = run_checks(cb2, None, ["bss"])
    check("bss: local object read only by another unit fails", rb2.units["u_a.cpp"]["bss"]["status"], FAIL)
    # data-order + vtable
    D0 = 0x80600000
    blob = struct.pack(">3I", 0, 0, T0) + b"hello\x00\x00\x00" + struct.pack(">3I", 0, 0, T0 + 0x20)
    dol_v = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti), (D0, blob)])
    syms_v = syms + parse_symbols(["__vt__A = .data:0x%X; // type:object size:0xC scope:global" % D0,
                                   "str_h = .data:0x%X; // type:object size:0x8 scope:local data:string" % (D0 + 12),
                                   "__vt__B = .data:0x%X; // type:object size:0xC scope:global" % (D0 + 20)])
    sp_v = parse_splits(sp_text.replace("	.sdata2     start:0x80300000 end:0x80300004",
                                        "	.sdata2     start:0x80300000 end:0x80300004\n\t.data       start:0x%X end:0x%X" % (D0, D0 + 32), 1))
    cv = Ctx(sp_v, syms_v, dol_v, None, None, scan=False)
    rv = run_checks(cv, dataorder_rows(syms_v), ["data-order", "vtable"])
    check("data-order: a V->S seam inside one unit's .data fails", rv.units["u_a.cpp"]["data-order"]["status"], FAIL)
    check("vtable: B's slot is in unit B, so A's .data holding it fails", rv.units["u_a.cpp"]["vtable"]["status"], FAIL)
    # jump-table ownership
    jt = struct.pack(">II", T0 + 0x04, T0 + 0x24)
    dol3 = _make_dol([(T0, code), (0x80200000, ctors), (S2, pool), (0x80400000, eti), (0x80500000, jt)])
    syms3 = syms + parse_symbols(["jumptable_80500000 = .data:0x80500000; // type:object size:0x8 scope:local"])
    sp3 = parse_splits(sp_text.replace("\t.sdata2     start:0x80300000 end:0x80300004",
                                       "\t.sdata2     start:0x80300000 end:0x80300004\n\t.data       start:0x80500000 end:0x80500008"))
    r3 = run_checks(Ctx(sp3, syms3, dol3, None, None, scan=False), None, ["jumptable"])
    check("jump table branching into another unit fails", r3.units["u_a.cpp"]["jumptable"]["status"], FAIL)
    # gap 1: a `.ctors` word is not a cut at the end of its __sinit - the closure of its local callees ends the TU
    T = 0x80100000
    hdr = "Sections:\n\t.text       type:code align:32\n\t.ctors      type:rodata align:16\n\t.sdata2     type:rodata align:4\n\n"
    ctor_blob = (0x80200000, struct.pack(">I", T + 0x10))

    def ctors_of(end, words, fns):
        c, _s = _mini(T, words, fns, hdr + "u.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n" % (T, T + end),
                      [ctor_blob])
        r = run_checks(c, None, ["ctors"]).units["u.cpp"]["ctors"]
        return r["status"], [i.get("cut_at") for i in r["items"] if i.get("cut_at")]

    # F0 0x00..0x10, sinit = `b ctor` 0x10..0x14, ctor 0x14..0x24, g 0x24..0x34
    thunk = [NOP, NOP, NOP, BLR, _b(T + 0x10, T + 0x14), BLR, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, NOP, BLR]
    fn_thunk = [("F0", 0, 0x10), ("sinit", 0x10, 4), ("ctor", 0x14, 0x10), ("g", 0x24, 0x10)]
    thunk += [NOP] * 4
    check("ctors closure: a `b ctor` sinit whose ctor ends at the unit end passes", ctors_of(0x24, thunk, fn_thunk), (PASS, []))
    check("ctors closure: a unit that goes on after the ctor is cut at L, the function after the closure", ctors_of(0x34, thunk, fn_thunk), (FAIL, [T + 0x24]))
    called = list(thunk)
    called[1] = _b(T + 4, T + 0x24, True)                       # F0 calls g: g belongs to the TU before the sinit
    check("ctors closure: the function at L called from before L is not confirmed", ctors_of(0x34, called, fn_thunk), (UNKNOWN, []))
    # sinit 0x10..0x20 takes the address of the dtor at 0x20 (lis/addi), then `after` 0x28..0x38
    addr = [NOP, NOP, NOP, BLR, _lis(3, T >> 16), _w(14, 3, 3, 0x20), BLR, NOP, BLR, NOP, NOP, BLR, NOP, NOP, NOP, BLR, NOP, NOP, NOP, BLR]
    fn_addr = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("dtor", 0x20, 0x8), ("after", 0x28, 0x10)]
    check("ctors closure: an address-taken dtor after the sinit is in the closure", ctors_of(0x28, addr, fn_addr), (PASS, []))
    check("ctors closure: the tail after an address-taken dtor is cut at L", ctors_of(0x38, addr, fn_addr), (FAIL, [T + 0x28]))
    plain = [NOP, NOP, NOP, BLR, BLR, NOP, NOP, NOP, NOP, NOP, NOP, BLR, NOP, NOP, NOP, NOP]
    check("ctors closure: a sinit with no local callee cuts at its own end", ctors_of(0x34, plain, [("F0", 0, 0x10), ("sinit", 0x10, 0x8), ("h", 0x18, 0x10)]),
          (FAIL, [T + 0x18]))

    # gap 1b: inline virtual functions emitted after the sinit are the unit's own: slots of a vtable the unit stores
    DV = 0x80600000
    vt_hi, vt_lo = DV >> 16, DV & 0xFFFF
    slots_code = [NOP, NOP, NOP, BLR, _lis(3, vt_hi), _w(14, 3, 3, vt_lo), BLR, NOP, BLR, NOP, BLR, NOP]   # F0, sinit (stores __vt__V), slot1, slot2
    slot_fns = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("slot1", 0x20, 8), ("slot2", 0x28, 8)]
    vt_blob = struct.pack(">4I", 0, 0, T + 0x20, T + 0x28)
    vt_sym = ["__vt__V = .data:0x%X; // type:object size:0x10 scope:global" % DV]
    vt_units = (hdr + "\t.data       type:rodata align:8\n\nu.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n"
                "\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, DV, DV + 0x10))

    def slot_run(words, cut_to=0x30, fns=None):
        c, _s = _mini(T, words, fns or slot_fns, vt_units.replace("end:0x%X\n\t.ctors" % (T + 0x30), "end:0x%X\n\t.ctors" % (T + cut_to)),
                      [ctor_blob, (DV, vt_blob)], vt_sym)
        r = run_checks(c, None, ["ctors"]).units["u.cpp"]["ctors"]
        return r["status"], [i.get("cut_at") for i in r["items"] if i.get("cut_at")]

    check("ctors closure: the slots of a vtable the unit stores, after the sinit, are the unit's own", slot_run(slots_code), (PASS, []))
    no_store = list(slots_code)
    no_store[4], no_store[5] = NOP, NOP
    check("ctors closure: slots of a vtable the unit never stores are not (cut at the first slot)", slot_run(no_store), (FAIL, [T + 0x20]))
    check("ctors closure: a unit that goes on past the slot run is cut after the run's end", slot_run(slots_code + [NOP] * 4, 0x40), (FAIL, [T + 0x30]))
    # a unit that ENDS inside its own slot run is early: the closure overshoots the unit end (was a PASS)
    check("ctors closure: a cut placed over the unit's own slots (unit ends at the first slot) is flagged at the run's end",
          slot_run(slots_code, 0x20), (FAIL, [T + 0x30]))
    check("ctors closure: an overshoot inside the alignment slack is not flagged (unit ends 0x28, run ends 0x30)", slot_run(slots_code, 0x28), (PASS, []))
    check("ctors closure: a unit that ends at its sinit, over slots of a vtable it never stores, is not an overshoot", slot_run(no_store, 0x20), (PASS, []))
    big_fns = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("slot1", 0x20, 0x100), ("slot2", 0x120, 8)]
    check("ctors closure: a large function after the end is another TU's member, not an own inline slot",
          slot_run(slots_code + [NOP] * 0x40, 0x20, big_fns), (PASS, []))
    mixed_fns = [("F0", 0, 0x10), ("sinit", 0x10, 0x10), ("slot1", 0x20, 8), ("slot2", 0x28, 0x100)]
    vt_blob2 = struct.pack(">4I", 0, 0, T + 0x20, T + 0x28)
    check("ctors closure: a run that opened with a stub goes on over a large slot (the cut is at the run's end)",
          slot_run(slots_code + [NOP] * 0x40, 0x20, mixed_fns), (FAIL, [T + 0x128]))

    # gap 2: a lis/addi that only forms a pool address is not a read of the literal; a load through it is
    S2 = 0x80300000
    hi, lo = S2 >> 16, S2 & 0xFFFF
    x_fn = [_lis(3, hi), _w(14, 3, 3, lo), _b(T + 8, T + 0x40, True), BLR]                          # X: passes the address of lit0 to a call
    y_fn = [_lis(3, hi), _w(48, 1, 3, lo), BLR, NOP]                                                # Y: lis + lfs lit0
    z_fn = [_lis(3, hi), _w(14, 3, 3, lo), _w(48, 1, 3, 0), BLR]                                    # Z: lis + addi + lfs 0(r3)
    w_fn = [_lis(3, hi), _w(14, 3, 3, lo), _b(T + 0x38, T + 0x40, True), _w(48, 1, 3, 0)]            # W: address, a call, then lfs 0(r3): r3 is gone
    u_txt = hdr + "".join("%s.cpp:\n\t.text       start:0x%X end:0x%X\n%s" % (n, T + o, T + o + 0x10, ("\t.sdata2     start:0x%X end:0x%X\n" % (S2, S2 + 4)) if n == "y" else "")
                          for n, o in (("x", 0), ("y", 0x10), ("z", 0x20), ("w", 0x30)))
    cg, _s = _mini(T, x_fn + y_fn + z_fn + w_fn, [("x", 0, 0x10), ("y", 0x10, 0x10), ("z", 0x20, 0x10), ("w", 0x30, 0x10)], u_txt,
                   [(S2, struct.pack(">f", 1.5))], ["lit0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2])
    lit = cg.data_sym_at(S2)[1]
    check("pool decode: every lis/addi/load stays a reference", sorted(cg.readers(lit)), [T + 4, T + 0x14, T + 0x24, T + 0x34])
    check("pool decode: only loads read the literal (Y's lis+lfs, Z's addi+lfs; not X's address, not W's stale r3)",
          sorted(cg.literal_readers(lit)), [T + 0x14, T + 0x28])
    rg = run_checks(cg, None, ["pool"])
    check("pool decode: a unit that only forms the address is not a second reader of the literal",
          rg.units.get("x.cpp", {}).get("pool", {}).get("status", NA), NA)

    # gap 2b: a literal in an UNOWNED `.sdata2` pool past the last owned range is still decoded (the map's data extent counts)
    un_txt = hdr + "y.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T, T + 0x10)
    cu, _s = _mini(T, y_fn + [NOP] * 4, [("y", 0, 0x10)], un_txt, [(S2, struct.pack(">f", 1.5))],
                   ["lit0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2])
    lit_u = cu.data_sym_at(S2)[1]
    check("pool decode: a literal of an unowned pool beyond every owned range is read by the text that loads it",
          (cu.owner(".sdata2", S2), sorted(cu.literal_readers(lit_u))), (None, [T + 4]))

    # gap 3: pool first-use order is judged per function (scheduling reorders two loads of one function)
    lits2 = struct.pack(">ff", 1.5, 2.5)
    s2_syms = ["l0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % S2,
               "l1 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (S2 + 4)]
    pool_txt = hdr + "p.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n" % (T, T + 0x20, S2, S2 + 8)
    one_fn = [_lis(3, hi), _w(48, 2, 3, 4), _w(48, 1, 3, 0), BLR, BLR, NOP, NOP, NOP]              # l1 loaded before l0, in ONE function
    two_fn = [_lis(3, hi), _w(48, 2, 3, 4), BLR, NOP, _lis(3, hi), _w(48, 1, 3, 0), BLR, NOP]      # f0 uses l1, the later f1 uses l0
    for label, words, want in (("the same function", one_fn, PASS), ("an earlier function", two_fn, FAIL)):
        cp, _s = _mini(T, words, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], pool_txt, [(S2, lits2)], s2_syms)
        rp = run_checks(cp, None, ["pool"]).units["p.cpp"]["pool"]
        check("pool order: l1 first used before l0 in %s -> %s" % (label, want), rp["status"], want)

    # gap 7: a V->S seam whose vtable's constructor is in the closure of the unit's own sinit is not a seam
    D0 = 0x80600000
    dhdr = hdr + "\t.data       type:rodata align:8\n\n"
    vblob = struct.pack(">3I", 0, 0, T) + b"hello\x00\x00\x00" + struct.pack(">3I", 0, 0, T + 4)
    vsyms = ["__vt__A = .data:0x%X; // type:object size:0xC scope:global" % D0,
             "str_h = .data:0x%X; // type:object size:0x8 scope:local data:string" % (D0 + 12),
             "__vt__B = .data:0x%X; // type:object size:0xC scope:global" % (D0 + 20)]
    vunits = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, D0, D0 + 32)
    vhi, vlo = (D0 + 20) >> 16, (D0 + 20) & 0xFFFF
    store = [_lis(3, vhi), _w(14, 3, 3, vlo), BLR, NOP]                                               # a ctor storing the address of __vt__B
    fns_v = [("F0", 0, 0x10), ("sinit", 0x10, 4), ("ctor", 0x20, 0x10)]
    for label, sinit_word, want in (("the sinit calls a ctor that stores __vt__B", _b(T + 0x10, T + 0x20), PASS),
                                    ("the store is in a function the sinit never reaches", BLR, FAIL)):
        words = [NOP, NOP, NOP, BLR, sinit_word, NOP, NOP, NOP] + store + [NOP] * 4
        cv2, sv = _mini(T, words, fns_v, vunits, [(0x80200000, struct.pack(">I", T + 0x10)), (D0, vblob)], vsyms)
        rv2 = run_checks(cv2, dataorder_rows(sv), ["data-order"])
        check("data-order: %s" % label, rv2.units["v.cpp"]["data-order"]["status"], want)

    # local-static: a scope:local object is one TU's; read by two units it fails both, and no proposed cut may sit between its readers
    LS = 0x80700000
    ls_hdr = "Sections:\n\t.text       type:code align:32\n\t.bss        type:bss align:8\n\n"
    ls_units = ls_hdr + "u1.cpp:\n\t.text       start:0x%X end:0x%X\n\t.bss        start:0x%X end:0x%X\n\nu2.cpp:\n\t.text       start:0x%X end:0x%X\n" \
        % (T, T + 0x10, LS, LS + 8, T + 0x10, T + 0x20)
    rd_fn = [_lis(3, LS >> 16), _w(14, 3, 3, LS & 0xFFFF), BLR, NOP]                               # forms the address of the object
    ls_syms = ["stat = .bss:0x%X; // type:object size:0x4 scope:local" % LS, "stat2 = .bss:0x%X; // type:object size:0x4 scope:local" % (LS + 4)]
    cl, _s = _mini(T, rd_fn + rd_fn + rd_fn + rd_fn, [("a", 0, 0x10), ("b", 0x10, 0x10), ("c", 0x20, 0x10), ("d", 0x30, 0x10)], ls_units, (), ls_syms)
    rl = run_checks(cl, None, ["local-static"])
    check("local-static: a local object read from two units fails both", (rl.units["u1.cpp"]["local-static"]["status"],
                                                                           rl.units["u2.cpp"]["local-static"]["status"]), (FAIL, FAIL))
    cg2, _s = _mini(T, rd_fn + rd_fn + rd_fn + rd_fn, [("a", 0, 0x10), ("b", 0x10, 0x10), ("c", 0x20, 0x10), ("d", 0x30, 0x10)], ls_units, (),
                    [ls_syms[0].replace("scope:local", "scope:global"), ls_syms[1]])
    rg2 = run_checks(cg2, None, ["local-static"])
    check("local-static: a global object read from two units is not a finding", rg2.units.get("u1.cpp", {}).get("local-static", {}).get("status", NA), NA)
    # the update-form store leaves rA = the effective address: `stwu r0, lo(r3)` then `stw r0, 4(r3)` reads/writes stat2, not a stale lis
    upd = [_lis(3, LS >> 16), _w(37, 0, 3, 0x10), _w(36, 0, 3, 4), BLR]
    cu2, _s = _mini(T, upd + [NOP] * 12, [("a", 0, 0x10), ("b", 0x10, 0x30)], ls_units, (),
                    ls_syms + ["stat3 = .bss:0x%X; // type:object size:0x4 scope:local" % (LS + 0x10),
                               "stat4 = .bss:0x%X; // type:object size:0x4 scope:local" % (LS + 0x14)])
    check("decode: an update-form store moves rA, so the next displacement is relative to the updated address (not to the lis value)",
          [sorted(cu2.readers(cu2.data_sym_at(a)[1])) for a in (LS + 4, LS + 0x10, LS + 0x14)], [[], [T + 4], [T + 8]])
    cl2, _s = _mini(T, [_lis(3, LS >> 16), _b(T + 4, T + 0x30, True), _w(36, 0, 3, 0), BLR] + [NOP] * 12, [("a", 0, 0x10), ("b", 0x10, 0x30)], ls_units, (), ls_syms)
    check("decode: a `lis` register does not survive a call", cl2.readers(cl2.data_sym_at(LS)[1]), [])

    # reproduce rows: the per-word closure line and the pool interval line print the numbers a finding quotes
    cdet, _s = _mini(T, slots_code, slot_fns, vt_units.replace("end:0x%X\n\t.ctors" % (T + 0x30), "end:0x%X\n\t.ctors" % (T + 0x20)),
                     [ctor_blob, (DV, vt_blob)], vt_sym)
    check("ctors_detail: names the word, the closure end, the own-slot end and the unit end",
          ctors_detail(cdet, cdet.by_name["u.cpp"]),
          [".ctors word 0x80200000 = 0x%X (sinit, ends 0x%X): closure ends 0x%X, with own vtable slots 0x%X; unit text ends 0x%X"
           % (T + 0x10, T + 0x20, T + 0x20, T + 0x30, T + 0x20)])
    eq_pool = struct.pack(">ff", 1.5, 1.5)
    pairs_fn = [_lis(3, hi), _w(48, 1, 3, 0), BLR, NOP, _lis(3, hi), _w(48, 1, 3, 4), BLR, NOP]
    cpi, _s = _mini(T, pairs_fn, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], pool_txt, [(S2, eq_pool)], s2_syms)
    check("pool_intervals: one value at two pool addresses prints both reads and the interval a TU starts in",
          pool_intervals(cpi), ["pooldup value 0x3fc00000: l0 0x%X (last read 0x%X) and l1 0x%X (first read 0x%X); a TU starts in (0x%X, 0x%X]: 1 function starts"
                                % (S2, T + 4, S2 + 4, T + 0x14, T, T + 0x10)])

    two_txt = hdr + "p1.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n\np2.cpp:\n\t.text       start:0x%X end:0x%X\n" % (T, T + 0x10, S2, S2 + 8, T + 0x10, T + 0x20)
    c2u, _s = _mini(T, pairs_fn, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], two_txt, [(S2, eq_pool)], s2_syms)
    check("pool_intervals: a pair read by two units says so, and --unit filters by either reader",
          ([l.endswith("[already two units: p1.cpp | p2.cpp]") for l in pool_intervals(c2u)], len(pool_intervals(c2u, lambda n: n == "p2.cpp")), len(pool_intervals(c2u, lambda n: n == "zzz"))),
          ([True], 1, 0))

    # checker rules for data a unit holds
    # pool: a unit claiming `.sdata` strings and `.sdata2` literals has two pools; first-use order is judged inside each, never across
    SD = 0x80200100
    two_pools = hdr + "p.cpp:\n\t.text       start:0x%X end:0x%X\n\t.sdata      start:0x%X end:0x%X\n\t.sdata2     start:0x%X end:0x%X\n" % (T, T + 0x20, SD, SD + 8, S2, S2 + 4)
    words_sd = [_lis(3, hi), _w(48, 1, 3, 0), BLR, NOP, _lis(3, SD >> 16), _w(14, 3, 3, SD & 0xFFFF), BLR, NOP]      # f0 loads the literal, the later f1 takes the string's address
    cps, _s = _mini(T, words_sd, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], two_pools, [(SD, b"hello\0\0\0"), (S2, struct.pack(">f", 1.5))],
                    ["str0 = .sdata:0x%X; // type:object size:0x8 scope:local data:string" % SD, s2_syms[0]])
    check("pool order: a string of `.sdata` first used after a `.sdata2` literal that sits above it is two pools, not one inverted pool",
          run_checks(cps, None, ["pool"]).units["p.cpp"]["pool"]["status"], PASS)
    both_inv = two_pools.replace("end:0x%X\n" % (S2 + 4), "end:0x%X\n" % (S2 + 8))
    words_inv = [_lis(3, hi), _w(48, 2, 3, 4), BLR, NOP, _lis(3, hi), _w(48, 1, 3, 0), BLR, NOP]                         # `.sdata2`: l1 first used before l0
    cpi, _s = _mini(T, words_inv, [("f0", 0, 0x10), ("f1", 0x10, 0x10)], both_inv, [(SD, b"hello\0\0\0"), (S2, lits2)],
                    ["str0 = .sdata:0x%X; // type:object size:0x8 scope:local data:string" % SD] + s2_syms)
    check("pool order: an inversion inside one section is still a FAIL, reported at the literal that breaks the order", (lambda r: (r["status"], "first use of l1" in r["finding"]))(
          run_checks(cpi, None, ["pool"]).units["p.cpp"]["pool"]), (FAIL, True))
    # data-order: a strong V->S row asserts a boundary in [first string, next vtable): a unit ending inside the gap can end at it
    for label, dend, want in (("the unit ends where the second vtable group starts", D0 + 20, PASS), ("the unit ends inside the string gap", D0 + 16, PASS),
                              ("the unit holds the second vtable group too", D0 + 32, FAIL)):
        vu3 = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, D0, dend)
        cv3, sv3 = _mini(T, [NOP] * 12, fns_v, vu3, [(D0, vblob)], vsyms)
        check("data-order V->S: %s -> %s" % (label, want), run_checks(cv3, dataorder_rows(sv3), ["data-order"]).units["v.cpp"]["data-order"]["status"], want)
    # data-order: a zigzag pair (adjacent vtables whose first slots go up) whose vtable the unit's own sinit closure stores is instantiated, not a seam
    zblob = struct.pack(">3I", 0, 0, T) + struct.pack(">3I", 0, 0, T + 4)
    zsyms = ["__vt__A = .data:0x%X; // type:object size:0xC scope:global" % D0, "__vt__B = .data:0x%X; // type:object size:0xC scope:global" % (D0 + 12)]
    zunits = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.ctors      start:0x80200000 end:0x80200004\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x30, D0, D0 + 24)
    zstore = [_lis(3, (D0 + 12) >> 16), _w(14, 3, 3, (D0 + 12) & 0xFFFF), BLR, NOP]                    # a ctor storing the address of __vt__B
    for label, sinit_word, want in (("the sinit calls a ctor that stores __vt__B", _b(T + 0x10, T + 0x20), PASS),
                                    ("the store is in a function the sinit never reaches", BLR, FAIL)):
        czz, szz = _mini(T, [NOP, NOP, NOP, BLR, sinit_word, NOP, NOP, NOP] + zstore + [NOP] * 4, fns_v, zunits, [(0x80200000, struct.pack(">I", T + 0x10)), (D0, zblob)], zsyms)
        check("data-order zigzag: %s -> %s" % (label, want), run_checks(czz, dataorder_rows(szz), ["data-order"]).units["v.cpp"]["data-order"]["status"], want)
    # data-order: a zigzag pair whose classes' member functions alternate in the text is one TU (no text cut separates them); an ordered pair is a seam
    ffns = [("g%d" % i, 0x10 * i, 0x10) for i in range(6)]
    funits = dhdr + "v.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n" % (T, T + 0x60, D0, D0 + 32)
    fsyms = ["__vt__A = .data:0x%X; // type:object size:0x10 scope:global" % D0, "__vt__B = .data:0x%X; // type:object size:0x10 scope:global" % (D0 + 16)]
    for label, aslots, bslots, want in (("alternating members", (T + 0x10, T + 0x30), (T + 0x20, T + 0x40), PASS),
                                        ("A's members all before B's", (T + 0x00, T + 0x10), (T + 0x20, T + 0x30), FAIL)):
        cfz, sfz = _mini(T, [NOP] * 24, ffns, funits, [(D0, struct.pack(">4I", 0, 0, *aslots) + struct.pack(">4I", 0, 0, *bslots))], fsyms)
        check("data-order zigzag: %s -> %s" % (label, want), run_checks(cfz, dataorder_rows(sfz), ["data-order"]).units["v.cpp"]["data-order"]["status"], want)
    # jumptable: the foreign reader a finding names is the first by name (a set of units has no order of its own)
    JT = 0x80500000
    jt_units = "Sections:\n\t.text       type:code align:32\n\t.data       type:data align:8\n\nown.cpp:\n\t.text       start:0x%X end:0x%X\n\t.data       start:0x%X end:0x%X\n\n" % (T, T + 0x10, JT, JT + 8)
    names8 = ["r%d.cpp" % k for k in (5, 2, 7, 0, 3, 6, 1, 4)]
    jt_units += "".join("%s:\n\t.text       start:0x%X end:0x%X\n\n" % (n, T + 0x20 * (k + 1) - 0x10, T + 0x20 * (k + 2) - 0x10) for k, n in enumerate(names8))
    jt_reader = [_lis(3, JT >> 16), _w(14, 3, 3, JT & 0xFFFF), 0x5480103A, (31 << 26) | (3 << 16) | (23 << 1), 0x7C0903A6, 0x4E800420, NOP, NOP]       # the dispatch
    cjt, _s = _mini(T, [NOP] * 4 + jt_reader * 8, [("own", 0, 0x10)] + [("f%d" % k, 0x10 + 0x20 * k, 0x20) for k in range(8)], jt_units, [(JT, bytes(8))],
                    ["jumptable_80500000 = .data:0x%X; // type:object size:0x8 scope:local" % JT])
    check("jumptable: the finding names the foreign reader first by name, not first in a set", run_checks(cjt, None, ["jumptable"]).units["own.cpp"]["jumptable"]["finding"],
          "jumptable_80500000 is read by r0.cpp, not by this unit")

    # --readers: one line per map symbol of a range with its owner and the units that read it
    DS = 0x80300000
    wa = [_lis(3, DS >> 16), _w(48, 1, 3, 0), BLR, NOP, NOP, NOP, NOP, NOP]                       # fn A (0x00..0x20) reads l0
    wb = [_lis(3, DS >> 16), _w(48, 1, 3, 4), BLR, NOP, NOP, NOP, NOP, NOP]                       # fn B (0x20..0x40) reads l1
    r_txt = hdr + "big.cpp:" + chr(10) + "	.text       start:0x%X end:0x%X" % (T, T + 0x40) + chr(10) + "	.sdata2     start:0x%X end:0x%X" % (DS, DS + 8) + chr(10)
    rctx, _rs = _mini(T, wa + wb, [("A", 0, 0x20), ("B", 0x20, 0x20)], r_txt, [(DS, struct.pack(">ff", 1.5, 2.5))],
                      ["l0 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % DS,
                       "l1 = .sdata2:0x%X; // type:object size:0x4 scope:local data:float" % (DS + 4)])
    check("--readers: owner and decoded readers of every symbol in the range (A reads l0, B reads l1)",
          [" ".join(l.split()) for l in readers_report(rctx, ".sdata2:0x%X-0x%X" % (DS, DS + 8))],
          [".sdata2 0x%08X l0 size 0x4 owner big.cpp readers big.cpp x1" % DS, ".sdata2 0x%08X l1 size 0x4 owner big.cpp readers big.cpp x1" % (DS + 4)])

    # ---- phase 3 review fixes: the decoder, the `__sinit` definers, the pool's initialiser order, the file-order check, the jump-table dispatch ----
    TT = 0x80100000
    HDR = "Sections:\n\t.text       type:code align:32\n\t.ctors      type:rodata align:4\n\t.data       type:data align:8\n\t.sdata      type:data align:8\n\t.sdata2     type:rodata align:4\n\t.sbss2      type:bss align:4\n\n"

    def ADDI(rt, ra, imm):
        return _w(14, rt, ra, imm)

    def STW(rs, ra, d):
        return _w(36, rs, ra, d)

    def LHZ(rt, ra, d):
        return _w(40, rt, ra, d)

    def X31(rt, ra, rb, xo):
        return (31 << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (xo << 1)

    def MR(ra, rs):
        return X31(rs, ra, rs, 444)                              # or rA, rS, rS

    def ADD(rt, ra, rb):
        return X31(rt, ra, rb, 266)

    def LWZX(rt, ra, rb):
        return X31(rt, ra, rb, 23)

    MTCTR_R0, BCTR_W, SLWI_R0_R4_2 = 0x7C0903A6, 0x4E800420, 0x5480103A

    def dsym(name, sec, addr, size=4, extra="", scope="global"):
        return "%s = %s:0x%X; // type:object size:0x%X scope:%s%s" % (name, sec, addr, size, scope, extra)

    def unit_text(*blocks):
        return HDR + "".join("%s:\n%s\n" % (nm, "".join("\t%-11s start:0x%X end:0x%X\n" % (sec, a, b) for sec, a, b in rr)) for nm, rr in blocks)

    # decoder (rule 3): a `lis` in a callee-saved register outlives the 200-instruction window, until the register is written; `mr` copies it
    DD = 0x80580000
    long_fn = [_lis(18, DD >> 16)] + [NOP] * 250 + [ADDI(24, 18, DD & 0xFFFF), BLR]                       # r18 is callee-saved: still live at the addi
    vol_fn = [_lis(5, DD >> 16)] + [NOP] * 250 + [ADDI(6, 5, DD & 0xFFFF), BLR]                           # r5 is volatile: the window ended
    mr_fn = [_lis(5, DD >> 16), MR(18, 5), ADDI(24, 18, DD & 0xFFFF), BLR]                                 # mr r18, r5 copies the lis
    nomr_fn = [_lis(5, DD >> 16), NOP, ADDI(24, 18, DD & 0xFFFF), BLR]                                     # r18 never held it
    dead_fn = [_lis(18, DD >> 16), ADD(18, 4, 5), ADDI(24, 18, DD & 0xFFFF), BLR]                          # add r18, r4, r5 overwrote the lis
    long_ori = [_lis(18, DD >> 16)] + [NOP] * 250 + [_w(24, 18, 24, DD & 0xFFFF), BLR]                      # ori r24, r18, lo: the same window rule
    words, fns, off = [], [], 0
    for nm, w in (("long", long_fn), ("vol", vol_fn), ("mr", mr_fn), ("nomr", nomr_fn), ("dead", dead_fn), ("long_ori", long_ori)):
        w = w + [NOP] * (-len(w) % 4)
        fns.append((nm, off, 4 * len(w)))
        words += w
        off += 4 * len(w)
    cdec, _sd = _mini(TT, words, fns, unit_text(("u.cpp", [(".text", TT, TT + off)])), [(DD, bytes(8))], [dsym("g", ".data", DD, 8)])
    gi, _gs = cdec.data_sym_at(DD)
    fn_of = lambda c, x: c.fn_at(x)["name"]
    check("decode: a lis in a callee-saved register outlives the 200-instruction window; a volatile one does not; mr copies it; an overwrite kills it",
          [fn_of(cdec, x) for x in sorted(cdec.refs.get(gi, []))], ["long", "mr", "long_ori"])

    # decoder (rule 3): a function that forms two section starts (`_f_sbss2`, `_f_sdata2`: start-up / module-loader code) reads none of the first symbols
    SB2, SD2, SD1 = 0x80594000, 0x80596000, 0x80592000
    lk = [_lis(30, SB2 >> 16), _lis(29, SD2 >> 16), _lis(28, SD1 >> 16), ADDI(30, 30, SB2 & 0xFFFF), ADDI(29, 29, SD2 & 0xFFFF), ADDI(28, 28, SD1 & 0xFFFF), BLR, NOP]
    one = [_lis(3, SB2 >> 16), ADDI(3, 3, SB2 & 0xFFFF), BLR, NOP]                                          # one section start: an ordinary reference
    two = [_lis(3, SB2 >> 16), _lis(4, SD2 >> 16), ADDI(3, 3, SB2 & 0xFFFF), ADDI(4, 4, SD2 & 0xFFFF)]       # two: below the threshold, both stay
    clk, _sl = _mini(TT, lk + one + two, [("start", 0, 0x20), ("one", 0x20, 0x10), ("two", 0x30, 0x10)], unit_text(("u.cpp", [(".text", TT, TT + 0x40)])),
                     [(SB2, bytes(8)), (SD2, bytes(8)), (SD1, bytes(8))],
                     [dsym("sb2_first", ".sbss2", SB2, 8), dsym("sd2_first", ".sdata2", SD2, 8), dsym("sd_first", ".sdata", SD1, 8)])
    check("decode: the section starts a module loader forms (three or more: _f_sbss2, _f_sdata2, _f_sdata) are linker operands, not reads; a function with fewer keeps them",
          sorted((s["name"], sorted(fn_of(clk, x) for x in clk.refs.get(i, []))) for i, s in enumerate(clk.data_syms) if s["name"] in ("sb2_first", "sd2_first", "sd_first")),
          [("sb2_first", ["one", "two"]), ("sd2_first", ["two"]), ("sd_first", [])])

    # decoder: stores and the calls that pass an address; the `__sinit` definers (rule 1)
    CT = 0x80200000
    A1, A2, A3, A4, A5, AW, AV = (0x80580000 + 0x20 * k for k in range(7))
    sinit = [_lis(4, A1 >> 16), STW(0, 4, A1 & 0xFFFF),                                                    # 0x00 a store into g1 defines it
             _lis(3, A2 >> 16), ADDI(3, 3, A2 & 0xFFFF), _b(TT + 0x10, TT + 0x80, True),                   # 0x08 ctor(this = &g2)
             _lis(3, A3 >> 16), ADDI(3, 3, A3 & 0xFFFF), _lis(5, A4 >> 16), ADDI(5, 5, A4 & 0xFFFF),       # 0x14 __register_global_object(&g3, dtor, &g4)
             _b(TT + 0x24, TT + 0xA0, True),
             _lis(4, AV >> 16), ADDI(4, 4, AV & 0xFFFF), _lis(5, AW >> 16), STW(4, 5, AW & 0xFFFF),        # 0x28 the vtable is only the VALUE stored into a word
             _lis(3, A5 >> 16), ADDI(3, 3, A5 & 0xFFFF), _b(TT + 0x40, TT + 0x80)]                          # 0x38 lis/addi r3; b ctor (a tail call)
    words = sinit + [NOP] * (0x80 // 4 - len(sinit)) + [BLR] + [NOP] * 7 + [BLR] + [NOP] * 3
    cdf, _s2 = _mini(TT, words, [("sinit_a", 0, 0x80), ("ctor", 0x80, 0x20), ("__register_global_object", 0xA0, 0x10)],
                     unit_text(("a.cpp", [(".text", TT, TT + 0xB0), (".ctors", CT, CT + 4)])), [(CT, struct.pack(">I", TT)), (0x80580000, bytes(0x100))],
                     [dsym("g1", ".bss", A1), dsym("g2", ".bss", A2), dsym("g3", ".bss", A3), dsym("g4", ".bss", A4), dsym("g5", ".bss", A5), dsym("word", ".sbss", AW),
                      dsym("vt", ".data", AV, 0x10)])
    got = {s["name"]: sorted(cdf.definers(s)) for s in cdf.data_syms if s["name"] in ("g1", "g2", "g3", "g4", "g5", "word", "vt")}
    check("sinit definers: a store, a ctor's this, __register_global_object's object and node, a tail-called ctor's this define; a vtable stored as a value does not",
          got, {"g1": ["a.cpp"], "g2": ["a.cpp"], "g3": ["a.cpp"], "g4": ["a.cpp"], "g5": ["a.cpp"], "word": ["a.cpp"], "vt": []})
    check("decode: the passes record the site, the register and the callee (r3 = &g2 into the ctor at +0x80)",
          [(hex(x), r, hex(c)) for x, r, c in cdf.pass_refs[cdf.data_sym_at(A2)[0]]], [(hex(TT + 0x10), 3, hex(TT + 0x80))])

    SS = 0x80790100
    sb = [_lis(3, SS >> 16), ADDI(3, 3, SS & 0xFFFF), _b(TT + 8, TT + 0x10, True), BLR]                      # sinit: bl f(&str) - a string is an argument
    cstr, _cs = _mini(TT, sb + [BLR, NOP, NOP, NOP], [("sb_sinit", 0, 0x10), ("sb_ctor", 0x10, 0x10)],
                      unit_text(("s.cpp", [(".text", TT, TT + 0x20), (".ctors", CT, CT + 4)])), [(CT, struct.pack(">I", TT)), (SS, bytes(8))],
                      [dsym("str", ".sdata", SS, 8, " data:string")])
    check("sinit definers: a string literal passed to a call in a sinit is an argument, not an object the unit constructs",
          [(s["name"], sorted(cstr.definers(s))) for s in cstr.data_syms if s["name"] == "str"], [("str", [])])

    # pool order (rule 4): a string a pointer initialiser of the unit's own table names is used first; the same table in another unit's data is not
    S0, S1, TB = 0x80790000, 0x80790008, 0x80590000
    pf = [_lis(3, S1 >> 16), ADDI(3, 3, S1 & 0xFFFF), BLR, NOP, _lis(3, S0 >> 16), ADDI(3, 3, S0 & 0xFFFF), BLR, NOP]            # f0 reads s1, f1 reads s0: s0 follows s1
    for label, tab, want in (("no table: s0 is first used after s1 (an inversion)", None, FAIL),
                             ("a table of the unit's own data points at s0: s0 is used first", "own", PASS),
                             ("the table is another unit's data: no help", "other", FAIL)):
        own_r = [(".text", TT, TT + 0x20), (".sdata", S0, S0 + 0x10)] + ([(".data", TB, TB + 8)] if tab == "own" else [])
        oth_r = [(".text", TT + 0x20, TT + 0x40)] + ([(".data", TB, TB + 8)] if tab == "other" else [])
        csp, _sp = _mini(TT, pf + [BLR] + [NOP] * 7 + [BLR] + [NOP] * 7, [("f0", 0, 0x10), ("f1", 0x10, 0x10), ("o", 0x20, 0x20)],
                         unit_text(("own.cpp", own_r), ("other.cpp", oth_r)), [(S0, bytes(0x10)), (TB, struct.pack(">II", S0, 0))],
                         [dsym("s0", ".sdata", S0, 8, " data:string"), dsym("s1", ".sdata", S1, 8, " data:string"), dsym("tbl", ".data", TB, 8)])
        check("pool: %s" % label, run_checks(csp, None, ["pool"]).units["own.cpp"]["pool"]["status"], want)

    # file order (rule 10): the ranges of a section, by address, must have non-decreasing file positions - a data-only unit has only its file position
    O2 = 0x80300000
    ou = unit_text(*[("%s.cpp" % nm, [(".sdata2", O2 + 4 * k, O2 + 4 * k + 4)]) for nm, k in (("a", 0), ("c", 1), ("b", 2), ("z", 3))])
    co1, _o1 = _mini(TT, [BLR], [("f", 0, 4)], ou, [], [])
    check("order: units in file order are in link order", file_order_offenders(co1), [])
    ou2 = unit_text(*[("%s.cpp" % nm, [(".sdata2", O2 + 4 * k, O2 + 4 * k + 4)]) for nm, k in (("z", 3), ("a", 0), ("b", 1), ("c", 2))])
    co2, _o2 = _mini(TT, [BLR], [("f", 0, 4)], ou2, [], [])
    check("order: a data-only unit placed first in the file but last by address is out of link order (the fewest ranges that must move)",
          [(o[1], hx(o[2]), o[4]) for o in file_order_offenders(co2)], [("z.cpp", hx(O2 + 12), "c.cpp")])
    check("order: the offender is an `order` FAIL of its own unit and of nobody else",
          sorted((n, r["order"]["status"]) for n, r in run_checks(co2, None, ["order"]).units.items() if "order" in r),
          [("a.cpp", PASS), ("b.cpp", PASS), ("c.cpp", PASS), ("z.cpp", FAIL)])

    # jump tables (rule 11): a reader is the indexed dispatch, not any reference whose address falls inside the symbol
    JT2 = 0x80580100
    own_fn = [_lis(3, JT2 >> 16), ADDI(3, 3, JT2 & 0xFFFF), SLWI_R0_R4_2, LWZX(0, 3, 0), MTCTR_R0, BCTR_W, NOP, NOP]            # the dispatch
    field_fn = [_lis(3, JT2 >> 16), LHZ(0, 3, (JT2 & 0xFFFF) + 8), BLR, NOP]                                                     # lhz r0, 8(r3): a struct field that falls in the symbol
    for label, foreign, want, find in (("a foreign lhz through lis is a field access, the unit's own dispatch is the reader", field_fn, PASS, None),
                                       ("a foreign dispatch is a foreign reader", own_fn, FAIL, "jumptable_80580100 is read by foreign.cpp, not by this unit")):
        cj, _sj = _mini(TT, own_fn + foreign + [NOP] * (0x40 // 4 - len(foreign)), [("own", 0, 0x20), ("f", 0x20, 0x20), ("g", 0x40, 0x20)],
                        unit_text(("own.cpp", [(".text", TT, TT + 0x20), (".data", JT2, JT2 + 0x20)]), ("foreign.cpp", [(".text", TT + 0x20, TT + 0x60)])),
                        [(JT2, struct.pack(">8I", *([TT] * 8)))], [dsym("jumptable_80580100", ".data", JT2, 0x20, scope="local")])
        r = run_checks(cj, None, ["jumptable"]).units["own.cpp"]["jumptable"]
        check("jumptable: %s" % label, (r["status"], r["finding"] if want == FAIL else None), (want, find))
    check("jumptable_reader_sites: only the dispatch counts (the addi that forms the address, followed by lwzx, mtctr, bctr)",
          [hex(x) for x in jumptable_reader_sites(cj, next(s for s in cj.data_syms if s["name"].startswith("jumptable_")))], [hex(TT + 4), hex(TT + 0x24)])

    # ranking
    td = top_defects(ctx, res, 50)
    check("defects are ranked by score", [d["score"] for d in td] == sorted((d["score"] for d in td), reverse=True) and bool(td), True)


def test_rows(c):
    from tools.splits.invariants.context import Results
    res = Results()
    res.add("u_a.cpp", "pool", PASS)
    res.add("u_a.cpp", "pool", FAIL, 0x80300004, "literal lit1 is also read by u_b.cpp")
    res.add("u_a.cpp", "order", NA)
    res.add("u_b.cpp", "bss", UNKNOWN, None, "no object of the range is read")
    c.check("one lib.findings Row per unit and invariant, in invariant order, the worst verdict with its finding",
            [(r.name, r.status, r.detail, r.evidence) for r in res.rows()],
            [("u_a.cpp: order", "SKIP", "", "0 pass, 0 fail"),
             ("u_a.cpp: pool", "FAIL", "0x80300004 literal lit1 is also read by u_b.cpp", "1 pass, 1 fail"),
             ("u_b.cpp: bss", "UNKNOWN", "no object of the range is read", "0 pass, 0 fail")])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
