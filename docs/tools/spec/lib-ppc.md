# `lib/ppc` - PowerPC instruction decode and the reference scanner, once

## Purpose

PowerPC instruction decode and the reference scanner, once.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `decode(word, address) -> Insn`: opcode class, fields (`rt`, `ra`, `rb`, `si`, `d`, `sh/mb/me`), `is_branch`, `is_call`, `target`, `is_load/is_store`, `width`, `d_form`, `record`, `fused`, `rlwinm_alias`
* `branch_target(address, word)`; `materialisations(insns)` (`lis`+`addi`/`ori` pairs, sda bases); `find_sda_bases(dol)`
* `scan_refs(code, base, sda13, sda2, fn_starts) -> [Ref]` with `splitcheck.scan_refs`'s semantics (loads, stores, calls, pool literals by loads only)
* `looks_like_prologue(words)`, `is_dead_epilogue(words)`

## Absorbs (today's implementations)

`splitcheck.scan_refs/scan_calls/written_reg/find_sda_bases`, `phantom.index_refs/looks_like_prologue`, `infer.Insn` and field helpers, `dossier.decode_li`, `callers.branch_target`, `vtableaudit.is_code_pointer`

## Test contract

Tier: fixture (a lib test never reads the live tree). hand-assembled words for every class (the `infer_selftest` encoders become the fixture); `scan_refs` on `splitcheck`'s mini DOLs reproduces its reference sets

## Known gaps

None until implemented; `migration.md` names the package.
