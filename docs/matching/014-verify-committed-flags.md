---
id: 14
title: Prove the committed flag list reproduces the proven object
status: works
problem: The flags eventually get written into `configure.py` as a per-library override. A hand-written list can silently differ from the command line that was tested - a leftover `-O4,p`, a duplicated `-inline auto`, a flag appended after a conflicting default.
tags: [flags, measurement]
applies: []
demo:
reviewed: 2026-09-29
related: [5, 6]
---

# 14. Prove the committed flag list reproduces the proven object

**Problem.** The flags eventually get written into `configure.py` as a per-library override. A hand-written list
can silently differ from the command line that was tested - a leftover `-O4,p`, a duplicated `-inline auto`, a
flag appended after a conflicting default.

**How it looks.** The scripted experiment gave a byte-identical unit, and the real `ninja` build of the same unit
does not (different hash, different sizes).

**Why it happens.** What matters is that the *repository* builds the proven object, not that some script did. The
scripted path replaces same-family flags; a hand-copied list in `configure.py` may keep the base's `-O4,p` and
add `-O3` after it (the compiler takes the last, but other flags interact), or lose a flag the script inherited.

**How to work it.** Compile the unit once through the scripted override path and once through `ninja`, and
compare the object hashes. The override should *remove* the conflicting defaults rather than append after them,
so the command line contains exactly one `-O` / `-inline` / etc. Delete the object first so ninja really rebuilds
it (idea 6). Then land the per-library `cflags_*` with the evidence next to it (`CLAUDE.md`, matching policy).

**When NOT to apply.** A per-object override on a `Matching` unit needs the DOL hash too (`ninja build/RMHE08/ok`),
since the object hash alone does not show a section or relocation change elsewhere.

**Result.** The committed configuration is the proven one.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "<final flags>" 1.3
sha1sum build/RMHE08/src/<Unit>/<file>.o
rm -f build/RMHE08/src/<Unit>/<file>.o && ninja build/RMHE08/src/<Unit>/<file>.o
sha1sum build/RMHE08/src/<Unit>/<file>.o        # must be the same hash
```
