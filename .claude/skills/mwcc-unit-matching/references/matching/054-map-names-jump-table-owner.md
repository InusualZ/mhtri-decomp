---
id: 54
title: Dolphin's `.map` names a jump table's OWNER and a `__FILE__` emitter
status: works
problem: Section 53 says to claim a jump table's `.data` range, but not *whose* range it is - so the claim is a guess at the boundaries, and two lanes can claim one table or split one TU's data across two units. Separately, the campaign's strongest naming evidence (class 1, the `__FILE__` static a TU emits) requires knowing which *function* emits the string, and the symbol map only gives you the string's address.
tags: [data, symbols]
applies: []
demo:
---

# 54. Dolphin's `.map` names a jump table's OWNER and a `__FILE__` emitter

**Problem.** Section 53 says to claim a jump table's `.data` range, but not *whose* range it is - so the claim is a
guess at the boundaries, and two lanes can claim one table or split one TU's data across two units. Separately,
the campaign's strongest naming evidence (class 1, the `__FILE__` static a TU emits) requires knowing which
*function* emits the string, and the symbol map only gives you the string's address.

**Why try it.** The Dolphin runtime dump carries the **original build's local symbols**, and the compiler's own
synthesized names encode exactly what you want:

* `_<fnaddr>switchdataD_<addr>` - the **owner** of the jump table at `<addr>`. This is section 53's missing half:
  53 tells you to claim the range, this tells you whose it is, which turns the claim into evidence.
* `_<fnaddr>s_<file>_<addr>` - a `__FILE__` static for `<file>`. **Take the file name; the `_<fnaddr>` prefix is
  not the emitter** (see the correction below). The campaign's naming-evidence class 1, resolved in one query -
  but resolve the *emitter* from the referrers, not from the prefix.

**Result.** Ask the dump's map for a symbol by address (`dumpmap`) and read the pattern: for `switchdataD` the
`_<fnaddr>` prefix is the owning function and the trailing `<addr>` is the data it owns, so one query answers
"whose table is this". For the `s_<file>_` form take the **file name** and nothing else - the prefix is not the
emitter, and the emitter comes from the referrer graph (or from the string being single-copy, which is the
decisive test: one copy in the DOL means one emitter, so every function whose relocations name it is in that one
TU, however long the span).

**Trap that came with it.** `objdump -d build/RMHE08/main.elf --start-address=...` annotates every call, but a
regex for `addr: op args` **silently drops `blr` and `nop`**, so a function can look one instruction short and a
length comparison will "confirm" a difference that is not there.

**Example.** `menu/menu_item.cpp` (2026-09-26). Its own `.data` holds `lbl_805CDFC8` = `"menu_item.cpp"`; the dump's
local symbol for that address is `_802a22a4s_menu_item.cpp_805cdfc8`, i.e. the **file name** is naming evidence
class 1 without reading a single assert - the prefix 0x802A22A4 is *not* the emitter (it lies inside
`fn_802A1714` at +0xb90). Two lanes then derived the
**same** file name independently from that one string, which is how a real defect was caught: `attribute.py`'s
`--max-bytes` cap had split one translation unit into two registered proposals, and only the agreeing `__FILE__`
evidence made it visible. The same pattern settled a boundary question the other way: `_8029e4e0switchdataD_805cdea8`
(owner 0x8029E4E0, which lies inside the *previous* run) sitting in a file's own `.data` run proves that TU starts
at or before 0x8029E4E0 - a bound, not a measured edge, which is why the left edge stayed where it was.

**Correction (2026-09-26, evening): the `_<fnaddr>s_<file>_` prefix is not the emitter.** The claim above that
`<fnaddr>` names the function emitting the `__FILE__` static is **wrong**, and seven seam reports leaned on it.
Measured against the map's own 46 `_<fnaddr>s_<file>_<addr>` entries: **0 of 46** prefixes are a function start in
`symbols.txt`, and of the entries whose string resolves to a referrer **0 of 24** have the prefix among those
referrers. The prefix is an address *inside* a `.text` function (`_802a22a4` sits at `fn_802A1714`+0xb90) that need
not be related to the string at all: `_802dbc48s_cockpit.cpp_805d55c8`'s string is referenced from 0x802DDC04.
**Only the file name is reliable.** For the emitter, read the referrer graph - and the strongest form of that
evidence is a **single copy in the DOL**: the static is TU-local, so one copy means one emitter and every function
whose relocations name it belongs to that one TU. Several copies are the ambiguous case (a name two TUs emit),
never the reverse. This is also the test that outranks a span cap: the `__FILE__` name `menu_infomation.cpp` had
been *rejected* by `--source-span-max` for its 65 KB span, and its single copy proved that span is one genuine TU
(0x80308FB4..0x8031A6C0) - three registered files had cut it in three, and the two cuts are false.
