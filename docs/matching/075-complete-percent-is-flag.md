---
id: 75
title: `complete_code_percent` is a FLAG we set, not a measurement
status: works
problem: `tools/project.py` writes `metadata.complete: true` into `objdiff.json` for every `Object(Matching, ...)`, and the report's unit row then reads `complete_code: 404, complete_code_percent: 100.0` **whatever the bytes are**. So "the report says 100 %" is a restatement of a flag we set ourselves, not evidence that anything matches - which matters because a flip is exactly the moment the claim becomes load bearing, and because the ledger's progress totals inherit it.
tags: [measurement, tooling]
applies: []
demo:
---

# 75. `complete_code_percent` is a FLAG we set, not a measurement

**Problem.** `tools/project.py` writes `metadata.complete: true` into `objdiff.json` for every
`Object(Matching, ...)`, and the report's unit row then reads `complete_code: 404, complete_code_percent: 100.0`
**whatever the bytes are**. So "the report says 100 %" is a restatement of a flag we set ourselves, not
evidence that anything matches - which matters because a flip is exactly the moment the claim becomes load
bearing, and because the ledger's progress totals inherit it.

**What is actually suppressed.** Not the diff. Measured on a `Matching` unit with one byte of its banner
changed: `fuzzy_match_percent` moved **100.0 -> 99.95049** while `complete_code_percent` stayed **100.0**, and
`ninja build/RMHE08/ok` FAILED with the DOL sha1 moving `bf4850739478caaedfe675949eb7c28595a7fde9 ->
abc3729830d69a412d6a5a112e735d628dbc3f31`. objdiff still diffs the unit and still reports the difference;
what the flag suppresses is the unit's contribution to the *completion* totals. This is the long-documented
trap (`complete_code_percent: 100.0` beside `fuzzy_match_percent: 1.77`) with its mechanism attached.

**Result.** The rule: **never cite `complete_code_percent`, the `complete` flag, or "the report says 100 %" as
evidence for a `Matching` unit.** For a row, cite `fuzzy_match_percent`; for the *unit*, cite the byte-level
comparison and `ninja build/RMHE08/ok`'s DOL hash - the two things a flipped unit cannot fake.
`tools/units/verifyunit.py` now performs that byte comparison as a landing-gate row, address-aware so a
dtk-`pad_`-named row still resolves, and it still refuses a corrupted byte. `flipcheck.py` READY remains
necessary and not sufficient.
