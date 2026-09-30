---
id: 17
title: Cross-family version matrix: a unit's compiler is per unit, not per project
status: works
problem: `mw_version` is set once per library in `configure.py` and every unit inherits the same one (the template default here is `Wii/1.3`). Prebuilt SDK libraries in particular were compiled by Nintendo with whatever compiler the SDK shipped with, so a unit can legitimately come from a *different* toolchain - and then no flag or source change will ever close its residual.
tags: [flags, tooling]
applies: [GC/1.2.5n, GC/3.0a3, Wii/1.3, Wii/1.7]
demo:
---

# 17. Cross-family version matrix: a unit's compiler is per unit, not per project

**Problem.** `mw_version` is set once per library in `configure.py` and every unit inherits the same one
(the template default here is `Wii/1.3`). Prebuilt SDK libraries in particular were compiled by Nintendo
with whatever compiler the SDK shipped with, so a unit can legitimately come from a *different* toolchain -
and then no flag or source change will ever close its residual.

**Why try it.** The target object's `.comment` cannot answer it: the retail DOL has no `.comment` section at
all (`grep -c -a CodeWarrior orig/RMHE08/sys/main.dol` -> `0`), so the comment in a split object is
synthesized from `config.yml`'s `mw_comment_version` and says nothing about the original build (trick 4).
Codegen is the oracle, so compile the unit with every installed compiler *across families* and diff each
result: a single function whose instruction count differs is enough to separate them.

**Result.** For `RSO/runtime` the matrix is flat *within* each family but separates the families on exactly
one function - every other reconstructed function is byte-identical under all of them:

| compiler | `fn_804DA7E4` | `fn_804DA834` | `fn_804DAA24` |
| --- | --- | --- | --- |
| GC 1.0 - 1.2.5n | 69.5 % | 82.2 % | 84.3 % |
| GC 1.3 - 2.7 | 69.75 % | 90.8 % | 93.3 % |
| **GC 3.0a3 / 3.0a5 / 3.0a5.2** | **100 %** | **100 %** | **99.30 %** (460 B / 115 insns = retail) |
| Wii 0x4201_127 - 1.7 | 100 % | 100 % | 97.30 % (116 insns: one extra `lwz`) |

So the unit was built with a GC-era compiler (its lib entry now carries `mw_version: "GC/3.0a3"`); every Wii
compiler emits one instruction more than retail in the `R_PPC_REL24` case. The three 3.0a* builds are
codegen-identical on every reconstructed function, so the choice among them is only tied down by
`config.yml`'s `mw_comment_version` (14 = 3.0a3's comment byte; 3.0a5.x and the Wii compilers emit 15) -
that is a config value, not retail evidence. Older compilers also reject options the newer ones accept
(GC 1.x rejects `-gccinc`), so the driver drops unknown options and retries.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u RSO/runtime GC/1.2.5n GC/3.0a3 Wii/1.3 Wii/1.7
# cross-family specs are "<family>/<version>"; a bare version still means the unit's own family
```
