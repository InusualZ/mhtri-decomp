---
id: 17
title: Cross-family version matrix: a unit's compiler is per unit, not per project
status: works
problem: `mw_version` is set once per library in `configure.py` and every unit inherits it, but prebuilt SDK code may come from a different compiler family, and then no flag or source change closes its residual.
tags: [flags, tooling]
applies: [GC/1.2.5n, GC/3.0a3, Wii/1.3, Wii/1.7]
demo:
reviewed: 2026-09-29
related: [4, 13, 25, 81]
---

# 17. Cross-family version matrix: a unit's compiler is per unit, not per project

**Problem.** `mw_version` is set once per library in `configure.py` and every unit inherits the same one (the
template default here is `Wii/1.3`). Prebuilt SDK libraries in particular were compiled by Nintendo with
whatever compiler the SDK shipped with, so a unit can legitimately come from a *different* toolchain - and then
no flag or source change will ever close its residual.

**How it looks.** A unit whose functions are otherwise right keeps the same one or two functions short by an
instruction (one extra `lwz` in one relocation case), whatever flags are tried, while every other function is
byte-identical.

**Why it works.** The target object's `.comment` cannot answer the question: the retail DOL has no `.comment`
section at all (`grep -c -a CodeWarrior orig/RMHE08/sys/main.dol` -> `0`), so the comment in a split object is
synthesized from `config.yml`'s `mw_comment_version` and says nothing about the original build (idea 4).
Codegen is the oracle: compile the unit with every installed compiler *across families* and diff each result;
a single function whose instruction count differs is enough to separate them.

**How to work it.**

```sh
python tools/flags/mwcc_matrix.py -u <unit> GC/1.2.5n GC/3.0a3 Wii/1.3 Wii/1.7
# cross-family specs are "<family>/<version>"; a bare version still means the unit's own family
python tools/flags/mwcc_matrix.py --list-versions -u <unit>     # lists the files of the unit's compiler directory (-u is required)
ls build/compilers/GC build/compilers/Wii                      # the installed versions
```

Read the per-function scores, not the unit total. Compilers inside one family are usually identical on small
functions, so look for the function that separates *families*. Older compilers also reject options the newer
ones accept (GC 1.x rejects `-gccinc`), so the driver drops unknown options and retries. A `mw_version` change
is non-negotiable #3 territory: it needs this kind of evidence, called out in `configure.py`.

**Result** (`RSO/runtime`, measured at the time). The matrix is flat within each family and separates the
families on exactly one function:

| compiler | `fn_804DA7E4` | `fn_804DA834` | `fn_804DAA24` |
| --- | --- | --- | --- |
| GC 1.0 - 1.2.5n | 69.5 % | 82.2 % | 84.3 % |
| GC 1.3 - 2.7 | 69.75 % | 90.8 % | 93.3 % |
| **GC 3.0a3 / 3.0a5 / 3.0a5.2** | **100 %** | **100 %** | **99.30 %** (460 B / 115 insns = retail) |
| Wii 0x4201_127 - 1.7 | 100 % | 100 % | 97.30 % (116 insns: one extra `lwz`) |

The unit was built with a GC-era compiler; its lib entry in `configure.py` carries `mw_version: "GC/3.0a3"`
(and the reasoning in a comment). Every Wii compiler emits one instruction more than retail in the
`R_PPC_REL24` case. The three 3.0a* builds are codegen-identical on every reconstructed function, so the choice
among them rests only on `config.yml`'s `mw_comment_version` (14 = 3.0a3's comment byte; 3.0a5.x and the Wii
compilers emit 15) - a config value, not retail evidence.

**When NOT to apply.** Do not run the matrix as a first resort: the unit's flags (ideas 5, 27, 28) and source
shape (18, 19) are far more common causes, and a matrix over a unit that has never matched anywhere is
noise. It answers "is the compiler family wrong", which needs one function that is otherwise fully reconstructed
and stays off by an instruction across every flag (idea 13).

**Evidence.** Numbers above: `RSO/runtime`, the numbers predate this review; the unit is `NonMatching` in the current tree.
Idea 81 (`ruled-out`) is the general form of the suspicion ("a different compiler release"): run the matrix once, and if it is flat the version is not the lever. This idea is the one case where it is.
