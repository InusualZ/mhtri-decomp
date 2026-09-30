---
id: 4
title: Ignore version/`.comment` hints; make codegen the oracle
status: works
problem: A unit's `.comment` section and `mw_comment_version` in `config.yml` look like a compiler fingerprint and invite "we must be using the wrong compiler version".
tags: [flags, measurement]
applies: []
demo:
---

# 4. Ignore version/`.comment` hints; make codegen the oracle

**Problem.** A unit's `.comment` section and `mw_comment_version` in `config.yml` look like a compiler
fingerprint and invite "we must be using the wrong compiler version".

**Why try it.** In a decomp-toolkit project the target object's `.comment` is *synthesized* from that
config value, not emitted by the original compiler, so it is not evidence about the original toolchain at
all.

**Result.** The cheapest way to close the question is to compile the unit with every available compiler and
diff each result. If the matrix is flat, version is not the lever and all attention can go to flags and
source. (A version matrix is also a good smoke test for the harness - see trick 6.)

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> 0x4201_127 1.0RC1 1.0a 1.0 1.1 1.3 1.5 1.6 1.7
# flat for the Camellia unit: identical sizes, match % and first divergence for all nine
```
