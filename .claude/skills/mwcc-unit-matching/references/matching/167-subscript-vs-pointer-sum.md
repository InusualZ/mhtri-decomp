---
id: 167
title: A subscript and a pointer sum give opposite add operand orders
status: works
problem: A computed address differs only in the operand order of one `add` - retail `add rD,rOffset,rBase`, ours `add rD,rBase,rOffset` - and the row sits at 97-99 %.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [18, 135]
---

# 167. A subscript and a pointer sum give opposite add operand orders

**Problem.** A header-relative address (`base + header->offset + 8`) compiles to the same instructions as retail's
except that one `add` has its operands swapped.

**Why it happens.** MWCC keeps the source's operand order in the `add`. A subscript `&p[i]` puts the index first
(`add rD, rI, rP`); a pointer sum `p + i` puts the base first (`add rD, rP, rI`).

**How to work it.** Spell the address as a subscript on a byte pointer when retail's `add` has the offset first, and
as a pointer sum when it has the base first.

**Result.** `ef/ef_emitter` and its neighbours (Wii/1.3): `ef_res_block_body`, `ef_emres_get_ptcl_track`,
`ef_emres_num_emit_track`, `ef_emres_get_emit_track_tbl` and fn_800A8CE8 from 97.5-99.2 to 100 each with
`&((u8*)h)[h->offset + 8]`.

**Example.**

```
return &((u8*)h)[h->offset + 8];   /* add rD, rOffset, rBase - retail */
return (u8*)h + h->offset + 8;     /* add rD, rBase, rOffset - ours */
```
