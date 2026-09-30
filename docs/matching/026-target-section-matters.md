---
id: 26
title: The target's section is part of the match
status: works
problem: A unit can be instruction-identical and relocation-identical and still measure as unmatched, because the code landed in the wrong section (`.text` by default; `.init` for the MSL runtime and boot code).
tags: [sections, measurement]
applies: [Wii/1.3]
demo: 026-target-section-matters.cpp
reviewed: 2026-09-29
related: [23, 55, 74]
---

# 26. The target's section is part of the match

**Problem.** A unit can be instruction-identical and relocation-identical and still measure as *unmatched*, because
the code landed in the wrong section. The compiler emits `.text` by default; the map and the split object may say
something else - `.init` for the MSL runtime and the boot code.

**How it looks.** objdiff pairs sections, so a `.text`-vs-`.init` mismatch reports `fuzzy_match_percent: None`
while the per-function diff shows every row equal. That is the worst kind of false negative: the diff view says
the code is right and the report says nothing matched, which reads like "not decompiled yet".

**Why it happens.** The code is compiled into `.text` unless the source says otherwise, and the target object
`dol split` produced carries the function in the section the original linker put it in.

**How to work it.** Dump both objects' section tables (`python tools/elf/elfsect.py <obj>`, or `mt.py sections -u
<unit>`) and compare names and sizes before reading any instruction. Then force the section on the definition:
`__declspec(section ".init")` puts the function in the target's section and the unit then measures 100 %.
Confirm with `elfsect.py`: the forced section plus `.rela.<section>`, and **no** `.mwcats.<section>` - that one shows
up when the flags you test with omit the cats pragma (`-pragma cats off`), so test with the whole flag list from
`mt.py info`.

**Demonstration.** `026-target-section-matters.cpp` (`ideas.py demo-check 26`): the same 12-byte function compiles
to `.text` when plain and to `.init` under the declspec, with identical bytes and no `.mwcats.init`.

**Example** (measured at the time): `src/Runtime.PPCEABI.H/memset.c` (`memset`, `.init` 0x80004350-0x80004380,
48 B / 12 instructions). Plain C produced `.text` + `.rela.text` where the target object has `.init` +
`.rela.init`; everything else agreed (`memset` at offset 0, one `R_PPC_REL24` to `__fill_mem` at 0x14, both 48 B)
and the report said `None`. With the declspec the unit reads `fuzzy_match_percent 100.0`, `matched_code 48/48`.
The unit still carries the declspec today.

**When NOT to apply.** A whole `.init` fragment claimed by a unit that is more than one function is a splits/linker
matter (ideas 55 and 74), not just a per-function declspec. A function that legitimately belongs in `.text` needs
no section change; check the *target's* section first.
