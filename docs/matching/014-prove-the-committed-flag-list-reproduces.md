---
id: 14
title: Prove the committed flag list reproduces the proven object
status: works
problem: The flags eventually get written into `configure.py` as a per-library override. A hand-written list can silently differ from the command line that was tested - a leftover `-O4,p`, a duplicated `-inline auto`, a flag appended after a conflicting default.
tags: [flags]
applies: []
demo:
---

# 14. Prove the committed flag list reproduces the proven object

**Problem.** The flags eventually get written into `configure.py` as a per-library override. A
hand-written list can silently differ from the command line that was tested - a leftover `-O4,p`, a
duplicated `-inline auto`, a flag appended after a conflicting default.

**Why try it.** What matters is that the *repository* builds the proven object, not that some script did.

**Result.** Compile the unit once through the scripted override path and once through `ninja`, and compare
the object hashes. The override should *remove* the conflicting defaults rather than append after them, so
the command line contains exactly one `-O` / `-inline` / etc.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "<final flags>" 1.3
sha1sum build/RMHE08/src/<Unit>/<file>.o
rm -f build/RMHE08/src/<Unit>/<file>.o && ninja build/RMHE08/src/<Unit>/<file>.o
sha1sum build/RMHE08/src/<Unit>/<file>.o        # must be the same hash
```
