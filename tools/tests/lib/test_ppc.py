"""lib.ppc: field decode, branch targets, the infer predicates, the reference scanner, the text register decode."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import struct

from tools.lib import ppc, testing

TIER = "fixture"


# --- hand assemblers (the infer_selftest encoders, plus what the scanner needs) ---------------------------------

def d_form(op, rt, ra, imm):
    return (op << 26) | (rt << 21) | (ra << 16) | (imm & 0xFFFF)


def lis(rt, imm): return d_form(15, rt, 0, imm)
def addi(rt, ra, imm): return d_form(14, rt, ra, imm)
def li(rt, imm): return d_form(14, rt, 0, imm)
def ori(ra, rs, imm): return d_form(24, rs, ra, imm)
def lwz(rt, ra, d): return d_form(32, rt, ra, d)
def lwzu(rt, ra, d): return d_form(33, rt, ra, d)
def stw(rs, ra, d): return d_form(36, rs, ra, d)
def stb(rs, ra, d=0): return d_form(38, rs, ra, d)
def lfs(frt, ra, d): return d_form(48, frt, ra, d)
def lmw(rt, ra, d=0): return d_form(46, rt, ra, d)
def stmw(rs, ra, d=0): return d_form(47, rs, ra, d)
def stwu(rs, ra, d): return d_form(37, rs, ra, d)
def mr(ra, rs): return (31 << 26) | (rs << 21) | (ra << 16) | (rs << 11) | (444 << 1)
def mflr(rt): return (31 << 26) | (rt << 21) | (8 << 16) | (0x2A6 & 0x7FF)
def add(rt, ra, rb): return (31 << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (266 << 1)
def b(disp, link=False, aa=False): return (18 << 26) | (disp & 0x03FFFFFC) | (2 if aa else 0) | (1 if link else 0)
def bc(bo, bi, disp, link=False): return (16 << 26) | (bo << 21) | (bi << 16) | (disp & 0xFFFC) | (1 if link else 0)
def rlwinm(rs, ra, sh, mb, me, rc=0): return (21 << 26) | (rs << 21) | (ra << 16) | (sh << 11) | (mb << 6) | (me << 1) | rc
def clrlwi(rs, ra, n, rc=0): return rlwinm(rs, ra, 0, n, 31, rc)
def srwi(rs, ra, n, rc=0): return rlwinm(rs, ra, 32 - n, n, 31, rc)
def slwi(rs, ra, n): return rlwinm(rs, ra, n, 0, 31 - n)
def fmadds(t, a, b_, c): return (59 << 26) | (t << 21) | (a << 16) | (b_ << 11) | (c << 6) | (29 << 1)
def fmuls(t, a, c): return (59 << 26) | (t << 21) | (a << 16) | (c << 6) | (25 << 1)
def fadds(t, a, b_): return (59 << 26) | (t << 21) | (a << 16) | (b_ << 11) | (21 << 1)
def psq_lx(t, a, b_): return (4 << 26) | (t << 21) | (a << 16) | (b_ << 11) | (6 << 1)


BLR = 0x4E800020
NOP = 0x60000000
BASE = 0x80004000


def code(*ws):
    return struct.pack(">%dI" % len(ws), *ws)


def at(i):
    return BASE + 4 * i


# --- decode -------------------------------------------------------------------------------------------------------

def test_fields(c):
    w = lwz(5, 31, -8)
    i = ppc.decode(w, 0x80001000)
    c.check("op/rt/ra/si/d of lwz r5,-8(r31)", (i.op, i.rt, i.ra, i.si, i.d), (32, 5, 31, -8, -8))
    c.check("the module field helpers agree", (ppc.op(w), ppc.rt(w), ppc.ra(w), ppc.si(w)), (32, 5, 31, -8))
    c.check("ui is unsigned", ppc.decode(ori(3, 3, 0x8000)).ui, 0x8000)
    psq = (56 << 26) | (1 << 21) | (3 << 16) | (0xFF8 & 0xFFF)
    c.check("psq_l has a 12-bit signed displacement", ppc.displacement(psq), -8)
    c.check("signed()", (ppc.signed(0xFFFF, 16), ppc.signed(0x7FFF, 16), ppc.signed(0x02000000, 26)),
            (-1, 0x7FFF, -0x02000000))
    c.check("Insn is a frozen value: equal by address and word", ppc.decode(w, 4) == ppc.Insn(4, w), True)
    c.raises("Insn is frozen", Exception, setattr, i, "word", 0)
    c.check("width/d_form/load/store", (i.width, i.d_form(), i.is_load(), i.is_store()), (4, True, True, False))
    c.check("lfd width 8", ppc.decode(d_form(50, 1, 3, 0)).width, 8)
    c.check("a non-memory op has no width", ppc.decode(add(3, 4, 5)).width, None)


def test_branches(c):
    c.check("bl forward", ppc.branch_target(0x80001008, 0x48000279), 0x80001280)
    c.check("b forward", ppc.branch_target(0x800010AC, 0x4800000D & ~1), 0x800010B8)
    c.check("bl backward wraps", ppc.branch_target(0x800010A8, 0x4BFFFF59), 0x80001000)
    c.check("bc uses the 16-bit BD field", ppc.branch_target(0x80002000, bc(12, 2, -0x20)), 0x80001FE0)
    c.check("an absolute branch has no relative target", ppc.branch_target(0x80001000, b(0x100, aa=True)), None)
    c.check("a non-branch has no target", ppc.branch_target(0x80001000, 0x9421FFF0), None)
    i = ppc.decode(b(0x40, link=True), 0x80001000)
    c.check("decode: is_call/is_branch_link/target", (i.is_call(), i.is_branch_link(), i.is_branch(), i.target),
            (True, True, True, 0x80001040))
    c.check("blr is a branch, not a call", (ppc.decode(BLR).is_branch(), ppc.decode(BLR).is_call()), (True, False))
    c.check("bctrl is a call but not a `bl`", (ppc.decode(0x4E800421).is_call(), ppc.decode(0x4E800421).is_branch_link()),
            (True, False))


def test_infer_predicates(c):
    c.check("rlwinm aliases", [ppc.rlwinm_alias(w) for w in (clrlwi(3, 4, 24), srwi(3, 4, 8), slwi(3, 4, 2),
                                                           rlwinm(3, 4, 0, 0, 30), rlwinm(3, 4, 0, 0, 31))],
            ["clrlwi", "srwi", "slwi", "clrrwi", "rlwinm"])
    c.check("a non-rlwinm is spelled rlwinm", ppc.rlwinm_alias(lwz(3, 4, 0)), "rlwinm")
    rec = ppc.decode(clrlwi(3, 4, 24, rc=1))
    c.check("record clrlwi: record form, still a clr mask", (rec.is_record(), rec.is_clr_mask()), (True, True))
    c.check("non-record", ppc.decode(clrlwi(3, 4, 24)).is_record(), False)
    c.check("shift", ppc.decode(srwi(3, 4, 8)).is_shift(), True)
    c.check("narrow store", ppc.decode(stb(4, 3)).is_narrow_store(), True)
    c.check("stmw/lmw opcodes", (ppc.decode(stmw(31, 1)).op, ppc.decode(lmw(31, 1)).op), (47, 46))
    c.check("psq_lx", ppc.decode(psq_lx(31, 1, 0)).is_psq_indexed(), True)
    c.check("fmadds is an FMA, fmuls is not", (ppc.decode(fmadds(1, 2, 3, 4)).is_fma(), ppc.decode(fmuls(1, 2, 3)).is_fma()),
            (True, False))
    c.check("fmul / fadd", (ppc.decode(fmuls(1, 2, 3)).is_fmul(), ppc.decode(fadds(1, 2, 3)).is_faddsub()), (True, True))
    c.check("lis", (ppc.decode(lis(3, 0x8050)).is_lis(), ppc.decode(d_form(15, 3, 4, 1)).is_lis()), (True, False))
    c.check("decode_li", (ppc.decode_li(li(4, -3)), ppc.decode_li(addi(4, 3, 1))), ((4, -3), None))


def test_shapes(c):
    c.check("prologues", [ppc.looks_like_prologue(w) for w in (stwu(1, 1, -0x10), mflr(0), stmw(27, 1, 8), BLR, None)],
            [True, True, True, False, False])
    c.check("a lone blr is a dead epilogue", (ppc.is_dead_epilogue(code(BLR)), ppc.is_dead_epilogue(code(BLR, BLR)),
                                              ppc.is_dead_epilogue(code(NOP)), ppc.is_dead_epilogue(b""), ppc.is_dead_epilogue(None)),
            (True, False, False, False, False))
    c.check("stwu_frame is the positive N of `stwu rS,-N(rA)`, None for anything else",
            [ppc.stwu_frame(w) for w in (stwu(1, 1, -0x10), stwu(1, 1, -0x1F0), stwu(3, 4, 0x8), mflr(0), BLR)],
            [0x10, 0x1F0, -0x8, None, None])


def test_written_reg_and_sda(c):
    c.check("written_reg: add writes rD, slwi writes rA, a store writes nothing",
            (ppc.written_reg(add(7, 3, 4)), ppc.written_reg(slwi(4, 9, 2)), ppc.written_reg(stw(3, 1, 8))), (7, 9, None))
    c.check("written_reg: mulli/subfic (7, 8) write rD", (ppc.written_reg(d_form(7, 6, 3, 2)), ppc.written_reg(d_form(8, 5, 3, 0))),
            (6, 5))
    words = [(at(0), lis(13, 0x8079)), (at(1), ori(13, 13, 0x8E20)), (at(2), lis(2, 0x807A)), (at(3), addi(2, 2, -0x2560))]
    c.check("find_sda_bases: ori for r13, addi (signed) for r2", ppc.find_sda_bases(words), (0x80798E20, 0x8079DAA0))
    c.check("find_sda_bases: nothing formed", ppc.find_sda_bases([(at(0), NOP)]), (None, None))


def test_scan_refs(c):
    data = lambda t: 0x80500000 <= t < 0x80600000  # noqa: E731
    sda13, sda2 = 0x80598000, 0x805A0000
    words = [
        lis(3, 0x8050),            # 0
        addi(3, 3, 0x10),          # 1  forms 0x80500010 (a reference, not a read)
        lwz(4, 3, 4),              # 2  loads 0x80500014 through the formed register
        lwz(5, 13, -0x10),         # 3  sda21 read 0x80597FF0
        stw(5, 2, 0x8),            # 4  sda2 write 0x805A0008
        lis(31, 0x8051),           # 5  callee-saved: survives the call
        lis(6, 0x8052),            # 6  volatile: dies at the call
        b(0x100, link=True),       # 7  bl, r3 holds 0x80500010 -> a pass
        lwz(7, 31, 0x20),          # 8  lis r31 + lwz: 0x80510020 (ref + load)
        lwz(8, 6, 0x30),           # 9  r6's lis died: nothing
        mr(9, 31),                 # 10 r9 takes r31's lis
        stw(0, 9, 0x40),           # 11 lis r9 + stw: 0x80510040 (ref + store)
        lfs(1, 31, 0x50),          # 12 float load 0x80510050
    ]
    loads, stores, passes = {}, {}, {}
    refs = ppc.scan_refs(code(*words), BASE, sda13, sda2, data, (), loads, stores, passes)
    c.check("references", sorted((hex(t), [hex(s) for s in v]) for t, v in refs.items()),
            sorted([(hex(0x80500010), [hex(at(1))]), (hex(0x80597FF0), [hex(at(3))]), (hex(0x805A0008), [hex(at(4))]),
                    (hex(0x80510020), [hex(at(8))]), (hex(0x80510040), [hex(at(11))]), (hex(0x80510050), [hex(at(12))])]))
    c.check("loads: through a formed register, r13, lis+lwz, lfs",
            sorted(hex(t) for t in loads), sorted(hex(t) for t in (0x80500014, 0x80597FF0, 0x80510020, 0x80510050)))
    c.check("the addi that only forms an address is not a read", 0x80500010 in loads, False)
    c.check("stores: r2 and a copied lis", sorted(hex(t) for t in stores), sorted(hex(t) for t in (0x805A0008, 0x80510040)))
    c.check("a bl passes the formed r3", passes, {0x80500010: [(at(7), 3, (at(7) + 0x100) & 0xFFFFFFFF)]})

    # a function start forgets the lis; the window expires a volatile lis; an update form rebases rA
    words = [lis(3, 0x8050), NOP, lwz(4, 3, 8)]
    c.check("a function start between the lis and the load forgets it",
            dict(ppc.scan_refs(code(*words), BASE, None, None, data, (at(2),))), {})
    far = [lis(3, 0x8050)] + [NOP] * (ppc.LIS_WINDOW + 1) + [lwz(4, 3, 8)]
    c.check("a volatile lis expires after LIS_WINDOW", dict(ppc.scan_refs(code(*far), BASE, None, None, data)), {})
    far31 = [lis(31, 0x8050)] + [NOP] * (ppc.LIS_WINDOW + 1) + [lwz(4, 31, 8)]
    c.check("a callee-saved lis does not", list(ppc.scan_refs(code(*far31), BASE, None, None, data)), [0x80500008])
    upd = [lis(3, 0x8050), lwzu(4, 3, 0x10), lwz(5, 3, 4)]
    c.check("an update form leaves rA at the effective address",
            sorted(ppc.scan_refs(code(*upd), BASE, None, None, data)), [0x80500010, 0x80500014])
    clob = [lis(3, 0x8050), add(3, 4, 5), lwz(6, 3, 0)]
    c.check("an X-form write kills the lis", dict(ppc.scan_refs(code(*clob), BASE, None, None, data)), {})
    c.check("is_data filters", dict(ppc.scan_refs(code(lis(3, 0x8070), lwz(4, 3, 0)), BASE, None, None, data)), {})


def test_scan_calls(c):
    fns = (at(0), at(4))
    words = [b(0x10, link=True), b(0, link=False), BLR, NOP, b(-0x10, link=True), b(0), BLR]
    c.check("calls to another function's start; a branch to the own start is a loop",
            ppc.scan_calls(code(*words), BASE, fns), [(at(0), at(4)), (at(4), at(0))])


def test_materialisations(c):
    words = [lis(3, 0x8000), addi(3, 3, 0x1280), lis(4, 0x8000), NOP, NOP, NOP, NOP, ori(4, 4, 0x10),
             lis(5, 0x8000), ori(5, 6, 0x10)]
    c.check("lis+addi/ori pairs in the same register within the window",
            ppc.materialisations(code(*words), BASE, 16), [(at(1), 0x80001280)])
    c.check("a wider window admits the far pair", ppc.materialisations(code(*words), BASE, 32),
            [(at(1), 0x80001280), (at(7), 0x80000010)])
    c.check("the low half is added unsigned (the phantom scan's rule, not the hardware's sign extension)",
            ppc.materialisations(code(lis(3, 0x8000), addi(3, 3, 0x8000)), BASE), [(at(1), 0x80008000)])


def test_decode_rw(c):
    c.check("li writes", ppc.decode_rw("li", "r3, 0x0"), (set(), {3}, False, False, True))
    c.check("addi: dest first", ppc.decode_rw("addi", "r4, r3, 0x10")[:2], ({3}, {4}))
    c.check("lwz: base read, dest written", ppc.decode_rw("lwz", "r5, 0x8(r31)")[:2], ({31}, {5}))
    c.check("stw: both read", ppc.decode_rw("stw", "r5, 0x8(r31)")[:2], ({5, 31}, set()))
    c.check("lmw writes the run", ppc.decode_rw("lmw", "r29, 0x8(r1)")[1], {29, 30, 31})
    c.check("lwzx reads both bases", ppc.decode_rw("lwzx", "r3, r4, r5")[:2], ({4, 5}, {3}))
    c.check("bl is a call", ppc.decode_rw("bl", "foo")[2:], (True, True, True))
    c.check("beq is a branch, not a call", ppc.decode_rw("beq", ".L_1")[2:], (True, False, True))
    c.check("float forms are inert", ppc.decode_rw("fadds", "f1, f2, f3"), (set(), set(), False, False, True))
    c.check("an unknown mnemonic is undecoded", ppc.decode_rw("dcbz_l", "r3, r4")[4], False)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
