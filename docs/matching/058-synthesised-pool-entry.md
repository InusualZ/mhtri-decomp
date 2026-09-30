---
id: 58
title: A compiler-synthesised pool entry can be claimed only while your unit is its sole referencer
status: works
problem: a unit's `.text` matches 100 % and it still cannot flip, because its object emits an 8-byte `.sdata2` entry - MWCC's implicit int->double magic (`0x4330000080000000`), which the compiler pools per TU - that the target's TU never owned. Claiming the range in `splits.txt` looks right and links green while the unit is `NonMatching`, but the **flip** fails:
tags: [data, sections]
applies: []
demo:
---

# 58. A compiler-synthesised pool entry can be claimed only while your unit is its sole referencer

Problem: a unit's `.text` matches 100 % and it still cannot flip, because its object emits an 8-byte `.sdata2`
entry - MWCC's implicit int->double magic (`0x4330000080000000`), which the compiler pools per TU - that the
target's TU never owned. Claiming the range in `splits.txt` looks right and links green while the unit is
`NonMatching`, but the **flip** fails:

```
mwldeppc.exe Linker Error: undefined: 'lbl_8079A008'   Referenced from 'fn_802751B4' in fn_80273B14.o
```

dtk defines the map's global name only in the *claiming* unit's target object; our `src/` object can emit
nothing but its own local `@519` pool entry, so the claim promises a symbol no object provides.

Why try it: both routes the playbook already names stop here. Naming the constant in source is not an escape -
`extern "C" const f64 lbl_8079A008 = 4503601774854144.0;` makes MWCC emit **both** the named constant and its
pool copy (`.sdata2` 8 -> 16 B) - and no flag removes the entry: `-pool off`, `-sdata2 0`, `-sdata2 32768`,
`-str reuse` and `-fp_contract off` all keep the local 8 bytes, and `-sdata2 0` drops it while growing `.text`
92 -> 96 B. What decides it is *who else references the address*, so measure that first: `0x8079A008` is loaded
by **28 `lfd`s in 9 functions of four registered units** (`Pl/fn_802693C4.cpp`, `Pl/fn_8026FFBC.cpp`,
`Pl/pl_skill.cpp`, `Pl/fn_80273B14.cpp`).

Result: **claim a pool range only while your unit is its sole referencer.** A *private* entry - one no other
unit references - is the case the claim is for, and it links: `ef/fn_80101DF4`'s `0x807966E8-0x807966F0` took
that unit from data 20/20 to **28/28** and to `Object(Matching)`, with `main.dol: OK` and the sha1 unchanged.
A *shared* entry means the unit is **not flippable at all** until the original TU is one registered unit again
(here `.text 0x802693C4-0x80276B58`) or the build post-processes the pool symbol into the map's global name -
so write that in the unit header instead of claiming anything.

Example: `enemy/fn_8012BA00` (`0x80796C68`, shared with `fn_801251D0`/`fn_8012BDF4`/`fn_8021EC74`) and
`stage/fn_802B2978` (`0x8079A460`, shared with `fn_802B2AA0`/`stg_w`) carry exactly that note;
`tools/units/datagap.py --flip-blockers` lists the units this class blocks (20 of 228 on 2026-09-26).
