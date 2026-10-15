---
id: 145
title: A two-exit loop without goto: retail's own inline helpers, or the tail moved into the loop
status: works
problem: Retail's control flow was written with `goto` (a jump out of a nested loop, a shared round-up or cleanup tail), rule 8 forbids it, and the goto-free spellings duplicate the tail or add a compare.
tags: [source-shape]
applies: [Wii/1.3, GC/3.0a5.2]
demo: 
related: [101, 126, 143]
---

# 145. A two-exit loop without goto: retail's own inline helpers, or the tail moved into the loop

**Problem.** A function whose retail layout jumps from inside a loop (or from several places) to one shared block
cannot be written with `goto` (section 6.5 rule 8). The obvious replacements - a flag, a `break` plus a test after
the loop, an invented helper - either duplicate the shared block, add a compare (`q < end`) or lose the
`mtctr`/`bdnz` loop.

**Why it happens.** MWCC lays out each source exit as its own path; a `goto` names one block that several exits
share. Three goto-free forms produce the same sharing: an **inline helper with early returns** (its exits join
at the helper's return, idea 101), a loop whose tail is **moved inside** it (the shared block runs on the path that
used to jump, then `break`s), and a `do { ... break; ... } while (0)` whose `break`s reach one cleanup tail.

**How to work it.** In order:

1. If the vendor's code had a helper there (idea 143), write it as a `static inline` with early returns.
2. For a jump out of a nested loop to the end of the function: put the tail in the loop, on the path that used to
   jump (`else continue` for the other path, then the tail, then `break`).
3. For a shared cleanup: `do { ... if (err) break; ... } while (0); cleanup;`.

A helper **invented** to stand for a goto does not work: it adds its own argument copies (idea 126) and moves the
colouring. When none of the three reaches the goto score, record both measurements as the residual (rule 8).

**Result.** Wii/1.3: `MSL_C/ansi_fp` `__num2dec` 92.05 -> 98.67 through MSL's `__rounddec`/`__must_round`/
`__dorounddecup` (100 with idea 124); `__dec2num` reached the goto spelling's score exactly with the bracket tail
moved into the loop. Ruled out: invented helpers for `__timesdec` 98.67 -> 97.69 and `__str2dec` 89.15 -> 76.78.
The remaining walls, each measured both ways: `__str2dec`/`__timesdec`/`__minus_dec` 100 with goto (the unit 10/10)
vs 89.15/98.67/99.26 without; `MSL_C/printf` `round_decimal` 99.73 vs 95.61, `__pformatter` 97.32 vs 96.63;
`USB/usb` `usbIsoPacketTotal` 89.57 (a flag and an inline BOOL helper both keep a test). The `do { } while (0)`
form is the shared exit of `USB/usb` and of `NHTTP/d_nhttp`'s `NHTTPi_createRequestObject` (GC/3.0a5.2; unit
headers).

**Example.**

```c
for (;;) {                       /* was: goto done from the inner loop */
    ...
    if (!converged) {
        continue;
    }
    result = finish(x);          /* the tail, moved in */
    break;
}
return result;
```
