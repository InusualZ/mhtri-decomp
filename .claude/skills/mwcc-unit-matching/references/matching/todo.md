# Ideas without a section of their own

Hand-kept (a later stage turns these rows into `status:` values on idea files). The tables below hold ideas with
**no section of their own**: tried in one unit's context and failed, or not tried at all. `no` means tried and it
did not work (or does not apply) - it stays listed so nobody re-runs it; `todo` means not tried yet. The moment
an idea works it earns an `NNN-slug.md` file. A few `todo` rows were raised by the `Camellia` flag hunt
(`camellia_setup256`), which is closed - re-queue one only if the same shape reappears.

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
