---
id: 78
title: A data claim must cover the run the unit actually TOUCHES, not the extent the symbol happens to name
status: works
problem: A `.sbss` claim covered 4 B (the list head) while the unit's own rows relocate words of a 16-byte run that the target holds as one object - so rows the unit owns stayed blocked and the blocked total was mis-recorded.
tags: [data, sections]
applies: [NHTTP]
demo:
reviewed: 2026-09-29
related: [23, 29, 53, 54, 58, 70]
---

# 78. A data claim must cover the run the unit actually TOUCHES, not the extent the symbol happens to name

**Problem.** `NHTTP/d_nhttp`'s `.sbss` claim covered **4 B** (`0x80795880`, the list head) while the unit's own rows
relocate three words of a **16-byte run** at `0x80795878-0x80795888`: two lazy-init flags (`li r0,1` / `stw`,
inside `NHTTPi_RegisterCallbacks` and `fn_8051A8A4`) and the list head, with `NHTTPi_systemInfoP` as the fourth
word - and `powerpc-eabi-objdump -t` shows all four as **one object** in `auto_10_80795878_sbss.o`. So the 4 B claim
left rows this unit owns still unowned, the blocked total was 796 B rather than the 524 B recorded, and the same
pass separately recorded `.bss 0x80762C20` at its *used* extent (0x24) where the map, the split and the target
symbol all say 0x40.

**How it looks.** No instruction diff shows it: the unit's functions read or write a `lbl_`/`extern` symbol whose
address is in *no* registered range, and `datagap.py --unit` or the target object lists the bytes as belonging to
a larger symbol than the one the claim names. Rows that touch the unowned words cannot be written (rule 12 refuses
an `extern` for the range from anywhere, band header included), so they stay blocked.

**Why it happens.** A claim is a statement about **what our object emits**, so its boundaries must match the object
that owns the bytes - the map row and the target symbol - not the part of them this unit currently reads. The
*used* extent is an observation about our progress; the *object* extent is the fact. Getting it wrong has a
specific cost that is easy to underestimate: every row that touches the unowned words stays blocked, and the next
pass re-derives the same claim from scratch.

**How to work it.** (1) List the unit's referrers of the range (`python tools/units/callers.py <addr>` - keyed by
address, works when the asm dump is stale). (2) Take the extent from the **target object's symbol size**
(`powerpc-eabi-objdump -t build/RMHE08/obj/<unit>.o` on the split object) or the map row, never from the offsets
the unit happens to use. (3) Write the `splits.txt` lines in the file's own format and, in the same change, move the
map rows and band-header declarations that fall inside the claim into the unit (rule 7 names, rule 2 inversion).
(4) **Take claims in increments**: the `.sdata`/`.data` half of a run is a separate measured step because a `.data`
claim can drop the target's `R_PPC_NONE` pool relocations (idea 23), and a *partial* `.sdata`/`.sdata2` claim
does not link (idea 29). `datagap.py --unit <unit>` must show no target-extra for the claimed range afterwards.

**When NOT to apply.** Do not widen a claim to bytes another *registered* unit owns (include that owner's header
instead, rule 2), nor to bytes the target object does not carry at all (compiler-synthesised, idea 58). And do not
take a whole neighbouring symbol just because it is adjacent: the rule is the run the unit touches, rounded out to
the object that run belongs to.

**Result.** The amendment to the 16-byte run makes writable the two rows that only need `.sbss` -
`NHTTPi_RegisterCallbacks` (140 B) and `fn_8051A8A4` (132 B) - and, with `.bss 0x80762C20` at its symbol's own
0x40, the three list rows (272 + 32 + 96).

**Evidence.** (Measured at the time; re-checked 2026-09-29: `config/RMHE08/splits.txt` now carries
`NHTTP/d_nhttp.c` `.sbss 0x80795878-0x80795888` and `.bss 0x80762C20-0x80762C60`, i.e. exactly the 16 B and 0x40
extents this idea names.) The amendment was authorised under the claim-amendment protocol. Related: idea 23 (what a `.data` claim can drop),
29 (literal pools), 54 (who owns a table), 70 (unowned data is the unit's to claim).
