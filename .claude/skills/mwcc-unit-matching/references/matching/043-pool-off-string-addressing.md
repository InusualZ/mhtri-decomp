---
id: 43
title: Per-object `lis`/`addi` addressing is `-pool off` (or `-str` without `pool`) - `-pool` shares one base across 3+ objects
status: works
problem: Retail materialises each string or table with its own `lis`/`addi` pair, where ours shares one base register (`lis r31,base@ha ; addi r3,r31,delta`, or an `@stringBase0` symbol) - a few bytes of `.text` short and the `.rela.text` records name a base symbol retail never had.
tags: [flags, data]
applies: [Wii/1.3]
demo: 043-pool-off-string-addressing.cpp
reviewed: 2026-09-29
related: [23, 44, 45, 64]
---

# 43. Per-object `lis`/`addi` addressing is `-pool off` (or `-str` without `pool`) - `-pool` shares one base across 3+ objects

**Problem.** Retail materialises each string or table with its own `lis`/`addi` pair (`lis` = load-immediate-shifted,
the `@ha` half of a 32-bit address; `addi` adds the `@l` half); ours addresses several of them off one base register
(`lis r31,base@ha ; addi r31,r31,base@l`, then `addi r3,r31,delta`). The object is a few bytes short of the target
and the `.rela.text` records name a base symbol retail never had (`@stringBase0`, or the section symbol
`...data.0` / `...rodata.0`).

**How it looks.**

```
ours:    lis r31,base@ha ; addi r31,r31,base@l ; addi r3,r31,0 ; ... ; addi r3,r31,18 ; ... ; addi r3,r31,37
retail:  lis r3,str1@ha ; addi r3,r3,str1@l ; ... ; lis r3,str2@ha ; addi r3,r3,str2@l ; ... (one pair per object)
```

Every reference in retail carries its own `R_PPC_ADDR16_HA/LO` pair against its own symbol (`@NN` for a string, the
table's name for a table); ours share one symbol and one saved register.

**Why it happens.** Two independent switches produce the same shape, and they need telling apart:

* **`-pool` (on by default; `-pool off` disables).** A function that references **three or more distinct
  data objects of one non-small-data section** defined in the same TU addresses all of them off ONE base register
  (the section symbol, `...data.0` for strings, `...rodata.0` for `static const` tables). `-pool off` gives each its
  own `lis`/`addi` pair. Below three objects the flag does nothing.
* **`-str ...,pool`** (a sub-option of `-str`) merges every string of the unit into one object named `@stringBase0`
  and addresses them base + displacement; with it two strings already share a base and `-pool off` cannot split
  them (it is one object). Without `pool` each literal is its own object, and `-pool` then behaves as above.

**How to work it.** First read the target: is each string/table addressed by its own pair? Then, per unit:
(1) check the library's `-str` flags - if it has `pool`, drop it (per unit; there is no source pragma); (2) if the
function references three or more distinct objects of one section and still shares a base, add `-pool off` (per
library or unit, evidence in a comment next to it); (3) check the section the data lands in (idea 44) and the
`.data` layout. Probe the shape first (below): a flag that leaves the object byte-identical is noise.

**When NOT to apply.** Measured 2026-09-29 (scratch matrix in `.pi/notes/idea43.md`), `-pool off` is a
byte-identical no-op when: a function references fewer than three distinct objects of one section (two strings,
two tables - straight-line, in branches, around calls or in a loop); the objects are `extern` (not defined in this
TU); the strings are small enough for `.sdata` (SDA `li r3,@NN@sda21` - never `lis`); or the strings are merged by
`-str ...pool` (one `@stringBase0` object). A project audit (2026-09-29) measured the real object of every unit
carrying the flag: it changed **Camellia** (`.text` 0x5F64 with the flag, 0x5F34 without; 7 functions 100 % ->
98.9-99.4 % without it - the S-box tables) and `Network/network_socket_streams`
(`NetworkMultipleUdp_receive` 94.06 % with, 89.53 % without - its log strings), and it was byte-identical on `RSO/runtime`
and seven other `Network` objects, where the flag was removed. Do not add it "in case".

**The reverse case (a unit that needs `pool`).** When the target's strings are packed contiguously behind one `@stringBase0` (offsets 0,
0x15, 0x3a ... with no alignment padding), a 4 B string such as `"%s
"` sits inside that pool, and the first string is addressed
`addi r3,r30,@stringBase0@l` with no displacement, the unit was built with `-str reuse,pool`; plain `-str reuse` aligns each string
to 4, moves strings of 8 B or less to `.sdata` and adds a `+0x40`-style displacement when other `.data` precedes the pool. Per object:
`Object(NonMatching, "TRK/dolphin_trk.c", extra_cflags=["-str reuse,pool"])` (measured: `TRK/dolphin_trk` 14 -> 17 of 20 rows at 100,
`.sdata` extra gone; `TRK/support` `TRK_RequestSend` 95.5 -> 96.8; `TRK/msg` and `TRK/msgbuf` unchanged).

**Example.**

```
-str reuse             # default -pool: three strings -> lis r31,...data.0@ha once; 0x44 B
-str reuse -pool off   # three strings -> lis/addi each; 0x40 B; relocs @7 @8 @9
-str reuse,pool        # one @stringBase0 either way (-pool off changes nothing for strings)
```

**Result.** Wii/1.3 `-O4,p -inline auto`, three strings in one function: `-str reuse` 0x44 B, 1 `lis`;
`-str reuse -pool off` 0x40 B, 3 `lis`; `-str reuse,pool` 0x44 B with or without `-pool off`. Three `static const int[8]`
tables: 0x7C B with `-pool`, 0x78 B with `-pool off`; two tables or two `extern` arrays: identical. Same on Wii 1.0, 1.5,
1.7 and GC 2.7. The old attribution of `auto/800CB948_fn_800CB948`'s two-string result to `-pool off` remains
unexplained (its command line was not recorded and two strings cannot show the effect); treat it as a confound.

**Demonstration.** `043-pool-off-string-addressing.cpp` (`ideas.py demo-check 43`) compiles under
`-str reuse,pool -pool off` and asserts both levers: `strings3` keeps `@stringBase0` and one `lis` (`-pool off` cannot
split a merged object), while `tables3` (three `static const` tables) has three `lis` and names `tabA`/`tabB`/`tabC`
by their own relocations, no `...rodata.0`. The demo checker allows one `FLAGS:` line, so the other combinations were
verified by compiling the same file: with `-str reuse` (default `-pool`) `strings3` and `tables3` each take **one**
`lis` off `...data.0` / `...rodata.0`, `.text` 0xCC; with `-str reuse -pool off` each takes three `lis`, `.text` 0xB8.

**Evidence.** `.pi/notes/idea43.md` (the flags x shape matrix); the whole-project audit table in MAIN's
`.pi/notes/pool-audit.md` (landed 58991fa4b, 2026-09-29): Camellia and `network_socket_streams` demonstrated, RSO/runtime
and seven Network units a no-op.
