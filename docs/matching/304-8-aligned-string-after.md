---
id: 304
title: An 8-aligned string after a 4-aligned inline tail is the next TU's first literal; an unreferenced inline is never emitted
status: works
problem: Every `.text` row is at 100 and `.data` still differs - retail has a string right after the vtable and another one 8-aligned after it, while ours emits every string before the vtable; making the owners `inline` moves the strings but never reproduces the 8-byte gap or emits a copy nothing in the TU calls.
tags: [data, sections, source-shape, vtable]
applies: [Wii/1.3]
demo: 
related: [80, 163, 174, 298]
---

# 304. An 8-aligned string after a 4-aligned inline tail is the next TU's first literal; an unreferenced inline is never emitted

**Problem.** `Network/NetworkSessionManager`: 89 rows at 100, `.data` 0x3B8 against a 0x3C0 claim, 225 bytes
differing. Retail's run is `strings, descriptors, string ..0x17A | vtable 0x180..0x348 | getArgument's string
0x348..0x37B | final's string 0x380..0x3BE`; ours puts all four strings ahead of the vtable. The two strings after
the table are an inline tail (idea 80), but spelling `getArgument` and `final` `inline` gives the tail in the other
order, never 8-aligns the second string, and drops `getArgument` altogether because nothing in the TU calls it.

**Why it happens.** Measured on the unit's own command line (Wii/1.3 `-O3 -inline auto -str reuse`, scratch
demos under the lane's `.pi/tmp`):

* an `inline` member nothing references is **never emitted** - plain, `extern inline`, `__declspec(weak) inline`,
  `never_inline`, and every `-inline` mode (`auto`, `deferred`, `all`, `off`, `noauto`, `on`, `level=8`), `-O4,p`
  and `-ipa file` alike. What counts as a reference is decided after the front end folds: a virtual call `this->g()`
  (a destructor's too) marks an inline override used with no relocation; a dead pointer-to-member (`&A::h` in an
  unused local) marks it too; `sizeof(call)`, `0 && call`, `if (0)`, a `const`/enum-false branch and code after
  `return` do not. `__declspec(weak)` on a plain definition keeps the string in the main pool;
* a literal inside an `inline` function is a weak `@STRING@<fn>` object after the vtables whether the function was
  inlined or emitted as a copy; the tail is in **reverse parse order** of the owners (in-class bodies of one class
  come out forward, idea 163's rule otherwise), and the fragments of two different functions are **4-aligned**
  (ours: 0x37B -> 0x37C). An 8-aligned string after the tail is therefore an **object start**: the next TU's first
  main-pool literal (its `.data` section is 8-aligned). The vtable's own alignment is 8 inside one TU, so a 6-byte
  gap before a vtable is not a seam (idea 80's "gap" reading needs this correction);
* `#pragma pool_data on/off` and `pool_strings` change nothing about the tail.

**How to work it.** Read the gaps by kind: a gap before a vtable is its 8-alignment; a 4-aligned string after the
table is the tail; an 8-aligned string after a 4-aligned tail string opens the next TU, and its owner is that TU's
first function (check the extab/extabindex rows: they must be the last of the old claim). Re-draw the seam there
and move the function. For a tail string whose owner nothing in the TU calls, define it `inline` under
`dont_inline` and give it the one-call referencer the link strips (idea 174): `R f(T* p, ...) { return p->m(...); }`
right where retail's copy sits - the copy lands after the referencer, and the link drops the 4-byte `b`.

**Result.** `Network/NetworkSessionManager`: `release` (`NetworkSessionManagerPat::final` by its string) moved to
the front of `Network/NetworkSessionManagerPat.cpp` (`.text`/`.data`/`extab`/`extabindex` seams at 0x803D6F98 /
0x805FAB08 / 0x8001A340 / 0x8003A8A8), `getArgument` an out-of-line inline with `networkRequestArgument` as its
stripped referencer: 88/88 rows at 100, `.data` 0x37B of 0x380 identical, flipcheck READY, trial flip `main.dol:
OK`. The Pat unit's rows are unchanged (103/107, `release` 100) and its data rises 1904/3716 -> 1968/3800.

**Example.**

```
#pragma dont_inline on
inline s32 NetworkRequest::getArgument(u32 idx) { ... log->warn_10("NetworkRequest::getArgument: ...", ...); ... }
s32 networkRequestArgument(NetworkRequest* req, u32 idx) { return req->getArgument(idx); }   /* a `b`; link-stripped */
#pragma dont_inline off
```
