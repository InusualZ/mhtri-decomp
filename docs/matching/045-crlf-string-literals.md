---
id: 45
title: A hand-written string literal's `\n` becomes CRLF on this host
status: works
problem: A data range's string pool cannot be written in-source because MWCC on this host translates the `\n` in the literals to CRLF, so the emitted bytes do not match the DOL's LF. It reads as a data-claim problem and invites a hand-written definition that will not match.
tags: [data, tooling]
applies: []
demo: 045-crlf-string-literals.cpp
---

# 45. A hand-written string literal's `\n` becomes CRLF on this host

**Problem.** A data range's string pool cannot be written in-source because MWCC on this host translates the `\n` in the literals to CRLF, so the emitted bytes do not match the DOL's LF. It reads as a data-claim problem and invites a hand-written definition that will not match.

**Why try it.** Record it before spending a pass on the pool: the bytes come from the compiler's literal path, which is host-newline sensitive. Leave the range to the data pass, or reference the strings as `extern` declarations so the object does not emit them.

**Result.** Measured independently by 1 worker(s):

* `800cc5b0-fn-800cc5b0-39c9.md` - possible but MWCC on this host turns the `\n` in the literals into CRLF, so the bytes do not match the DOL's LF - left as a follow-up for the data pass.

**Demonstration.** `045-crlf-string-literals.cpp` (`ideas.py demo-check 45`) did **not** reproduce the claim: an LF
source file's `"a\nb\n"` is emitted as `61 0a 62 0a 00` (no `0d 0a`) through `sjiswrap` and Wii/1.3 on this host. If
the CRLF ever appears it comes from something other than the compiler's literal path (for example a CRLF source file);
the demo pins the LF behaviour so a regression is visible.
