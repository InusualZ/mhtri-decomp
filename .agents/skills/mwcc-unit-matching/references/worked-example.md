<!-- GENERATED FILE - do not edit.
     Source: docs/matching.md
     Regenerate: python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py
-->

## Worked example: the `RSO/runtime` unit (DOL-side RSO loader/linker)

Nine contiguous functions at `0x804D9B4C..0x804DAE40` (4852 B), unsplit when it was picked up, with two
SDK names and seven `fn_XXXX`. Everything below is in the repository now (`src/RSO/runtime.c`,
`cflags_rso` + `mw_version` in `configure.py`, the `.text` and jump-table ranges in `splits.txt`).

**Finding out what the unit is** (idea 25): one `mcpScript` fanning the nine addresses through the shared
dump returned `LocateObject`, `RSOStaticLocateObject`, `RSOUnLocateObject`, `RSOLink`, `RSOUnLink`,
`FindExportIndex`, `RSORelocate` and `RSORelocateSmallDataSection` - eight of the nine (`fn_804DA7E4` is
a `zz_` placeholder there too) - plus the four 4-byte `RSONotify*` thunks that sit immediately *before*
the range, typed signatures whose body lengths match this repo's map sizes exactly, and an `RSOModule`
layout that matched every offset derived from the disassembly.

**Finding out how it was built** (ideas 17, 21): the record-form count (9 in this target, 0 in
`Camellia`'s) said peephole + scheduling were on, so `-opt nopeephole` - the project's `Camellia` setting -
was wrong here; a cross-family version matrix then showed every Wii compiler emitting one instruction more
than retail in `RSORelocate`'s `R_PPC_REL24` case (116 vs 115) while GC 3.0a3/3.0a5/3.0a5.2 reproduced the
retail opcode sequence, so the lib entry carries `mw_version: "GC/3.0a3"`.

**Reconstruction** - the last percent of each function came from one of the levers:

| function | lever that closed it | before -> after |
| --- | --- | --- |
| `fn_804DA7E4` | signed `int count` + named `pEntry` (idea 18), `while (count--)` (idea 19) | 61.45 -> **100 %** |
| `fn_804DA834` | named temps at the `strcmp` sites, named pointer before named offset, `hash == p->hash` (idea 18) | 81.56 -> **100 %** |
| `RSOUnLocateObject` | typed array walk (`((RSORelocation*)p->table)[i]`) + hoisted `pEntry` | 74.92 -> **100 %** |
| `RSOLink` | value merge instead of early return, `offset` as a second induction variable, declaration order (ideas 18, 19) | 59.12 -> **100 %** |
| `fn_804DA6C8` | loop-invariant address through a `u32 buf[1]` (idea 20) | 94.77 -> **100 %** |
| `fn_804DAA24` | comparison operand order (idea 18) - 31 further shapes made no difference | 97.30 -> 99.30 % |
| `fn_804D9B4C` | two named temporaries - then ~300 variants, 31 compilers, 6 `-opt` sets all identical (idea 22) | 0 -> 99.39 % |
| `RSOStaticLocateObject` | two-variable `for` + local `char* msg` (ideas 18, 19); jump table claimed in `splits.txt` (idea 23) | 74.92 -> 99.28 % |
| `fn_804DABF0` | split `u32 e0 = ...; u32 name = e0 + ...;` (idea 18) | 0 -> 99.36 % |

Unit: `fuzzy_match_percent` 5.47 -> 99.71, `matched_code` 2132/4852, **5 of 9 functions byte-identical**,
and every function's instruction count equal to retail. The four residuals are allocator colour (ideas 22
and 13) and are recorded on the functions themselves; the string-pool `splits.txt` claim that looked like
the last fix was measured, found harmful (-1.35 %) and reverted (idea 23).

## Worked example: the `Camellia` unit

Starting point: the project defaults (`-O4,p -inline auto -use_lmw_stmw on -str reuse,pool,readonly`),
`.text` 21600 B against the target's 24420 B, unit at 1.49 % fuzzy, **6 of 10 functions at 0 %**.

Final flag set (now `cflags_camellia` in `configure.py`):

```
-O3  -inline noauto  -opt nopeephole  -pool off  -use_lmw_stmw off
```

which gives **9 of 10 functions byte-identical** and the target's exact `.text` size. Each flag removed one
specific symptom:

| flag | symptom it removes | symptom seen in the target |
| --- | --- | --- |
| `-O3` (not `-O4,p`) | fused index math, hoisted table bases | `extrwi`+`slwi` instead of one fused `clrlslwi` |
| `-inline noauto` | `camellia_setup192` inlined into `Camellia_Ekeygen` | `b camellia_setup192` tail call, 68 B dispatcher |
| `-opt nopeephole` | `srwi`+`clrlwi` fused into `extrwi` | the two-instruction byte-extraction pair |
| `-pool off` | one shared `lis` base for the four S-box tables | one `lis`+`addi` pair per table |
| `-use_lmw_stmw off` | `stmw`/`lmw` register save | EABI `_savegpr_14`/`_restgpr_14` calls |

The sweep, one flag at a time (each row adds one flag to the previous row):

| override | effect |
| --- | --- |
| `-inline noauto` | `Camellia_Ekeygen` **100 %** (68 B) - first confirmed function |
| `+ -O3` | `camellia_setup192` **100 %** |
| `+ -opt nopeephole` | `EncryptBlock`/`DecryptBlock` **100 %**; the four S-box users to ~99 % |
| `+ -pool off` | `encrypt/decrypt128/256` **100 %** (4 more) |
| `+ -use_lmw_stmw off` | `camellia_setup128` **100 %** (9 of 10) |

The one residual, `camellia_setup256`, is a source/liveness difference (one extra 4-byte stack slot, all
instructions identical except the r1 offsets); it is documented in the unit's file header comment in
`src/Camellia/camellia.c`. The full blow-by-blow log, including everything that was tried and rejected, is
in `.pi/notes/camellia-match-process.md`.
