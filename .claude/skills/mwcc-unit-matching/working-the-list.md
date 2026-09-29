# Working the playbook list (hand-kept)

Not generated: the ideas tried without a section of their own, and the recipe for working the list. The
generated index of every numbered idea is `references/index.md`; the full text is `references/playbook.md`.

The two tables below hold ideas with **no section of their own**: tried in one unit's context and failed,
or not tried at all. `no` means tried and it did not work (or does not apply) - it stays listed so nobody
re-runs it; `todo` means not tried yet. Neither table needs a status kept current: the moment an idea works
it earns a section, and `references/index.md` picks it up by number. A few `todo` rows were raised by the
`Camellia` flag hunt (`camellia_setup256`), which is closed - re-queue one only if the same shape reappears.

Ruled out so far - tried, and it did not work (details in `docs/matching.md`):

| idea | problem it solves | status |
| --- | --- | --- |
| Compiler-version matrix | "The original used a different compiler release" is the first suspicion and has to be closed once, per unit. | no |
| The rest of the `-opt` axis | An unknown sub-option might be what controls fusion or stack allocation. | no |
| `-Cpp_exceptions` | `extab`/`extabindex` presence suggests exceptions were on; it adds those sections, and with a `new` expression in the body it moves `.text` too - see row 62. | no |
| `-O4`/`-O4,p`/`-O2`, `-schedule off`, `-fp_contract off`, `-ipa off` | Another optimizer level or codegen switch might be the retail setting. | no |
| A paired-single op in a function (the SDK's vector library, `fn_8007270C`'s fill loop) | It reads as a codegen lever and eats flag and shape sweeps. Measured 2026-09-25: **176 of 19,916** functions contain one and **0** of ~2,450 matched functions do, so the frontend cannot emit the body-store form. Record it and move on. | no |

New ideas with no section yet (inexpensive to try, and they earn a section only if they work):

| idea | problem it solves | status |
| --- | --- | --- |
| Third historical source variant | Our `camellia_setup256` matches NSS and NetBSD textually, but the retail file may be a third variant (original NTT 1.2.0 / SDK copy). Diffing another copy's absorb region is the cheapest way to find the source shape the allocator liked. | no - NSS 3.19.1 / 3.24 / 3.28.4, the older NSS fetch and NetBSD NTT-derived are all statement-identical to ours (only `PRUint32`/`SUBL` naming and whitespace); no third variant exists in that lineage. |
| Perturbation probe | The slot outcome is sensitive to IR shape (level 4 flips the frame). Deliberately perturb one independent statement, watch whether `subL[29]` coalesces, then look for the natural source form that produces the same perturbation. | no - 22 statement/pragma perturbations tried; none flips the frame at level 3. The productive version of this idea turned out to be per-function pragmas (row 16). |
| Absorb `dw` / `CAMELLIA_RL1` IR shape | The level-4 near-miss changes exactly this chain, and it is the only region whose IR shape demonstrably moves the frame. Rewrites here (temps, ordering, expression form) are the highest-probability remaining lever. | no - five source forms tried *with* the level-4 pragma (split comma, RL1 temp, operand swap, `tl` rewrite, `tl` temp): the 12-row window is unchanged, so it is level-4 optimizer behaviour, not source shape. |
| Pragma combination search | With the level-4 pragma the frame is correct and only a 12-row window differs; a pragma that suppresses the level-4 reorder would finish the job. | todo - try `opt_lifetimes on`, `opt_common_subs on` explicitly, and level 4 combined with each honoured pragma. |
| Level-4 pragma + window source forms | The window is 12 rows of register choice plus `CAMELLIA_RL1` placement; a source form that makes level 4 emit the target order there would be a 100 % match. | todo - 5 forms tried, more structural rewrites of the whole kw4 chain remain. |

How to work the list:

1. **Work one idea at a time** and record *evidence* - numbers, sizes, first-divergence indices - not
   impressions. For a unit that does not match, walk the ideas in `docs/matching.md` in the order they were
   learned; `references/index.md` is the map, the sections are the detail.
2. When an idea works, it earns a `docs/matching.md` section in the house style - **Problem / Why it
   happens / How to work it / Result / Example**, short and to the point - and `references/index.md` updates
   itself: the section's number is the row's number. **Record it in the same session it works**: a win that
   exists only in a chat message or a scratch report is lost at the next compaction, and the next unit
   re-derives it.
3. The same commit brings the skill's copy with it: `python
   .claude/skills/mwcc-unit-matching/scripts/sync_reference.py --check` must come back clean, because
   `references/` is what a fresh session and every subagent actually load. The index itself is checked by
   `python tools/agents/sync_playbook_index.py --check` (wired into `tools/selftest.py`; it writes `references/index.md`).
4. An idea that fails stays in the table below as `no`, with the evidence that killed it, so it is not
   re-run. A new idea goes there as `todo` first, with the problem it solves; a duplicate number is refused
   by the index tool, so a new section takes the next free number.
5. Unit-specific findings that are not playbook material (a residual diff, a known-bad flag) belong in
   the unit's own header comment, per the matching policy in `CLAUDE.md`.
