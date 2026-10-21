---
id: 169
title: nw4r's ref() asserts pass __FUNCTION__, a forward-ordered .sdata static, not a literal
status: works
problem: A unit's `.sdata` holds several identical short strings ("ref") whose bytes match but whose relocations come out in reverse order, so flipcheck reads READY and the flipped DOL hash still moves.
tags: [data, source-shape]
applies: [Wii/1.3]
demo: 
related: [80, 163]
---

# 169. nw4r's ref() asserts pass __FUNCTION__, a forward-ordered .sdata static, not a literal

**Problem.** The `.sdata` of a g3d resource unit holds one short string per `ref()` accessor ("ref"). Written as a
`"ref"` literal the bytes are identical but the copies come out in reverse order, so the relocations that name them
differ and the flipped link does not hash.

**Why it happens.** nw4r's `_ac.h` `ref()` asserts pass `__FUNCTION__`, a function-local static: MWCC emits each to
`.sdata` as `__FUNCTION__$NNN` in forward definition order. A `"ref"` literal inside an inline function is a weak
`@STRING@` object, emitted in reverse (idea 163).

**How to work it.** Pass `__FUNCTION__` in the accessor's assert, as nw4r does. Check the flip by hash, not only by
flipcheck: this difference is in the relocation targets, not the bytes.

**Result.** `g3d/g3d_resvtx` (Wii/1.3): with `"ref"` the trial flip moved the DOL hash (four `.sdata` relocations
swapped); with `__FUNCTION__` it hashed bf4850739478caaedfe675949eb7c28595a7fde9 (35/35 rows, flipcheck READY).

**Example.**

```
/* inside the accessor ref(): __FUNCTION__$NNN in .sdata, forward order; a "ref" literal emits reversed */
if (!IsValid()) nw4r::db::Panic("g3d_resvtx_ac.h", 211, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
```
