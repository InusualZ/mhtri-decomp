---
id: 26
title: The target's section is part of the match
status: works
problem: a unit can be instruction-identical and relocation-identical and still measure as *unmatched*, because the code landed in the wrong section. The compiler emits `.text` by default; the map and the split object may say something else - `.init` for the MSL runtime and the boot code.
tags: [sections, measurement]
applies: []
demo:
---

# 26. The target's section is part of the match

Problem: a unit can be instruction-identical and relocation-identical and still measure as *unmatched*, because
the code landed in the wrong section. The compiler emits `.text` by default; the map and the split object may
say something else - `.init` for the MSL runtime and the boot code.

Why try it: objdiff pairs sections, so a `.text`-vs-`.init` mismatch reports `fuzzy_match_percent: None`
while the per-function diff shows every row equal. That is the worst kind of false negative: the diff view says
the code is right and the report says nothing matched, which reads like "not decompiled yet".

Result: `__declspec(section ".init")` on the definition puts the function in the target's section, and the
unit then measures 100 %.

Example: `src/Runtime.PPCEABI.H/memset.c` (`memset`, `.init` 0x80004350-0x80004380, 48 B / 12 instructions).
Plain C produced `.text` + `.rela.text` where the target object has `.init` + `.rela.init`; everything else
agreed (`memset` at offset 0, one `R_PPC_REL24` to `__fill_mem` at 0x14, both 48 B) and the report said
`None`. With the declspec the object carries `.init`/`.rela.init` and the unit reads `fuzzy_match_percent
100.0`, `matched_code 48/48`. Confirm with `tools/elf/elfsect.py`: the forced section plus `.rela.<section>`,
and **no** `.mwcats.<section>` - that one shows up when the flags you test with omit the cats pragma, so test
with the whole flag list from `mt.py info`.
