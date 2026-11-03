---
id: 297
title: A local taken through a cast to another pointer type is kept as its own node and colours above the parameter
status: works
problem: Two pointer parameters both live across a call come out swapped (retail keeps the first parameter in r31, the second in r30; ours the reverse) in a function whose every other instruction matches, and no declaration order, `register`, plain local copy of either parameter, static helper or scoped pragma moves them.
tags: [source-shape, allocator]
applies: [GC/3.0a5.2, Wii/1.3]
demo: 
related: [243, 244, 283]
---

# 297. A local taken through a cast to another pointer type is kept as its own node and colours above the parameter

**Problem.** `btsnd_hcic_change_name(BT_HDR* p, u8* p_name)` matches every opcode but allocates `p` to r30 and
`p_name` to r31 where retail has `p` in r31. A plain copy (`BT_HDR* p = p_buf;`, with or without a kill of the
parameter after it, K&R declaration order, an identity static helper, `register`) changes nothing: the dump's node
table shows the local folded back into the parameter (`r32 -> p_buf`), so the parameter order decides.

**Why it happens.** Parameters take the lowest virtual registers in order (idea 243), so when both stall out of
the first simplify scan (degree >= 29) they are pushed in ascending number in the next scan and the *second*
parameter is coloured first - r31. The replay shows no forced spill pick at all (`forced=False`), so reference cost
is irrelevant: only the number order can move the pair, and `regsim search` cannot renumber a parameter. The
frontend propagates a plain copy of a parameter into the parameter's own node, but it keeps a local whose
initialiser goes through a cast to a *different* pointer type (`(BT_HDR*)((u8*)p_buf)`, a `BT_BUF*` view and
`&buf->hdr`, `(BT_HDR*)((u32)p_buf ^ 0)`, a `void*` round trip) as its own node, numbered above the parameters like
any named local; it stalls like the parameter it copies and is pushed after `p_name`, so it is coloured first.
`*&p_buf`, `p_buf ? p_buf : p_buf`, `(0, p_buf)` and `&p_buf[0]` are folded and do not help.

**How to work it.** Give the first parameter a view type (here the buffer's `BT_BUF*` whose `hdr` is the header and
whose `data` is the payload) and take the header pointer from it: the parameter dies at the view, the local carries
every use, the emitted code is byte-identical. Check it in the dump: the local must appear as its own node above the
parameters in `regalloc-gpr-pass-1-all.txt` (a folded copy shows the parameter's name on the node instead).

**Result.** `BTE/hcicmds` (GC/3.0a5.2, `cflags_nw4r`): `btsnd_hcic_change_name` 99.1875 -> 100 %, unit 58/59 ->
59/59 rows (99.97403 -> 100.0); flipcheck READY, trial flip links to the retail DOL
(`bf4850739478caaedfe675949eb7c28595a7fde9`). 30 other shapes (helpers, copies, K&R order, loop and header
spellings, 18 scoped pragmas) left the swap, measured in the same lab.

**Example.**

```
void btsnd_hcic_change_name(BT_HDR* p_buf, u8* p_name) {
    BT_BUF* buf = (BT_BUF*)p_buf;   /* the view: p below is a new node, numbered above p_name */
    BT_HDR* p = &buf->hdr;          /* was the parameter itself: p_name (the higher number) took r31 */
    u8* pp = buf->data;
    u16 len = (u16)(strlen((char*)p_name) + 1);
    ...
```
