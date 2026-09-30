---
id: 45
title: A hand-written string literal's `\n` becomes CRLF on this host
status: ruled-out
problem: A lane reported that MWCC on this host turns the `\n` in a string literal into CRLF so a hand-written string pool could not match the DOL's LF. Not reproduced: `\n` is 0x0A under every flag set and source encoding tried.
tags: [data, tooling]
applies: [Wii/1.3]
demo: 045-crlf-string-literals.cpp
reviewed: 2026-09-29
related: [23, 43, 44]
---

# 45. A hand-written string literal's `\n` becomes CRLF on this host

**Problem.** A data range's string pool cannot be written in-source, one lane reported, because MWCC on this host
translates the `\n` in the literals to CRLF (0x0D 0x0A), so the emitted bytes would not match the DOL's LF (0x0A).
It reads as a data-claim problem and invites leaving the range as `extern` declarations. **Status: ruled out** -
the claim does not reproduce (2026-09-29), so a literal reconstruction should not be skipped on this account.

**How it looks (if it were true).** The section's contents differ from the target's by inserted `0d` bytes before
each `0a`, so every string after the first `\n` shifts and the section size is larger than the target's.

**Why it was believed.** The only evidence is one line of an outbox (`800cc5b0-fn-800cc5b0-39c9`): "possible but MWCC
on this host turns the `\n` in the literals into CRLF ... left as a follow-up for the data pass". No measurement
or emitted bytes were recorded with it.

**What was measured (2026-09-29).** A source with `"a\nb\n"`, `"line1\r\nline2"` and a backslash-newline
continuation, compiled through `sjiswrap` + `mwcceppc` Wii/1.3, in both an **LF** and a **CRLF** source file, under
the base flags, `-str reuse,pool,readonly -pool off` and `-encoding ascii -multibyte`:

| source spelling | emitted bytes |
| --- | --- |
| `"a\nb\n"` (all 6 combinations) | `61 0a 62 0a 00` |
| `"line1\r\nline2"` | `... 0d 0a ...` (only because `\r` was written) |
| `"multi\` + newline + `line"` | `multiline` (no newline byte, LF or CRLF file) |

So the escape is never translated, and a CRLF source file does not change it either (the checked file's line
endings do not reach the literal). If retail bytes contain `0d 0a`, the original source wrote `\r\n`.

**How to work it.** Do not skip a string range for this reason. If a hand-written pool shows extra `0d` bytes, look
for a literal `\r`, a wrong string (encoding: the build passes `-multibyte`, deprecated in favour of `-encoding`,
so non-ASCII text goes through sjiswrap and can change bytes) or a claim that starts at the wrong address.

**When NOT to apply.** This entry only says the CRLF *translation* is not a compiler behaviour. String data can
still fail to reproduce for real reasons (section choice, pooling: ideas 43/44; who owns the range: idea 23).

**Demonstration.** `045-crlf-string-literals.cpp` (`ideas.py demo-check 45`) pins the measured behaviour: `"a\nb\n"`
is `61 0a 62 0a 00` with no `0d 0a`, and an explicit `"x\r\ny"` does emit `0d 0a`, so the checker can see a CR when
one is present. If a future compiler/wrapper *did* start translating, this demo would go red.

**Open question.** The lane's original file is gone, so what it really saw is unknown; measuring the retail
`.data` at `0x80594DE0..0x80594E89` against a literal reconstruction would settle it for that range.
