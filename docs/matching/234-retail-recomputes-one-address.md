---
id: 234
title: Retail recomputes one address at two call sites: spell the two uses differently (union member, element address) to defeat the CSE
status: works
problem: Ours keeps an address used by two calls in a callee-saved register (`addi r27,r28,0x35` + `mr r4,r27` / `mr r6,r27`) where retail recomputes it as each call's argument (`addi r4,r28,0x35` ... `addi r6,r28,0x35`).
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
---

# 234. Retail recomputes one address at two call sites: spell the two uses differently (union member, element address) to defeat the CSE

**Problem.** The same field address passed to two calls (`postMediatorRecord(.., rec->text)` then
`postChatLogLine(.., rec->text, ..)`) is common-subexpression-eliminated into a callee-saved register, which adds an
`addi` + `mr` pair, can grow the frame, and shifts every register after it. Retail recomputes the address at each
call.

**Why it happens.** MWCC's CSE merges identical expression trees; two spellings of the same address that lower
to different trees (a different union member, `&arr[i]` against `arr[i].first_field.text`) are not merged.

**How to work it.** Spell one of the uses through a different but equivalent expression: the other member of a union
over the same bytes (`chars_0x035` beside `text_0x035`), or the element's address (`(const char*)&peers[peer]`)
for its first member's text. Try each site with `tryvar`: which use takes the other spelling matters (in one row the
`strcmp` had to keep the field spelling and the later chat-log call take the element address). When every use in a
function wants recomputing, `#pragma opt_common_subs off` around that one function is the blunt form (idea 208).

**Result.** `Network/net_session_close`: `sessionReflectCallback` 98.59 -> 99.41 % (+34.6 B, two sites);
`communityReflectCallback` reached 100 % with `opt_common_subs off` (its 0x10 B frame then came from a 0x10 B id
buffer).

**Example.**

```
postMediatorRecord(getInstance(), rec->text_0x035);               /* addi r4,r28,0x35 */
postChatLogLine(0, member, rec->color_0x238, rec->chars_0x035, ..); /* addi r6,r28,0x35 again */
```
