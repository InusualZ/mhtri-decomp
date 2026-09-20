<!-- GENERATED FILE - do not edit.
     Source: docs/matching.md
     Regenerate: python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py
-->

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
instructions identical except the r1 offsets); it is documented on top of the function in
`src/Camellia/camellia.c`. The full blow-by-blow log, including everything that was tried and rejected, is
in `.pi/notes/camellia-match-process.md`.
