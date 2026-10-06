---
id: 54
title: Dolphin's `.map` names a jump table's OWNER and a `__FILE__` emitter
status: works
problem: Idea 53 says to claim a jump table's `.data` range but not whose range it is, so the claim guesses boundaries; and the strongest naming evidence (the `__FILE__` static) needs the emitting TU, while the symbol map gives only the string's address.
tags: [data, symbols]
applies: []
demo:
reviewed: 2026-09-29
related: [25, 53, 67, 70]
---

# 54. Dolphin's `.map` names a jump table's OWNER and a `__FILE__` emitter

**Problem.** Idea 53 says to claim a jump table's `.data` range, but not *whose* range it is - so the claim is a
guess at the boundaries, and two lanes can claim one table or split one TU's data across two units. Separately,
the campaign's strongest naming evidence (class 1, the `__FILE__` static a TU emits) requires knowing which TU
emits the string, and the symbol map only gives you the string's address.

**How it looks.** A `.data` run of N words sits between two registered units' ranges and you cannot tell which one
its `switch` belongs to; or a string `lbl_805CDFC8` = `"menu_item.cpp"` is unowned and you need the unit's file
name.

**Why it works.** The Dolphin runtime dump carries the **original build's local symbols**, and the compiler's own
synthesized names encode what you want (`docs/memory-dump.md`, idea 25):

* `_<fnaddr>switchdataD_<addr>` - the **owner** of the jump table at `<addr>`: the `_<fnaddr>` prefix is the owning
  function's address (not re-checked against function starts for this form, unlike the `s_` form below) and
  `<addr>` is the table it owns. This is idea 53's missing half: 53 tells you to claim the range, this tells you whose it is.
* `_<fnaddr>s_<file>_<addr>` - a `__FILE__` static for `<file>`. **Take the file name and nothing else**: the
  `_<fnaddr>` prefix is *not* the emitter (see the correction below); resolve the emitter from the referrers.

**How to work it.** Ask the dump's map for a symbol by address and read the pattern:

```sh
python tools/symbols/dumpmap.py lookup 0x805cdea8
# 0x805CDEA8 .data dump=_8029e4e0switchdataD_805cdea8 ... map=jumptable_805CDEA8 (object, size 0x68)
python tools/symbols/dumpmap.py lookup 0x805cdfc8
# 0x805CDFC8 .data dump=_802a22a4s_menu_item.cpp_805cdfc8 ... map=lbl_805CDFC8 (object, size 0xE)
```

(Both rows re-run 2026-09-29; `dumpmap.py join` cross-checks a whole map against the dump.) For the emitter of a
`__FILE__` static, use the referrer graph (`tools/units/callers.py <addr>`), or - the decisive test - whether the
string is **single-copy** in the DOL: the static is TU-local, so one copy means one emitter and every function whose
relocations name it belongs to that one TU, however long the span. Several copies are the ambiguous case (a name
two TUs emit), never the reverse.

**Result / Example.** `menu/menu_item.cpp` (2026-09-26): its own `.data` holds `lbl_805CDFC8` = `"menu_item.cpp"`;
the dump's local symbol for that address is `_802a22a4s_menu_item.cpp_805cdfc8`, so the **file name** is naming
evidence without reading a single assert - the prefix 0x802A22A4 is *not* the emitter (it lies inside
`fn_802A1714` at +0xB90). Two lanes then derived the **same** file name independently from that one string, which
is how a real defect was caught: the retired tiler's `--max-bytes` cap had split one translation unit into two
registered proposals, and only the agreeing `__FILE__` evidence made it visible. The same pattern settled a
boundary question the other way: `_8029e4e0switchdataD_805cdea8` (owner 0x8029E4E0, inside the *previous* run)
sitting in a file's own `.data` run proves that TU starts at or before 0x8029E4E0 - a bound, not a measured edge.

**Correction (2026-09-26, evening): the `_<fnaddr>s_<file>_` prefix is not the emitter.** An earlier version of this
idea said `<fnaddr>` names the function emitting the `__FILE__` static, and seven seam reports leaned on it. It is
**wrong**. Measured against the map's own 46 `_<fnaddr>s_<file>_<addr>` entries: **0 of 46** prefixes are a function
start in `symbols.txt`, and of the entries whose string resolves to a referrer **0 of 24** have the prefix among those
referrers. The prefix is an address *inside* a `.text` function that need not be related to the string at all:
`_802dbc48s_cockpit.cpp_805d55c8`'s string is referenced from 0x802DDC04 (`dumpmap.py lookup 0x805d55c8` still
prints that entry). **Only the file name is reliable.** This also outranks a span cap: `menu_infomation.cpp` was
*rejected* by `--source-span-max` for its 65 KB span, and its single copy proved that span is one genuine TU
(0x80308FB4..0x8031A6C0) - three registered files had cut it in three, and the cuts were false.

**Correction (2026-10-06): a `switchdataD` prefix need not be the owner either.** `_8034239cswitchdataD_805e918c`
(`dumpmap.py lookup 0x805E918C`) has its prefix inside `ef/eft050.cpp`'s `fn_8034235C` (+0x40, not a function start),
while the DOL's only referrer of `jumptable_805E918C` is `ef/eft_slot.cpp`'s `enemy_data_grp` (`lis`/`addi` at
0x803439E4/0x803439E8, `callers.py 0x805E918C`), whose `switch` MWCC emits the table for. Settle a table's owner from
its referrers; the prefix is a hint.

**When NOT to apply.** The dump is an oracle for *names and ownership*, never for codegen (`CLAUDE.md`, external
oracles), and it is read-only. The `switchdataD` owner prefix is a bound on where the TU starts, not its edge.
Treat any range you derive from it as a claim to measure before and after (ideas 23/53).

**Trap that came with it.** `objdump -d build/RMHE08/main.elf --start-address=...` annotates every call, but a
regex for `addr: op args` **silently drops `blr` and `nop`**, so a function can look one instruction short and a
length comparison will "confirm" a difference that is not there.

**Demo.** None: the evidence is the runtime dump's symbol names, which a compiled object cannot show.
