---
id: 75
title: `complete_code_percent` is a FLAG we set, not a measurement
status: works
problem: `tools/project.py` writes `metadata.complete: true` for every `Object(Matching, ...)`, so the report's unit row reads `complete_code_percent: 100.0` whatever the bytes are - a restatement of a flag we set, not evidence that anything matches.
tags: [measurement, tooling]
applies: []
demo:
reviewed: 2026-09-29
related: [1, 15]
---

# 75. `complete_code_percent` is a FLAG we set, not a measurement

**Problem.** `tools/project.py` writes `metadata.complete: true` into `objdiff.json` for every
`Object(Matching, ...)`, and the report's unit row then reads `complete_code: 404, complete_code_percent: 100.0`
**whatever the bytes are**. So "the report says 100 %" is a restatement of a flag we set ourselves, not
evidence that anything matches - which matters because a flip is exactly the moment the claim becomes load
bearing, and because the ledger's progress totals inherit it.

**How it looks.** A unit reports 100 % complete beside a lower `fuzzy_match_percent` (the long-documented
`complete_code_percent: 100.0` next to `fuzzy_match_percent: 1.77`), or a corrupted `Matching` unit still
reads complete.

**Why it happens / what is actually suppressed.** Not the diff. Measured (2026-09) on a `Matching` unit with
one byte of its banner changed: `fuzzy_match_percent` moved **100.0 -> 99.95049** while
`complete_code_percent` stayed **100.0**, and `ninja build/RMHE08/ok` FAILED with the DOL sha1 moving
`bf4850739478caaedfe675949eb7c28595a7fde9 -> abc3729830d69a412d6a5a112e735d628dbc3f31`. objdiff still diffs the
unit and reports the difference; what the flag suppresses is the unit's contribution to the *completion*
totals (objdiff-cli 3.6.1 pins the percent at 100 for a `complete` unit).

**How to work it.** **Never cite `complete_code_percent`, the `complete` flag, or "the report says 100 %"
as evidence for a `Matching` unit.** For a row, cite `fuzzy_match_percent`; for the *unit*, cite the
byte-level comparison and `ninja build/RMHE08/ok`'s DOL hash - the two things a flipped unit cannot fake.
`tools/units/verifyunit.py` performs that byte comparison as a landing-gate row, address-aware so a
dtk-`pad_`-named row still resolves (idea 74), and it still refuses a corrupted byte. `flipcheck.py` READY
remains necessary and not sufficient.

**When NOT to apply.** For a `NonMatching` unit the flag is not set and the percentages are ordinary
measurements (still subject to idea 15's pinned metric). The rule is about the *completeness* claim only.

**Result.** The rule itself; the corrupted-byte measurement above is the evidence. It is a measurement/process
idea, so it has no compiler demo.
