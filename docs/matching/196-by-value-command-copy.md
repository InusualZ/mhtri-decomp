---
id: 196
title: A by-value command copy above or below the command tells whether an inline builder built it
status: works
problem: a record built locally and passed by value to an inline push sits on the wrong side of its stack copy (every lwz/stw of the copy is off by the record size)
tags: [source-shape, allocator]
applies: [WPAD/wpad]
demo: 
---

# 196. A by-value command copy above or below the command tells whether an inline builder built it

**Problem.** A function fills a local record and hands it by value to an inline helper (a queue push); the diff shows
the 13 `lwz`/`stw` pairs of the by-value copy with the record and its copy at each other's stack offsets
(`lwz r31,0x238(r1)` / `stw r31,0x208(r1)` against `lwz r31,0x208(r1)` / `stw r31,0x238(r1)`), and nothing else.

**Why it happens.** MWCC lays the frame out from the inline nesting: a record declared in the function's own body is
placed above the by-value parameter copy the inlined push makes, while a record declared inside an inline builder that
itself calls the push is placed below it. The frame therefore records where the original built the record.

**How to work it.** Read which side of the copy the target's record is on. Below: move the fills into a small inline
builder (`static inline BOOL queueLed(Queue* q, u8 leds, Callback cb) { Cmd cmd; ...; return push(q, cmd); }`) and
call it; above: build it in the caller (or give an out-of-line wrapper its own copy of the body instead of forwarding
to the builder). Then try the builder's store order (the report id first, playbook 138), scored over every caller.

**Result.** `WPAD/wpad`: `WPADControlLed` 99.78 -> 100, `WPADGetInfoAsync` 99.80 -> 100, `WPADSendStreamData`
99.77 -> 100, `wpadSendWriteByte` 99.72 -> 100 (forwarding to the builder), `wpadSendReadMemory` 97.08 -> 97.37 (its
own body), `wpadInitSequence` 99.37 -> 99.77. It is not universal: three one-byte writes of `WPADControlDpd` and two of
`WPADControlSpeaker` still sit the other way, and a wrapper builder there moves the whole frame instead.

**Example.**

```
/* below the copy: built inside the builder */
static inline BOOL wpadQueueLed(WPADCmdQueue* queue, u8 leds, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x11;
    cmd.length = 1;
    cmd.data[0] = (leds << 4) & 0xF0;
    cmd.callback = callback;
    return wpadQueuePush(queue, cmd);
}
```
