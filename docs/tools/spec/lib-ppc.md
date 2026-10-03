# `lib/ppc` - PowerPC instruction decode and the reference scanner, once

## Purpose

Decodes Gekko/Broadway instruction words (fields, branch targets, the predicates the flag inference counts), scans retail code
bytes for the data addresses a function forms, reads or writes, and decodes the GPR reads/writes of one *disassembled* line.

## Users

`splitcheck` (`scan_refs`, `scan_calls`, `written_reg`, `find_sda_bases` and the opcode tables, through `lib.refs.text_refs`),
`phantom` (`branch_target`, `materialisations`, `looks_like_prologue`, `is_dead_epilogue`), `infer` (the field helpers,
`rlwinm_alias`, `Insn` as its base class), `callees` and `accessextent` (`decode_rw`, `CALL_MNEMONICS`), `dossier`
(`decode_li`), `lib.refs` (the dump's branch targets and the `arg` hint).

## Public API

* Field helpers on a word: `op`, `rt`, `ra`, `rb`, `frc`, `xo5`, `xo10`, `sh`, `mb`, `me`, `ui`, `si` (signed 16),
  `displacement` (12-bit for `psq_l/lu/st/stu`, else 16), `signed(value, bits)`, `words(code) -> tuple[int]`.
* `Insn(address, word)` (frozen, slotted; `op` stored, every other field a property): `rt`/`rs`/`ra`/`rb`/`si`/`ui`/`d`/
  `sh`/`mb`/`me`, `target`, `width`, and the predicates `is_record`, `is_fma`, `is_fmul`, `is_faddsub`, `is_psq_indexed`,
  `rlwinm_alias`, `is_clr_mask`, `is_shift`, `is_narrow_store`, `is_branch`, `is_branch_link` (an I-form `bl`), `is_call`
  (any branch with LK), `is_lis`, `is_load`, `is_store`, `d_form`. `decode(word, address=0) -> Insn`.
* `branch_target(address, word)`: relative `b`/`bl` (26-bit LI) and `bc`/`bcl` (16-bit BD); None for `AA=1` or a non-branch.
* `rlwinm_alias(word)`, `decode_li(word) -> (rD, imm) | None`, `written_reg(word)`.
* `find_sda_bases(code_words) -> (r13, r2)` over `[(address, word)]` (the `lis` + `ori|addi` pairs of `__init_registers`).
* `scan_refs(code, start, sda13, sda2, is_data, fn_starts=(), loads=None, stores=None, passes=None) -> {address: [site]}`
  and `scan_calls(code, start, fn_starts) -> [(site, target)]` - `splitcheck`'s scanner, verbatim.
* `materialisations(code, start, window=16) -> [(site, value)]`: the narrow `lis rD` + `addi|ori rD, rD` idiom.
* `looks_like_prologue(word)`, `is_dead_epilogue(code)`.
* `decode_rw(mnemonic, operands) -> (reads, writes, is_branch, is_call, decoded)`; `BRANCH_MNEMONICS`, `CALL_MNEMONICS`.
* Tables: `LOADS_INT`, `STORES`, `FP_MEM`, `LOAD_OPS`, `STORE_OPS`, `UPDATE_OPS`, `WIDTH`, `VOLATILE`, `X_ARITH`, `X_LOADS`,
  `X_LOGIC`, `RECORD_OPCODES`, `FMA_XO`/`FMUL_XO`/`FADD_XO`/`FSUB_XO`, `PSQ_INDEXED_XO`, `NARROW_STORE_OPS`, `LIS_WINDOW`.

## Invariants and rules

* **`scan_refs`** (the reader `dataattach` proved on the whole DOL): an address is a `lis` high half plus an `addi`/`ori`/load
  displacement, or r13/r2 plus a displacement. A `lis` value is forgotten at a function start, after a write to its register,
  and - in a volatile register - after `LIS_WINDOW` (200) instructions; r14..r31 survive calls. `mr rA, rS` copies what rS
  held; an update-form access leaves rA at the effective address; `lmw rD` kills rD..r31. A `bl` clobbers r0 and r3..r12. The
  `addi`/`ori` that forms an address is a **reference, not a read**: `loads` gets only loads (through r13/r2, `lis` + load, or a
  register an `addi`/`ori` formed), `stores` only stores. `passes` records the r3..r10 that hold a formed address at a `bl`
  (and at a tail-call `b` to a function start).
* `scan_calls` reports a branch to a function start other than the start of the function holding the site (a loop to its own
  start is not a call).
* `materialisations` is the phantom scan's narrow idiom: same register, at most `window` bytes apart, and the low half is
  **added unsigned** (`lis 0x8000` + `addi 0x8000` -> `0x80008000`), not sign-extended as the hardware would - the phantom
  verdicts were measured with that rule, so it is kept and pinned by the test.
* `Insn.op` is stored at construction: `infer` reads it in its hot loops (measured on the 411 MAIN target objects: 25.0 s old
  vs 25.2 s new; a property `op` cost +10 %).
* `infer`'s `writes_gpr`/`reads_gpr` are **not** `written_reg`: they are infer's best-effort GPR def/use sets (they count a
  store's rS as written), kept in `infer.Insn` because its fingerprints were calibrated on them. Folding them into one rule
  is a behaviour change to measure (see Known gaps).

## Absorbs (today's implementations)

`splitcheck.scan_refs/scan_calls/written_reg/find_sda_bases` + the opcode tables, `phantom.index_refs`'s decode (the branch and
`lis` loops), `phantom.looks_like_prologue/is_dead_epilogue`'s word tests, `infer`'s field helpers, opcode sets, `rlwinm_alias`
and `Insn` predicates, `dossier.decode_li`, `callers.branch_target`'s decode, `callees.decode_rw` and its mnemonic tables.

## Lib dependencies

None (stdlib).

## Test contract

Tier: fixture (`tools/tests/lib/test_ppc.py`, 62 checks). Hand-assembled words for every class (the `infer_selftest` encoders
plus `lis`/`ori`/`lwzu`/`mr`/`bc`): fields, branch targets (forward, backward, `bc`, `AA=1`), the infer predicates, the prologue
and dead-epilogue shapes, `written_reg`, `find_sda_bases`, `scan_refs` (formed vs read, r13/r2, the call clobber, a callee-saved
`lis` surviving, `mr`, function-start reset, the window, the update form, an X-form write, `is_data`), `scan_calls`,
`materialisations` (window, unsigned low half), `decode_rw`.

## Known gaps

* `infer.Insn.writes_gpr`/`reads_gpr` remain a second, looser def/use rule beside `written_reg` (measuring a fold means
  re-running `infer --accuracy`; out of this package's scope).
* `vtableaudit.is_code_pointer` and `vtslot.find_word_hits` (data words that are code addresses) are not instruction decode and
  stay in their tools; `dossier.panic_calls` keeps its window logic over `decode_li`.
* The disassembly-*text* parsers (`accessextent.split_insn/parse_mem`, `m2cinput.parse`, `callees.parse_disasm`) keep their
  format-specific parsing (`duplication.md` (b)); only the register decode is shared.
