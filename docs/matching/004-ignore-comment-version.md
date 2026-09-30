---
id: 4
title: Ignore version/`.comment` hints; make codegen the oracle
status: works
problem: A unit's `.comment` section and `mw_comment_version` in `config.yml` look like a compiler fingerprint and invite "we must be using the wrong compiler version".
tags: [flags, measurement]
applies: []
demo:
reviewed: 2026-09-29
related: [5, 17]
---

# 4. Ignore version/`.comment` hints; make codegen the oracle

**Problem.** A unit's `.comment` section and `mw_comment_version` in `config.yml` look like a compiler
fingerprint and invite "we must be using the wrong compiler version".

**How it looks.** `python tools/elf/elfsect.py` shows the target's `.comment` as `"CodeWarrior" 0e ...` and ours
as `... 0f ...`, and a unit with big size gaps suggests "older compiler".

**Why it happens.** In a decomp-toolkit project the target object's `.comment` is *synthesized* from the
`mw_comment_version` config value when the object is split out, not emitted by the original compiler, so it is
not evidence about the original toolchain. Ours carries the version byte of the compiler we run. (`CLAUDE.md`
Gotchas states the opposite reading for a *real* mismatch; that reading holds only for a `.comment` the original
compiler wrote, which the split objects never carry.)

**How to work it.** Close the question once, per unit, with codegen: compile the unit with every available
compiler and diff each result. If the matrix is flat (identical sizes, match % and first divergence), version is
not the lever and all attention goes to flags and source. A version matrix is also a good smoke test for the
harness: a matrix that is *too* flat may be a stale object (idea 6). The per-library answer differs (idea 17).

**When NOT to apply.** A family difference is real: a unit built by another Metrowerks generation (idea 17) shows
different code shapes, not just a different byte in `.comment`. Decide from the matrix, never from the byte.

**Result.** The Camellia unit's matrix was flat across nine versions, so version was ruled out and the work moved
to flags.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> 0x4201_127 1.0RC1 1.0a 1.0 1.1 1.3 1.5 1.6 1.7
python tools/flags/mwcc_matrix.py --list-versions     # what is installed under build/compilers/Wii
```

**Evidence.** Camellia unit, measured early in the campaign: identical sizes, match % and first divergence for all
nine versions listed above.
