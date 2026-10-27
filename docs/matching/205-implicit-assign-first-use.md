---
id: 205
title: The first implicit struct assignment calls __as__ out of line: instantiate it earlier in a never-called inline
status: works
problem: the first function that assigns a small struct (`dst->opts = src->opts`) calls an emitted `__as__<len><Type>FRC<len><Type>` where retail copies it inline with `lwz`/`stw`, while a later function's identical copy is inline
tags: [source-shape]
applies: [Wii/1.3]
demo: 
---

# 205. The first implicit struct assignment calls __as__ out of line: instantiate it earlier in a never-called inline

**Problem.** A C++ unit assigns a small struct member (`dst->opts_0x02 = src->opts_0x02`, a 4-byte `{ u16 ids[2]; }`,
or a 0x17-byte name block). The first function in the file that does it calls `__as__9LbSetOptsFRC9LbSetOpts`
(and the object grows a weak `__as__` for every such type), so that row scores ~30 %; a sibling compiled later with the
same assignment copies it inline and matches. Retail inlines both.

**Why it happens.** MWCC generates a class's implicit `operator=` the first time an assignment needs it. The function
that triggers the generation is already being compiled, so the call cannot be inlined there and the operator is
emitted out of line; every later use finds it defined and inlines it (under `dont_inline off`, as the copies need).

**How to work it.** Put a never-called `inline` function ahead of the first real copy that assigns each struct type
once, so the implicit operators are generated there. An uncalled `inline` emits no code, and the real copies inline.
Check the object afterwards: no `__as__` symbol and no `bl __as__` left (`objdump -t`/`-r`).

**Result.** `lobby/lb_pane_ui` (cflags_lobby, Wii/1.3, `-O3`): `lb_my_set_copy` 29.42 -> 100 %, both emitted
`__as__9LbSetOpts`/`__as__9LbSetName` gone; `lb_gun_set_copy` unchanged at 100 %.

**Example.**

```cpp
#pragma push
#pragma dont_inline off
inline void lb_set_parts_copy(LbSetOpts* d, const LbSetOpts* s, LbSetName* dn, const LbSetName* sn)
{
    *d = *s;            /* generates __as__9LbSetOpts here, never called */
    *dn = *sn;
}
extern "C" void lb_my_set_copy(LbMySet* dst, LbMySet* src)
{
    dst->weapon_0x00 = src->weapon_0x00;
    dst->opts_0x02 = src->opts_0x02;       /* now inline: lwz/stw */
    ...
}
#pragma pop
```
