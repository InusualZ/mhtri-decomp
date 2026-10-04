"""The `jumptable` invariant: a jump table sits in the unit that dispatches through it.
Spec: docs/tools/spec/invariants.md. CLI: none (an invariant of `splitcheck.py --baseline`)."""
from __future__ import annotations

from tools.splits.invariants.context import FAIL, PASS, UNKNOWN


#: how far past the instruction that forms a jump table's address the dispatch (`lwzx` ... `mtctr` ... `bctr`) may lie
JUMP_WINDOW = 16
BCTR = 0x4E800420


def jumptable_reader_sites(ctx, sym):
    """The sites that READ a jump table: the `addi`/`ori` that forms its address followed, within `JUMP_WINDOW` instructions, by the indexed load
    (`lwzx`/`lwzux` through that register), then `mtctr` and `bctr`.  Any other reference is not a read of the table: a displacement load whose address
    happens to fall inside the symbol (`lhz r0, 8(r3)` through a base that was formed with `lis`) is a struct-field access, not a dispatch."""
    out = []
    for x in ctx.readers(sym):
        w = ctx.dol.word(x)
        if w is None:
            continue
        op = w >> 26
        if op == 14:
            reg = (w >> 21) & 31                              # addi rD, rA, lo
        elif op == 24:
            reg = (w >> 16) & 31                              # ori rA, rS, lo
        else:
            continue
        loaded = False
        for k in range(1, JUMP_WINDOW + 1):
            v = ctx.dol.word(x + 4 * k)
            if v is None:
                break
            if v >> 26 == 31 and (v >> 1) & 0x3FF in (23, 55) and reg in ((v >> 16) & 31, (v >> 11) & 31):
                loaded = True
                continue
            if loaded and v >> 26 == 31 and (v >> 1) & 0x3FF == 467 and (v >> 11) & 0x3FF == 0x120:       # mtctr
                if any(ctx.dol.word(x + 4 * (k + j)) == BCTR for j in range(1, 8)):
                    out.append(x)
                break
    return out


def check_jumptable(ctx, res):
    for s in ctx.symbols:
        if not s["name"].startswith("jumptable_") or s["section"] not in (".data", ".rodata"):
            continue
        u = ctx.owner(s["section"], s["addr"])
        if u is None:
            continue
        targets = [ctx.dol.word(s["addr"] + o) for o in range(0, s["size"] - 3, 4)]
        targets = [t for t in targets if t]
        tu = {ctx.text_owner(t) for t in targets if ctx.fn_at(t)}
        rsites = jumptable_reader_sites(ctx, s) if s["name"].startswith("jumptable_") else ctx.readers(s)
        ru = {ctx.text_owner(x) for x in rsites if ctx.text_owner(x)}
        bad = sorted((x for x in ru if x is not u), key=lambda x: x.name)           # a set of units: name order, so the finding is the same on every run
        if bad:
            res.add(u.name, "jumptable", FAIL, s["addr"], "%s is read by %s, not by this unit" % (s["name"], bad[0].name))
        elif tu and tu != {u}:
            other = sorted(x.name if x else "no unit" for x in tu - {u})
            res.add(u.name, "jumptable", FAIL, s["addr"], "%s branches into %s" % (s["name"], ", ".join(other[:3])))
        elif ru or tu:
            res.add(u.name, "jumptable", PASS)
        else:
            res.add(u.name, "jumptable", UNKNOWN, s["addr"], "%s: no decoded reader and no code target" % s["name"])
