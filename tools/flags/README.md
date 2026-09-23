# Compiler-flag tools

Experiments and pre-flights that answer "which flags was this unit built with?" without compiling.
`docs/matching.md` is the playbook; these are its mechanical helpers.

| tool | what it does |
| --- | --- |
| `infer.py` | Reads a **target** object and infers the flags from its bytes, with evidence and confidence. |
| `frame.py` | Stack-frame size of a function, per flag set. |
| `mwcc_matrix.py` | Compiles a unit across the installed `mw_version`s (playbook 17). |
| `optsweep.py` | Sweeps `-opt` sub-options on one unit. |
| `tryvar.py` | Applies a named flag variant to one unit. |
| `variants/<lib>.py` | The named variants `tryvar.py` knows. |

## `infer.py` - the flag inferencer

```sh
python tools/flags/infer.py <unit|object>      # one unit, e.g. Camellia/camellia.c
python tools/flags/infer.py --all              # every unit registered in configure.py
python tools/flags/infer.py --accuracy         # the known-case accuracy table
python tools/flags/infer.py --markdown         # the run + accuracy as markdown
python tools/flags/infer.py --selftest         # deterministic self-test
```

It reads `build/RMHE08/obj/**` (the original object split out of the DOL) - **never** our
`build/RMHE08/src/` build - and decodes the instructions itself. It does not compile, link, re-split or
write to the repository.

### Fingerprints

| flag | evidence in the target | confidence |
| --- | --- | --- |
| `-opt nopeephole` | a record form (`rlwinm.`/`and.`/`add.`/`srwi.`/`extsb.`) proves the peephole is **on**; a `clrlwi` kept before a narrowing store, an unfused `srwi`+`clrlwi`, or a `li r0,N; psq_lx/psq_stx` epilogue proves it is **off** | on: high; off: medium |
| `-fp_contract off` | a fused `fmadds`/`fmsubs` proves **on**; an unfused `fmuls`+`fadds` chain is only a hint (the chain is not proof the source wrote one `a*b+c`) | on: high; off: low |
| `-pool off` | >= 2 object-local data symbols each addressed by its own `lis`+`addi` pair, with no base register shared across symbols | medium |
| `-str ...,readonly` | a NUL-terminated string pool (>= 3 strings) in `.rodata` vs `.data` | low |
| `-use_lmw_stmw off` | no `stmw`/`lmw` and an EABI `_savegpr_*`/`_restgpr_*` call | high |
| `-func_align 4` | a function start off a 16-byte boundary | high |
| `-func_align 16` | every function start 16-byte aligned **and** `gap_*` padding between them | high (low without the padding) |
| `-inline noauto` | a kept `bl` to a tiny (<= 0x40 B) function defined in the same object | medium |
| `-Cpp_exceptions` | `extab`/`extabindex` presence | **not inferable** (the compiler emits them with `off` too) |
| `-O` level | - | **not inferable** |
| `mw_version` | the `.comment` byte | **not inferable** (dtk synthesises `.comment` from `config.yml`; it is uniform) |

Every finding carries its evidence string; a lever the tool cannot see is reported `unknown` rather
than guessed.

### Accuracy

Measured against the campaign's documented cases (`configure.py` cflags groups for the non-`auto`
libs, and the source `#pragma` records from `docs/matching.md` rows 39-40 for the `auto` units):

* 16 confident `peephole` claims, **100 %** correct (the 5 abstentions are off units with no
  fold-shaped sequence; the `auto` units' `cflags_main` is a bulk-attribution guess and is not
  treated as evidence, so only a pragma scores there);
* 13 `func_align`, 4 `inline`, 2 `lmw_stmw`, 1 `pool` - **all 100 %**;
* `fp_contract` makes no confident claim: **no object in this project has a fused multiply-add**,
  so the flag is unobservable either way (the unfused-chain hint is right 7 of 8 times);
* `str_readonly` never fires: no registered unit's object carries its own string pool (the pools are
  separate data splits), so the section is not observable per unit.

The tool's own `--accuracy` prints the per-unit table and names any confident miss; the self-test
asserts there are none.

### Validation of the peephole-off signal

The fold-shaped sequences are not guesses: every object **we** build with the peephole on
(`cflags_main`/`cflags_runtime`, no `nopeephole` flag or pragma) has **zero** of them, while the
targets that need `#pragma peephole off` have them. The `src` side of the diff is the proof; the
target-only inference is the pre-flight.

`infer-run.md` is the committed output of `--markdown` (a per-unit row for all 155 registered units,
the aggregate, and the accuracy table).
