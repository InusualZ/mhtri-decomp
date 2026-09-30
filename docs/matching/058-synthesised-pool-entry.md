---
id: 58
title: A compiler-synthesised pool entry can be claimed only while your unit is its sole referencer
status: works
problem: A unit's `.text` matches 100 % and it cannot flip because its object emits an 8-byte `.sdata2` entry (MWCC's implicit int->double magic `0x4330000080000000`, pooled per TU) that the target's TU never owned; the claim links green while `NonMatching` and fails on the flip.
tags: [data, sections]
applies: []
demo: 058-synthesised-pool-entry.cpp
reviewed: 2026-09-29
related: [23, 29, 59, 70]
---

# 58. A compiler-synthesised pool entry can be claimed only while your unit is its sole referencer

**Problem.** A unit's `.text` matches 100 % and it still cannot flip, because its object emits an 8-byte `.sdata2`
entry - MWCC's implicit int->double magic (`0x4330000080000000`, the constant the int-to-double conversion
sequence subtracts), which the compiler pools per TU - that the target's TU never owned. Claiming the range in
`splits.txt` looks right and links green while the unit is `NonMatching`, but the **flip** fails:

```
mwldeppc.exe Linker Error: undefined: 'lbl_8079A008'   Referenced from 'fn_802751B4' in fn_80273B14.o
```

dtk defines the map's global name only in the *claiming* unit's target object; our `src/` object can emit nothing
but its own local `@519`-style pool entry, so the claim promises a symbol no object provides.

**How it looks.** The pool row `lfd fN, lbl_8079A008@sda21` in retail (a load through the map's global name) against
`lfd fN, @NNN@sda21` in ours (a local); the target object's `.sdata2` has 8 bytes ours lacks, or the reverse.

**Why it happens.** Both routes the playbook already names stop here. Naming the constant in source is not an
escape - `extern "C" const f64 lbl_8079A008 = 4503601774854144.0;` makes MWCC emit **both** the named constant and
its pool copy (`.sdata2` 8 -> 16 B), and the function still loads the *local* copy - and no flag removes the entry:
`-pool off`, `-sdata2 0`, `-sdata2 32768`, `-str reuse` and `-fp_contract off` all keep the local 8 bytes, and
`-sdata2 0` drops it while growing `.text` 92 -> 96 B. What decides it is *who else references the address*.

**How to work it.** Measure the referrers first (`python tools/units/callers.py 0x8079A008`): `0x8079A008` is loaded
by **28 `lfd`s in 9 functions of four registered units** (`Pl/fn_802693C4.cpp`, `Pl/fn_8026FFBC.cpp`,
`Pl/pl_skill.cpp`, `Pl/fn_80273B14.cpp`). **Claim a pool range only while your unit is its sole referencer.** A
*private* entry - one no other unit references - is what the claim is for, and it links: `ef/fn_80101DF4`'s
`0x807966E8-0x807966F0` took that unit from data 20/20 to **28/28** and to `Object(Matching)`, with `main.dol: OK`
and the sha1 unchanged. A *shared* entry means the unit is **not flippable at all** until the original TU is one
registered unit again (here `.text 0x802693C4-0x80276B58`) or the build post-processes the pool symbol into the
map's global name - so write that in the unit header instead of claiming anything. `python
tools/units/datagap.py --flip-blockers` lists the units this class blocks (20 of 228 on 2026-09-26; 25 of 300 scanned units on 2026-09-29, the list is the same kind of unit but no longer the same size). `callers.py 0x8079A008` still resolves the address as an unsplit `.sdata2` object with no registered owner (2026-09-29), which is consistent with the note above: the shared entry is not claimed by any of its four users.

**Result / Example.** `enemy/fn_8012BA00` (`0x80796C68`, shared with `fn_801251D0`/`fn_8012BDF4`/`fn_8021EC74`) and
`stage/fn_802B2978` (`0x8079A460`, shared with `fn_802B2AA0`/`stg_w`) carry exactly that note in their headers.

**When NOT to apply.** Idea 59 is the sibling case that a post-compile step *can* repair (extab names: the entry
always sits in a section the unit claims); a shared pool entry cannot be repaired that way. For an ordinary literal
pool the unit already owns, idea 29 (declare, never define) applies.

**Demonstration.** `058-synthesised-pool-entry.cpp` (`ideas.py demo-check 58`, 2026-09-29): a function returning
`(double)n` loads the magic through an `lfd` from an anonymous `@N` entry (`.sdata2` 8 bytes); adding
`extern const double lbl_8079A008 = 4503601774854144.0;` grows `.sdata2` to 16 bytes with `lbl_8079A008` defined -
but the `lfd` still relocates against the anonymous `@N`, not against the named constant. So naming the constant in
source adds a second copy and does not make the function reference it.
