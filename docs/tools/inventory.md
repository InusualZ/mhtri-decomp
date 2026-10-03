# Tool inventory (every tracked file under `tools/`)

Measured on branch `worktree-agent-a086d791dd01bc183` at `ec2609b46` (2026-10-03). 184 tracked paths: 183 files below plus the
`tools/m2c` submodule (vendored, out of scope). Numbers come from `git ls-files`, a tokenizer pass (doc = docstring lines,
cmt = `#` comment lines, prose % = (doc+cmt)/(non-blank lines)), `git log -1 --format=%cs` (last), `git log --format=%h | wc -l`
(commits), and a path-qualified reference scan over the tracked tree plus `MAIN/.pi/bin` (callers; `gate` = `tools/units/land.py`,
`selftest` = `tools/selftest.py`, `pi-bin` = the untracked landing scripts).

## Verdict counts

| verdict | files | meaning |
| --- | ---: | --- |
| CORE-GATE | 48 | a landing row, a ninja rule or the selftest harness depends on it |
| CORE-EVIDENCE | 56 | lane-facing analysis the decompiler loop uses |
| SPLITS-PROGRAM | 7 | the (now applied) splits program's proposal pipeline |
| AGENT-PLUMBING | 22 | claims, slots, queue, brief, backlog, launch lines, hooks |
| MATCHING-EXPERIMENT | 32 | flag/shape search, compiler and linker debuggers |
| TEMPLATE | 7 | dtk-template scripts at the top level |
| DEAD | 11 | nothing references it, or it is a one-shot/snapshot/superseded tool |
| **total** | **183** | plus `tools/m2c` (submodule) |

Totals: 168 Python files, 113597 lines (13768 docstring, 5526 comment, 82962 code, rest blank); 51 standalone `*_selftest.py` files
(14812 lines) and 73 tools exposing `--selftest`; `tools/selftest.py --list` discovers **89** entries after collapsing wrapper pairs.
The two JSON queues are 112229 lines together and are data, not code.

## How to read the tables

* **prose** - docstring + comment lines and their share of non-blank lines. Above ~25 % the file is carrying its spec in code;
  the prose audit (`prose-audit.md`) says where each block belongs.
* **callers** - who references the file, by bucket and count. `none` means nothing in the tracked tree or `.pi/bin` names it.
* **selftest** - `own file` (a `*_selftest.py` beside it), `--selftest` (the tool exposes the flag), both, or `none`.
* **overlap** - the family the file shares a concept with; `duplication.md` quantifies each family.

## `tools/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `__init__.py` | 0 | 0 (0%) | 2025-04-26 / 3 | none | none | TEMPLATE | Empty package marker so `from units import x` resolves under `tools/`. |
| `changes_fmt.py` | 162 | 2 (2%) | 2025-05-20 / 3 | none | tools 1 | TEMPLATE | dtk-template: render the `ninja changes` progress diff as text/markdown. |
| `decompctx.py` | 178 | 10 (7%) | 2025-08-30 / 9 | none | tools 1 | TEMPLATE | dtk-template: inline a source file's includes into one context file for decomp.me. |
| `download_tool.py` | 152 | 11 (9%) | 2025-11-11 / 11 | none | claude.md 1, tools 1 | TEMPLATE | dtk-template: download a pinned toolchain binary (binutils, compilers, dtk, objdiff-cli, sjiswrap, wibo). |
| `mwlink_debugger.py` | 3743 | 666 (19%) | 2026-09-28 / 4 | --selftest | agents 5, skills 12, claude.md 1, docs 23, tools 19 | MATCHING-EXPERIMENT | Interrogate `mwldeppc.exe` about a real link: trace a unit, diagnose a failing link, verify the map, derive phases/anchors/records/alignment from the PE. |
| `ninja_syntax.py` | 253 | 39 (18%) | 2026-09-22 / 9 | none | tools 3 | TEMPLATE | Vendored ninja build-file writer used by project.py. |
| `project.py` | 2176 | 224 (12%) | 2026-09-27 / 64 | none | configure 1, skills 6, claude.md 1, docs 7, tools 9 | TEMPLATE | dtk-template build generator: `build.ninja`, `objdiff.json`, `compile_commands.json`, progress; chains objalign/objextab into every MWCC rule. |
| `selftest.py` | 885 | 159 (20%) | 2026-09-30 / 10 | --selftest | gate 10, agents 2, skills 1, claude.md 2, docs 16, tools 14 | CORE-GATE | One runner for every tool selftest: discovery, dedupe of wrapper pairs, parallel bounded execution, flake retry, park list, `--changed` source mapping. |
| `selftests-known-failures.json` | 11 | 0 (0%) | 2026-09-27 / 1 | none | gate 2, selftest 2, claude.md 1, docs 2, tools 1 | CORE-GATE | The park list: a selftest allowed to be red, with reason, date and head; a park that passes is itself a failure. |
| `spawnretry.py` | 50 | 14 (34%) | 2026-09-30 / 1 | own file | selftest 1, docs 1, tools 4 | CORE-GATE | Retry `Popen` on Windows' transient `WinError 5` at process launch. |
| `spawnretry_selftest.py` | 85 | 2 (3%) | 2026-09-30 / 1 | (is one) | none | CORE-GATE | Selftest: WinError 5 is retried, anything else raised at once. |
| `transform_dep.py` | 84 | 14 (20%) | 2025-04-26 / 6 | none | tools 3 | TEMPLATE | dtk-template: rewrite MWCC `.d` dependency files to ninja's path form. |
| `unitutil.py` | 739 | 202 (31%) | 2026-09-30 / 12 | own file | gate 1, skills 4, claude.md 1, docs 2, tools 47 | CORE-GATE | The proto-lib: repo/main-tree resolution, unit spec resolution, the real ninja compile command, flag overrides, ELF frame readers, the official objdiff `report generate` metric, scratch dirs. |
| `unitutil_selftest.py` | 220 | 32 (17%) | 2026-09-30 / 3 | (is one) | none | CORE-GATE | Selftest: `repo_root(start=)` and `resolve_unit(root=)` root a fixture tree, never this file's directory. |

Inputs -> outputs, CLI shape, overlap family:

* `__init__.py` - -; CLI `-`; imports: -
* `changes_fmt.py` - changes JSON -> stdout; CLI `changes_fmt.py [--all] <file>`; imports: -
* `decompctx.py` - src -> ctx.c; CLI `decompctx.py <file> [-o]`; imports: -
* `download_tool.py` - tag -> build/tools/*; CLI `download_tool.py <tool> <out> --tag T`; imports: -
* `mwlink_debugger.py` - main.elf, link rsp, mwld PE -> report; CLI `mwlink_debugger.py {info,messages,order,anchors,records,align,timeline,verify,trace,diagnose,phases} [--json]`; overlap: compiler/linker; imports: -
* `ninja_syntax.py` - -; CLI `-`; imports: -
* `project.py` - configure.py config -> build.ninja, objdiff.json; CLI `(library; configure.py calls it)`; imports: ninja_syntax
* `selftest.py` - tools/** -> table/JSON, exit; CLI `selftest.py [--changed [REF]] [--json] [--list] [--no-dedupe] [--selftest]`; overlap: test harness; imports: -
* `selftests-known-failures.json` - -; CLI `-`; overlap: test harness; imports: -
* `spawnretry.py` - -; CLI `(library)`; overlap: subprocess; imports: -
* `spawnretry_selftest.py` - -; CLI `python <file>`; imports: spawnretry
* `transform_dep.py` - .d -> .d; CLI `transform_dep.py <in> <out>`; imports: -
* `unitutil.py` - objdiff.json, build.ninja, src/ -> Unit, tokens, scores; CLI `(library; `unitutil.py` lists units)`; overlap: lib seed; imports: spawnretry
* `unitutil_selftest.py` - -; CLI `python <file>`; imports: unitutil

## `tools/agents/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `edit.py` | 212 | 26 (15%) | 2026-09-30 / 1 | own file | agents 11, docs 3, tools 3 | AGENT-PLUMBING | Line-ending-safe text edits: `replace` (CRLF/LF-agnostic, count-asserted), `normalise`, `check` (tree vs index endings). |
| `edit_selftest.py` | 138 | 7 (6%) | 2026-09-30 / 1 | (is one) | none | AGENT-PLUMBING | Selftest for edit.py on LF/CRLF/mixed temp files. |
| `ideas.py` | 524 | 58 (13%) | 2026-09-30 / 3 | own file + --selftest | gate 2, selftest 14, skills 61, claude.md 2, docs 63, tools 8 | CORE-GATE | Find, read, add and check the matching playbook's ideas; `check`/`demo-check` are the docs-batch gate rows selftest.py maps. |
| `ideas_demo.py` | 305 | 34 (12%) | 2026-09-29 / 1 | none | skills 1, docs 1, tools 4 | CORE-GATE | Library for `ideas.py demo-check`: compile a demo with the real MWCC and evaluate its EXPECT lines against objdump. |
| `ideas_selftest.py` | 365 | 20 (6%) | 2026-09-29 / 2 | (is one) | tools 2 | CORE-GATE | Fixture tests for ideas.py (ranking, `new` scaffolding, id race, refusals) plus a real-tree check. |
| `install.sh` | 41 | 0 (0%) | 2026-09-29 / 3 | none | claude.md 2, docs 10 | AGENT-PLUMBING | Copy `.claude/agents/*.md` to `~/.claude/agents/` after `sync_profiles.py --check` passes. |
| `profileprobe.py` | 136 | 22 (17%) | 2026-09-29 / 9 | none | docs 6, tools 1 | AGENT-PLUMBING | Write a recall probe for a subagent profile and print the launch line and checklist. |
| `sync_playbook_index.py` | 370 | 49 (15%) | 2026-09-29 / 6 | own file + --selftest | selftest 18, agents 1, skills 8, claude.md 1, docs 5, tools 10 | CORE-GATE | Generate `docs/matching/index.md` from idea front matter; `--check` is a docs-batch gate row; its parser is the one every playbook tool imports. |
| `sync_playbook_index_selftest.py` | 210 | 18 (10%) | 2026-09-29 / 6 | (is one) | docs 1, tools 2 | CORE-GATE | Fixture tests for the index generator's refusals, ordering, `--where`, `--json` and the real tree. |
| `sync_profiles.py` | 366 | 104 (32%) | 2026-09-29 / 9 | own file + --selftest | selftest 6, agents 16, claude.md 2, docs 10, tools 8 | CORE-GATE | Inject the section 6.5 rule block (generated from docs/plan.md) into every profile; `--check` is a docs-batch gate row. |
| `sync_profiles_selftest.py` | 240 | 32 (15%) | 2026-09-29 / 5 | (is one) | docs 2, tools 3 | CORE-GATE | Fixture + real-tree tests: every profile in sync, no stale escape taught, coverage tripwire for unlisted profiles. |

Inputs -> outputs, CLI shape, overlap family:

* `edit.py` - files -> files, diff; CLI `edit.py replace FILE --old-file A --new-file B [--count N] | normalise FILE.. | check [--fix]`; overlap: line endings; imports: -
* `edit_selftest.py` - -; CLI `python <file>`; imports: edit, unitutil
* `ideas.py` - docs/matching/*.md -> stdout, new idea files; CLI `ideas.py {find,show,where,new,check,demo-check} [--selftest]`; overlap: playbook; imports: ideas_demo, sync_playbook_index
* `ideas_demo.py` - demo .cpp -> verdicts; CLI `(library)`; overlap: playbook; imports: unitutil
* `ideas_selftest.py` - -; CLI `python <file>`; imports: ideas, ideas_demo, sync_playbook_index
* `install.sh` - .claude/agents -> ~/.claude/agents; CLI `install.sh`; imports: -
* `profileprobe.py` - profile -> .pi/probes/*.md; CLI `profileprobe.py <agent>..`; imports: -
* `sync_playbook_index.py` - docs/matching/NNN-*.md -> index.md; CLI `sync_playbook_index.py [--check|--print|--where N|--json|--selftest]`; overlap: playbook; imports: -
* `sync_playbook_index_selftest.py` - -; CLI `python <file>`; imports: sync_playbook_index
* `sync_profiles.py` - docs/plan.md -> .claude/agents/*.md; CLI `sync_profiles.py [--check|--print|--selftest]`; overlap: profiles; imports: -
* `sync_profiles_selftest.py` - -; CLI `python <file>`; imports: sync_profiles, brief

## `tools/elf/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `dwarfmap.py` | 231 | 12 (6%) | 2026-09-20 / 1 | none | skills 2, claude.md 1, docs 1 | MATCHING-EXPERIMENT | Dump the DWARF2 local-variable -> stack-slot map MWCC produced for one function of a `-gdwarf-2` object. |
| `elfsect.py` | 44 | 1 (2%) | 2026-09-20 / 1 | none | agents 1, skills 16, claude.md 2, docs 8, tools 6 | CORE-EVIDENCE | Minimal ELF32-BE section dumper and importable `sections()` reader (the skill's byte-level helper). |
| `objalign.py` | 267 | 53 (23%) | 2026-09-26 / 1 | --selftest | skills 3, docs 6, tools 27 | CORE-GATE | Post-compile ninja step: lower a section's `sh_addralign` to `lowbit(claimed start)` so mwld can place an odd-start unit. |
| `objextab.py` | 483 | 92 (22%) | 2026-09-27 / 1 | --selftest | skills 2, docs 6, tools 12 | CORE-GATE | Post-compile ninja step: rename MWCC's ordinal `extab`/`extabindex` symbols to the map's `@etb_/@eti_` spelling and make them global. |

Inputs -> outputs, CLI shape, overlap family:

* `dwarfmap.py` - object -> table; CLI `dwarfmap.py <obj> <function> [--loc]`; overlap: binary readers; imports: -
* `elfsect.py` - object -> sections; CLI `elfsect.py <obj>..`; overlap: binary readers; imports: unitutil
* `objalign.py` - object + splits.txt -> object; CLI `objalign.py <obj> [--splits F] [--unit K] [--dry-run] [--selftest]`; overlap: build steps; imports: -
* `objextab.py` - object + splits.txt -> object; CLI `objextab.py <obj> [--splits F] [--unit K] [--dry-run] [--selftest]`; overlap: build steps; imports: objalign

## `tools/flags/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `README.md` | 73 | 0 (0%) | 2026-09-23 / 1 | --selftest | none | MATCHING-EXPERIMENT | Index of the flag tools (doc). |
| `frame.py` | 75 | 15 (24%) | 2026-09-20 / 1 | none | skills 4, claude.md 1, docs 3, tools 1 | MATCHING-EXPERIMENT | Per-function prologue frame size of a unit under flag/compiler variants, next to the target's. |
| `infer-run.md` | 266 | 0 (0%) | 2026-09-26 / 2 | none | tools 1 | DEAD | A generated snapshot of `infer.py --markdown` over the tree (stale the day after it was written). |
| `infer.py` | 1055 | 147 (16%) | 2026-09-27 / 4 | own file + --selftest | gate 2, configure 2, skills 1, claude.md 1, docs 2, tools 14 | MATCHING-EXPERIMENT | Infer compiler flags from a target object's bytes with evidence and confidence (peephole, fp_contract, pool, str, lmw/stmw, func_align, inline, extab). |
| `infer_selftest.py` | 404 | 49 (15%) | 2026-09-27 / 3 | (is one) | tools 2 | MATCHING-EXPERIMENT | Decoder unit tests + synthetic ELF per fingerprint + known-case accuracy on real objects (parked: pool false positive). |
| `mwcc_matrix.py` | 170 | 41 (27%) | 2026-09-28 / 4 | none | skills 11, claude.md 1, docs 10, tools 3 | MATCHING-EXPERIMENT | Compile a unit across MWCC versions / flag overrides and summarise the official report metric per variant. |
| `optsweep.py` | 78 | 17 (27%) | 2026-09-20 / 1 | none | skills 8, claude.md 1, docs 6, tools 1 | MATCHING-EXPERIMENT | Sweep `-opt` sub-options on a unit and print frame sizes. |
| `shapes.py` | 817 | 100 (14%) | 2026-09-23 / 1 | own file | skills 1, claude.md 1, docs 2, tools 3 | MATCHING-EXPERIMENT | Pure source-shape generators (`body -> [(name, body)]`) for shapesearch. |
| `shapes_selftest.py` | 198 | 18 (11%) | 2026-09-23 / 1 | (is one) | selftest 1 | MATCHING-EXPERIMENT | Offline tests of the lexical layer and each generator. |
| `shapesearch.py` | 578 | 83 (16%) | 2026-09-28 / 3 | own file + --selftest | skills 8, claude.md 1, docs 12, tools 8 | MATCHING-EXPERIMENT | Search source shapes (or hand-written candidate bodies) of one function for the target's codegen; scores with the official metric in scratch. |
| `shapesearch_selftest.py` | 116 | 16 (17%) | 2026-09-28 / 1 | (is one) | tools 1 | MATCHING-EXPERIMENT | Offline tests of the candidate-expression mode. |
| `tryvar.py` | 184 | 43 (26%) | 2026-09-30 / 3 | none | skills 7, claude.md 1, docs 8, tools 7 | MATCHING-EXPERIMENT | Try named source rewrites of a unit from a variants file and report the official per-function metric; `--apply` lands the winner. |

Inputs -> outputs, CLI shape, overlap family:

* `README.md` - -; CLI `-`; imports: -
* `frame.py` - unit -> table; CLI `frame.py [-u U] [--flags-extra] [--obj] [--versions]`; overlap: flag search; imports: unitutil
* `infer-run.md` - -; CLI `-`; imports: -
* `infer.py` - target object -> flags + evidence; CLI `infer.py <unit|obj> [--all] [--accuracy] [--json] [--markdown] [--selftest]`; overlap: flag search; decoder; imports: unitutil
* `infer_selftest.py` - -; CLI `python <file>`; imports: infer, unitutil
* `mwcc_matrix.py` - unit -> build/tmp/matrix/*; CLI `mwcc_matrix.py [-u U] [--flags-extra] [--list-versions] [versions..]`; overlap: flag search; imports: unitutil
* `optsweep.py` - unit -> table; CLI `optsweep.py [-u U] [subopts..] [--flags-extra]`; overlap: flag search; imports: unitutil
* `shapes.py` - -; CLI `(library)`; overlap: shape search; imports: -
* `shapes_selftest.py` - -; CLI `python <file>`; imports: shapes
* `shapesearch.py` - unit, function -> ranked table, build/tmp/shapes; CLI `shapesearch.py -u U [-f FN] [--gens] [--depth] [--expr..|--expr-file] [--emit] [--list-gens] [--selftest]`; overlap: shape search; imports: shapes, unitutil
* `shapesearch_selftest.py` - -; CLI `python <file>`; imports: shapesearch
* `tryvar.py` - unit + variants/<lib>.py -> table, src edit; CLI `tryvar.py [-u U] [--variants F] [--list] [--apply NAME]`; overlap: shape search; imports: unitutil

## `tools/flags/variants/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `camellia.py` | 284 | 45 (17%) | 2026-09-20 / 1 | none | skills 1, docs 1 | MATCHING-EXPERIMENT | Data for tryvar: the Camellia rewrites tried (all rejected, per its own header). |

Inputs -> outputs, CLI shape, overlap family:

* `camellia.py` - -; CLI `(data)`; imports: -

## `tools/git/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `commitlint.py` | 435 | 91 (24%) | 2026-09-29 / 3 | own file + --selftest | gate 10, agents 1, tools 5 | CORE-GATE | Lint a commit subject against the CLAUDE.md convention; member sets derived from the tree; the gate lints its own subject with it. |
| `commitlint_selftest.py` | 295 | 39 (16%) | 2026-09-29 / 3 | (is one) | tools 1 | CORE-GATE | Fixture tree + fixture history tests of the five checks and three modes. |
| `guard.py` | 197 | 56 (34%) | 2026-09-29 / 3 | own file | tools 4 | CORE-GATE | Pre-commit guard logic: warn on `core.autocrlf`, normalise a CRLF text blob in the index, refuse a binary with CR. |
| `guard_selftest.py` | 192 | 19 (12%) | 2026-09-29 / 7 | (is one) | docs 1, tools 1 | CORE-GATE | Temp-repo tests of the EOL case, the DOL-hash cross-check and the hook. |
| `prepcommit.py` | 333 | 48 (17%) | 2026-09-29 / 7 | none | gate 1, skills 2, docs 13, tools 4 | CORE-GATE | Classify `git status` paths into stage/refuse, write a results-bearing message, print the commit command (land.py imports its classifier). |

Inputs -> outputs, CLI shape, overlap family:

* `commitlint.py` - message -> findings, exit 0/1/2; CLI `commitlint.py <msgfile> | --message S | --last N | --install-hook | --selftest`; overlap: landing; imports: -
* `commitlint_selftest.py` - -; CLI `python <file>`; imports: commitlint
* `guard.py` - index -> index; CLI `guard.py {autocrlf,eol} [--root]`; overlap: line endings; imports: -
* `guard_selftest.py` - -; CLI `python <file>`; imports: guard, prepcommit
* `prepcommit.py` - git status, report.json -> staged index, .git/prepcommit_msg.txt; CLI `prepcommit.py [--dry-run] [--split] [--message S] [--commit]`; overlap: landing; imports: land

## `tools/git/hooks/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `pre-commit` | 42 | 0 (0%) | 2026-09-29 / 6 | none | gate 2, claude.md 1, docs 3, tools 7 | CORE-GATE | The per-clone pre-commit hook: calls guard.py autocrlf/eol. |

Inputs -> outputs, CLI shape, overlap family:

* `pre-commit` - -; CLI `(git hook)`; imports: -

## `tools/mwcc-debugger/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `LICENSE` | 116 | 0 (0%) | 2026-09-27 / 1 | none | none | MATCHING-EXPERIMENT | Upstream licence (vendored). |
| `PROVENANCE.md` | 167 | 0 (0%) | 2026-09-27 / 1 | none | none | MATCHING-EXPERIMENT | Upstream commit, vendored hashes and the list of port changes (doc). |
| `README.md` | 244 | 0 (0%) | 2026-09-27 / 2 | none | none | MATCHING-EXPERIMENT | How to run the compiler under gdb and read the dumps (doc). |
| `fetch_gdb.py` | 147 | 26 (20%) | 2026-09-27 / 1 | none | agents 2, docs 1, tools 9 | MATCHING-EXPERIMENT | Fetch a native mingw-w64 gdb from the MSYS2 package database without pacman. |
| `make_port.py` | 1083 | 14 (1%) | 2026-09-27 / 1 | none | tools 8 | MATCHING-EXPERIMENT | One-shot, anchored transformation of the vendored upstream into the Windows/Wii port (kept so the port diff is reproducible). |
| `mwcc_debugger.py` | 2084 | 193 (10%) | 2026-09-27 / 1 | none | tools 36 | MATCHING-EXPERIMENT | Windows-native port of cadmic/mwcc-debugger: run mwcceppc.exe under gdb and dump AST/PCode/regalloc per pass. |
| `versions.py` | 551 | 133 (26%) | 2026-09-27 / 2 | none | tools 20 | MATCHING-EXPERIMENT | Per-compiler-build address tables (RVA) and the PE reader/detect used by the debugger and verify_pcode. |

Inputs -> outputs, CLI shape, overlap family:

* `LICENSE` - -; CLI `-`; overlap: compiler/linker; imports: -
* `PROVENANCE.md` - -; CLI `-`; overlap: compiler/linker; imports: -
* `README.md` - -; CLI `-`; overlap: compiler/linker; imports: -
* `fetch_gdb.py` - network -> prefix dir; CLI `fetch_gdb.py --dest D [--cache] [--packages]`; overlap: compiler/linker; imports: -
* `make_port.py` - upstream py -> port py; CLI `make_port.py <upstream> <out>`; overlap: compiler/linker; imports: versions
* `mwcc_debugger.py` - source + flags -> build/mwcc-debug dumps; CLI `mwcc_debugger.py --exe E --gdb G --args ..`; overlap: compiler/linker; imports: versions
* `versions.py` - exe -> MwccVersion; CLI `(library)`; overlap: compiler/linker; imports: -

## `tools/mwcc-debugger/locate/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `README.md` | 197 | 0 (0%) | 2026-09-27 / 2 | none | none | MATCHING-EXPERIMENT | How the Wii/1.3 breakpoint row was derived (doc). |
| `dissect.py` | 168 | 14 (9%) | 2026-09-27 / 1 | none | tools 10 | MATCHING-EXPERIMENT | Read the CodeView NB11 symbol blob of an MWCC PE: list symbols, disassemble a range. |
| `extract_upstream_tables.py` | 55 | 11 (23%) | 2026-09-27 / 1 | none | tools 3 | MATCHING-EXPERIMENT | One-shot: lift the GC address tables out of upstream as RVAs. |
| `pass_points.py` | 97 | 18 (20%) | 2026-09-27 / 1 | none | tools 9 | MATCHING-EXPERIMENT | Derive the PCode breakpoint table from the pass call sites in the compiler PE. |
| `verify_pcode.py` | 625 | 89 (16%) | 2026-09-27 / 2 | own file | agents 1, skills 1, claude.md 1, docs 5, tools 9 | MATCHING-EXPERIMENT | Classify a PCode dump (final vs earlier pass) and compare it to the object: MATCH / FAIL / PASS-DELTA naming the pass. |
| `verify_pcode_selftest.py` | 289 | 33 (13%) | 2026-09-27 / 1 | (is one) | tools 2 | MATCHING-EXPERIMENT | Fixture dumps + an assembled object; objdump rows skipped without binutils. |

Inputs -> outputs, CLI shape, overlap family:

* `README.md` - -; CLI `-`; overlap: compiler/linker; imports: -
* `dissect.py` - mwcceppc.exe -> listing; CLI `dissect.py {syms,dis} <exe> ..`; overlap: compiler/linker; imports: -
* `extract_upstream_tables.py` - upstream py -> table; CLI `extract_upstream_tables.py <upstream> [exe]`; overlap: compiler/linker; imports: -
* `pass_points.py` - exe -> table; CLI `pass_points.py <exe> [rva:rva..]`; overlap: compiler/linker; imports: dissect
* `verify_pcode.py` - dump + object -> verdict; CLI `verify_pcode.py <dump> <obj> [--json] [--final] [--strict]`; overlap: compiler/linker; imports: versions
* `verify_pcode_selftest.py` - -; CLI `python <file>`; imports: verify_pcode

## `tools/mwcc-debugger/upstream/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `README.md` | 203 | 0 (0%) | 2026-09-27 / 1 | none | none | MATCHING-EXPERIMENT | Upstream README (vendored). |
| `mwcc_debugger.py` | 1742 | 76 (5%) | 2026-09-27 / 1 | none | tools 39 | MATCHING-EXPERIMENT | Vendored upstream copy (byte-identical, see PROVENANCE). |

Inputs -> outputs, CLI shape, overlap family:

* `README.md` - -; CLI `-`; overlap: compiler/linker; imports: -
* `mwcc_debugger.py` - -; CLI `-`; overlap: compiler/linker; imports: -

## `tools/mwlink-debugger/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `README.md` | 259 | 0 (0%) | 2026-09-28 / 3 | --selftest | none | MATCHING-EXPERIMENT | How to use mwlink_debugger.py (doc). |

Inputs -> outputs, CLI shape, overlap family:

* `README.md` - -; CLI `-`; overlap: compiler/linker; imports: -

## `tools/mwlink-debugger/locate/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `README.md` | 593 | 0 (0%) | 2026-09-28 / 1 | --selftest | none | MATCHING-EXPERIMENT | How the linker's anchors/phases/records were derived (doc). |

Inputs -> outputs, CLI shape, overlap family:

* `README.md` - -; CLI `-`; overlap: compiler/linker; imports: -

## `tools/objdiff/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `freshguard.py` | 189 | 61 (37%) | 2026-09-28 / 1 | own file | skills 1, docs 1, tools 11 | CORE-EVIDENCE | The one staleness rule: is a prebuilt object/report older than any source in the unit's include closure (strict `<`). |
| `freshguard_selftest.py` | 123 | 21 (20%) | 2026-09-28 / 1 | (is one) | none | CORE-EVIDENCE | Fixture tests of closure, newest, freshness and unit_reasons. |
| `metric_selftest.py` | 434 | 53 (14%) | 2026-09-28 / 4 | (is one) | selftest 1 | CORE-EVIDENCE | Contract test: symdiff/tryvar/mwcc_matrix/unitutil all print the `report generate` metric, never `diff`'s positional one. |
| `pairgap.py` | 877 | 164 (21%) | 2026-09-28 / 1 | own file + --selftest | agents 2, docs 4, tools 3 | CORE-EVIDENCE | Per unit: symbols objdiff declines to pair (size gap), missing on our side, extra on ours, cross-checked with the report score. |
| `pairgap_selftest.py` | 20 | 9 (53%) | 2026-09-28 / 1 | (is one) | none | CORE-EVIDENCE | Standalone entry that delegates to `pairgap.py --selftest`. |
| `relocdiff.py` | 441 | 89 (22%) | 2026-09-29 / 2 | own file + --selftest | agents 3, skills 1, docs 3, tools 3 | CORE-EVIDENCE | Relocation-level diff of a unit (both sides, four difference classes; `--by-owner` aligns per containing symbol). |
| `relocdiff_selftest.py` | 215 | 34 (19%) | 2026-09-29 / 2 | (is one) | tools 1 | CORE-EVIDENCE | Pure-rule tests + ELF fixtures (builder borrowed from sectiongap_selftest). |
| `slotmap.py` | 157 | 30 (21%) | 2026-09-23 / 2 | none | skills 3, claude.md 1, docs 2, tools 3 | MATCHING-EXPERIMENT | r1-relative stack-slot map between target and ours for one symbol, by index-aligned majority vote. |
| `symdiff.py` | 284 | 79 (31%) | 2026-09-28 / 5 | own file | agents 1, skills 10, claude.md 1, docs 21, tools 20 | CORE-EVIDENCE | Side-by-side instruction diff of one symbol, or every symbol's official score for a unit; refuses a stale prebuilt object. |
| `symdiff_selftest.py` | 197 | 24 (14%) | 2026-09-28 / 3 | (is one) | tools 1 | CORE-EVIDENCE | Unique tmpdir per run + stale refusal. |
| `unitscore.py` | 487 | 99 (22%) | 2026-09-28 / 4 | own file + --selftest | agents 6, skills 3, docs 9, tools 16 | CORE-EVIDENCE | Every symbol of a unit from one report read (or one `--measure` call), with a freshness verdict that refuses a stale report. |
| `unitscore_selftest.py` | 381 | 55 (16%) | 2026-09-28 / 3 | (is one) | tools 2 | CORE-EVIDENCE | Fake-repo fixture outside the tree; refusal of stale report/object; zero vs one objdiff call. |

Inputs -> outputs, CLI shape, overlap family:

* `freshguard.py` - paths, mtimes -> reasons; CLI `(library)`; overlap: score readers; imports: -
* `freshguard_selftest.py` - -; CLI `python <file>`; imports: freshguard
* `metric_selftest.py` - -; CLI `python <file>`; overlap: score readers; imports: mwcc_matrix, tryvar, unitutil
* `pairgap.py` - obj/ + src/ objects, report.json -> rows; CLI `pairgap.py [-u U] [--summary] [--mode] [--threshold] [--json] [--selftest]`; overlap: object comparison; imports: unitutil
* `pairgap_selftest.py` - -; CLI `python <file>`; imports: pairgap
* `relocdiff.py` - obj/ + src/ objects -> tables, exit; CLI `relocdiff.py <unit>.. [--section] [--rows] [--json] [--check] [--by-owner] [--selftest]`; overlap: object comparison; imports: freshguard, dossier, unitutil
* `relocdiff_selftest.py` - -; CLI `python <file>`; imports: relocdiff
* `slotmap.py` - diff.json -> table; CLI `slotmap.py -u U <sym> [--map] [--slot] | <diff.json> <sym>`; overlap: score readers; imports: unitutil
* `symdiff.py` - objects -> diff text; CLI `symdiff.py -u U [sym] [n] [--all] [--force-stale] | <diff.json> <sym>`; overlap: score readers; imports: freshguard, unitutil
* `symdiff_selftest.py` - -; CLI `python <file>`; imports: unitutil
* `unitscore.py` - report.json or objects -> table/JSON; CLI `unitscore.py <unit> [--measure] [--threshold] [--json] [--force-stale] [--selftest]`; overlap: score readers; imports: symdiff, verifyunit, unitutil
* `unitscore_selftest.py` - -; CLI `python <file>`; imports: -

## `tools/rso/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `inventory.py` | 154 | 32 (24%) | 2026-09-20 / 1 | none | docs 3, tools 1 | DEAD | Parse RSO module headers, sections and export/import tables (dormant: the RSO splitter blocker, docs/rso-modules.md). |
| `symbols.py` | 198 | 27 (15%) | 2026-09-20 / 1 | none | docs 1, tools 1 | DEAD | Seed a per-module `symbols.txt` from an RSO export table (dormant, same blocker). |

Inputs -> outputs, CLI shape, overlap family:

* `inventory.py` - orig/**/*.rso -> listing/JSON; CLI `inventory.py [--rso F] [--sections] [--symbols] [--json]`; overlap: rso (dormant); imports: unitutil
* `symbols.py` - rso -> config/RMHE08/<mod>/symbols.txt; CLI `symbols.py --all [--out] | --rso F --print`; overlap: rso (dormant); imports: inventory, unitutil

## `tools/selftest_site/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `sitecustomize.py` | 16 | 6 (40%) | 2026-09-30 / 1 | none | selftest 2, docs 1, tools 1 | CORE-GATE | PYTHONPATH shim the runner prepends so every selftest process installs spawnretry. |

Inputs -> outputs, CLI shape, overlap family:

* `sitecustomize.py` - -; CLI `-`; overlap: test harness; imports: spawnretry

## `tools/splits/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `applysplits.py` | 1899 | 138 (8%) | 2026-10-01 / 4 | --selftest | docs 20 | SPLITS-PROGRAM | Phase 4 of the splits program: plan/apply the reconciled candidate into `splits.txt` per window, write the lane manifest, verify a landed window. |
| `dataattach.py` | 2377 | 214 (10%) | 2026-10-01 / 4 | --selftest | docs 45, tools 2 | SPLITS-PROGRAM | Phase 2 engine: decide which candidate unit owns each unowned data symbol by reader evidence + link-order DP, with settling, holdout and explain modes. |
| `dataorder.py` | 561 | 81 (17%) | 2026-10-01 / 6 | --selftest | agents 2, skills 6, claude.md 1, docs 13, tools 21 | CORE-EVIDENCE | Classify retail `.data` symbols and list the TU seams (V->S, zigzag) MWCC's emission order implies; the reusable core five tools import. |
| `dump_asm.py` | 114 | 27 (27%) | 2026-09-30 / 2 | own file | skills 2, claude.md 1, docs 11, tools 24 | CORE-EVIDENCE | Regenerate dtk's per-unit asm dump on demand (write_asm is off) and stamp it with the input hashes. |
| `dump_asm_selftest.py` | 139 | 15 (14%) | 2026-09-30 / 2 | (is one) | none | CORE-EVIDENCE | Temp-config rewrite and the asm stamp state machine. |
| `gen_trk_vectors.py` | 119 | 29 (28%) | 2026-09-28 / 1 | none | docs 1 | DEAD | One-shot generator of the two TRK interrupt-vector units (claimed and matched 2026-09-28). |
| `matchinggain.py` | 139 | 20 (17%) | 2026-10-01 / 1 | --selftest | docs 6, tools 2 | SPLITS-PROGRAM | The `Matching` units whose data the splits candidate changes (phase 4 demotion list). |
| `splitcheck.py` | 3578 | 296 (9%) | 2026-10-01 / 6 | --selftest | docs 34, tools 6 | SPLITS-PROGRAM | Read-only checker of a `splits.txt`: 11 invariants per unit (`--baseline` is the phase-5 audit); renders and lints proposal files; owns the retail-text reference scanner `Ctx`. |
| `tudiscover.py` | 2168 | 361 (18%) | 2026-09-30 / 12 | --selftest | configure 6, skills 17, claude.md 2, docs 27, tools 20 | CORE-EVIDENCE | Propose a TU boundary around an address from the asm dump's referrer runs, pool model, data order and `__FILE__` anchors; owns the graph cache and the asm stamp. |

Inputs -> outputs, CLI shape, overlap family:

* `applysplits.py` - proposals, splits.txt, configure.py -> splits.txt, manifests; CLI `applysplits.py {plan,apply,manifest,verify,all,freeze} --window W [--json] [--md] [--dtk] [--selftest]`; overlap: splits pipeline; imports: dataattach, matchinggain, splitcheck, verifyunit, unitutil
* `dataattach.py` - proposals + DOL + symbols -> phase2 proposal JSON; CLI `dataattach.py [--window] [--out] [--explain] [--holdout] [--vtable-order] [--selftest]`; overlap: splits pipeline; decoder; imports: dataorder, splitcheck
* `dataorder.py` - DOL + symbols.txt -> seams; CLI `dataorder.py {scan,at} [--json] [--selftest]`; overlap: seam evidence; imports: tudiscover, dataseams, unitutil
* `dump_asm.py` - DOL, map -> build/RMHE08/asm/; CLI `dump_asm.py [--check] [--dry-run]`; overlap: seam evidence; imports: tudiscover
* `dump_asm_selftest.py` - -; CLI `python <file>`; imports: dump_asm, tudiscover
* `gen_trk_vectors.py` - target .o -> two .c files; CLI `gen_trk_vectors.py <target.o> <out_dir>`; imports: -
* `matchinggain.py` - configure.py + candidate -> table; CLI `matchinggain.py [--proposal F..] [--json] [--selftest]`; overlap: splits pipeline; imports: dataattach, splitcheck
* `splitcheck.py` - splits.txt, symbols.txt, DOL -> PASS/FAIL/UNKNOWN table, JSON; CLI `splitcheck.py --baseline [--only] [--unit RE] [--intervals] | --proposal F.. [--emit-splits] [--readers] [--selftest]`; overlap: splits pipeline; decoder; rule rows; imports: dataorder, unitutil
* `tudiscover.py` - build/RMHE08/asm, map, DOL -> scored cuts, build/tmp/tudiscover/graph.json; CLI `tudiscover.py [at ADDR] | stats | cache | bench | dataorder | prune [--json] [--selftest]`; overlap: seam evidence; imports: dataorder, poolseams, unitutil

## `tools/symbols/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `dumpmap.py` | 633 | 71 (12%) | 2026-09-23 / 1 | --selftest | configure 9, agents 2, skills 4, docs 19, tools 1 | CORE-EVIDENCE | Resolve map names against the runtime dump's Dolphin symbol map: `lookup` one address, `join` for rename/confirm/conflict rows. |
| `phantom.py` | 839 | 91 (13%) | 2026-09-28 / 2 | --selftest | skills 1, docs 5, tools 3 | CORE-EVIDENCE | Find phantom `fn_*` rows (dead epilogues) with reachability evidence from the linked DOL; prints merge plans for symedit merge-batch. |
| `symedit.py` | 1194 | 155 (14%) | 2026-09-29 / 9 | --selftest | configure 1, agents 13, skills 22, claude.md 5, docs 26, tools 42 | CORE-EVIDENCE | Query and surgically edit `symbols.txt` without loading it: find/show/at/range/refs/check, rename(-batch), merge-batch; the map parser other tools import. |

Inputs -> outputs, CLI shape, overlap family:

* `dumpmap.py` - symbols.txt + DumpSymbols.zip -> rows; CLI `dumpmap.py {lookup,join} [--json] [--kind] [--limit] [--selftest]`; overlap: map editing; imports: symedit
* `phantom.py` - DOL, map, dump -> records; CLI `phantom.py [--all] [--json] [--limit] [--max-size] [--selftest]`; overlap: map editing; decoder; imports: symbols, symedit, m2cinput
* `symedit.py` - symbols.txt -> rows; rename edits; CLI `symedit.py {find,show,at,range,refs,check,rename,rename-batch,merge-batch} [--file] [--json] [--limit] [--dry-run]`; overlap: map editing; map parser; imports: sharedfiles, unitutil

## `tools/units/`

| file | lines | prose | last / commits | selftest | callers | verdict | purpose |
| --- | ---: | ---: | --- | --- | --- | --- | --- |
| `accessextent.py` | 2064 | 405 (21%) | 2026-09-30 / 3 | own file + --selftest | tools 3 | CORE-EVIDENCE | How far anything reads a data block: decode every reference site (callers census) and abstractly interpret offsets and loop walks to bound the extent. |
| `accessextent_selftest.py` | 30 | 11 (48%) | 2026-09-28 / 1 | (is one) | none | CORE-EVIDENCE | Delegates to `accessextent.py --selftest`. |
| `attribute.py` | 1361 | 418 (34%) | 2026-09-30 / 14 | own file + --selftest | selftest 3, configure 1, skills 3, claude.md 1, docs 36, tools 19 | SPLITS-PROGRAM | Pre-program bulk tiler: partition unclaimed `.text` into proposals via tudiscover and write `attribution-queue.json` (the queue `queue.py next` hands out). |
| `attribute_selftest.py` | 907 | 82 (10%) | 2026-09-30 / 14 | (is one) | selftest 2, docs 4, tools 1 | SPLITS-PROGRAM | Partitioner, cap arithmetic and transactional apply against a fixture layout. |
| `attribution-queue.json` | 40767 | 0 (0%) | 2026-09-29 / 8 | none | configure 1, skills 3, docs 2, tools 5 | SPLITS-PROGRAM | The proposal queue attribute.py writes and queue.py/brief.py read (regenerable cache keyed by symbols_sha1). |
| `backlog.py` | 2036 | 483 (27%) | 2026-09-30 / 15 | own file + --selftest | agents 2, skills 1, claude.md 1, docs 16, tools 11 | AGENT-PLUMBING | The ranked register of everything filed and not done (outbox config_requests, tooling register, lint/undefrefs/dataclaim debt) with the credit ledger `queue.py next` spends; `triage` proves items resolved/stale. |
| `backlog_selftest.py` | 976 | 75 (8%) | 2026-09-30 / 12 | (is one) | tools 1 | AGENT-PLUMBING | Fixture outboxes: dedupe, defaults, status survival, refusal, triage classes. |
| `brief.py` | 2479 | 416 (18%) | 2026-09-30 / 52 | --selftest | gate 5, docs 21, tools 63 | AGENT-PLUMBING | Render the one file a worker is handed (unit, inventory, residuals, decided, task, rules) for a registered unit or a proposal; `--pool` pre-renders every handable entry. |
| `callees.py` | 940 | 157 (18%) | 2026-09-29 / 3 | own file + --selftest | docs 5, tools 10 | CORE-EVIDENCE | Name a unit's generated callees from the target object's relocations: owner/state via the lint's Ownership, call shape from `dtk elf disasm`, cross-file references. |
| `callees_selftest.py` | 30 | 11 (48%) | 2026-09-27 / 1 | (is one) | tools 1 | CORE-EVIDENCE | Delegates to `callees.py --selftest`. |
| `callers.py` | 1848 | 303 (18%) | 2026-09-30 / 4 | own file + --selftest | gate 1, agents 6, skills 13, claude.md 2, docs 53, tools 38 | CORE-EVIDENCE | Who calls / who reads an address: the whole-DOL reference index built from the asm dump on addresses (never labels), cached; object-relocation fallback; `--range` referrer runs. |
| `callers_selftest.py` | 30 | 11 (48%) | 2026-09-28 / 1 | (is one) | tools 1 | CORE-EVIDENCE | Delegates to `callers.py --selftest`. |
| `checklf.py` | 153 | 38 (29%) | 2026-09-28 / 1 | own file + --selftest | tools 3 | DEAD | Report a working-tree file whose line-ending style differs from its index/HEAD blob (overlaps `edit.py check`). |
| `checklf_selftest.py` | 135 | 19 (17%) | 2026-09-28 / 1 | (is one) | tools 1 | DEAD | Temp-repo test of the CRLF-over-LF invisibility. |
| `claims.py` | 2847 | 627 (24%) | 2026-09-30 / 30 | --selftest | gate 4, configure 1, pi-bin 3, agents 6, claude.md 3, docs 40, tools 52 | AGENT-PLUMBING | Claim a unit for a worker (branch = lock, worktree, registry), ack/status/timeout, the one-shot idempotent `release` teardown with rescue ref, `expire`; also the worktree build seeder. |
| `data-queue.json` | 71462 | 0 (0%) | 2026-09-29 / 3 | none | docs 1, tools 8 | CORE-EVIDENCE | The unowned-data run queue dataqueue.py writes and brief/dataclaim read. |
| `dataclaim.py` | 1616 | 209 (15%) | 2026-09-30 / 9 | --selftest | gate 4, agents 5, skills 1, docs 23, tools 9 | CORE-EVIDENCE | Decide whether a proposed data run may be claimed (overlap, target bytes, what ours emits, ledger effect); `--unit U` lists rule-12 references with the exact claim to paste. |
| `datagap.py` | 2816 | 427 (17%) | 2026-09-30 / 13 | --selftest | gate 3, configure 1, agents 12, skills 11, docs 46, tools 26 | CORE-GATE | Per-unit data-section gap target vs ours (flip blockers), the data-closure census (orphans the target references that no claim covers), strict/span/fold verdicts and the gate's `--touched-by` snapshot rows. |
| `dataqueue.py` | 902 | 165 (20%) | 2026-09-29 / 7 | --selftest | docs 8, tools 8 | CORE-EVIDENCE | Write `data-queue.json`: every unowned data run with labels, leak, density and a pre-measurement verdict; `--request` files a data request. |
| `dataseams.py` | 334 | 60 (21%) | 2026-09-29 / 2 | --selftest | skills 1, docs 3, tools 5 | CORE-EVIDENCE | Thin consumer layer over dataorder's strong seams: cut points, warnings and order-only notes for the claim tools. |
| `declclash.py` | 230 | 38 (19%) | 2026-09-26 / 1 | own file + --selftest | agents 1, claude.md 1, docs 1, tools 3 | CORE-EVIDENCE | List names declared more than once with different text in one file's include closure (the `illegal function overloading` list). |
| `declclash_selftest.py` | 137 | 12 (10%) | 2026-09-26 / 1 | (is one) | tools 1 | CORE-EVIDENCE | Hand-built include web fixture. |
| `dossier.py` | 937 | 148 (18%) | 2026-09-23 / 1 | own file + --selftest | tools 15 | CORE-EVIDENCE | One page of what the split target object already knows about a unit: `__FILE__` strings, mangled names, panic lines, literals, references, jump tables, layout, blanks. |
| `dossier_selftest.py` | 33 | 14 (54%) | 2026-09-23 / 1 | (is one) | selftest 1 | CORE-EVIDENCE | Delegates to `dossier.py --selftest`. |
| `escape.py` | 292 | 41 (16%) | 2026-09-28 / 1 | --selftest | agents 12, tools 1 | AGENT-PLUMBING | Write exact bytes / C-escaped text from one quoted argument without a heredoc; `--edit` is a count-asserted byte replace. |
| `flipcheck.py` | 844 | 215 (28%) | 2026-09-30 / 12 | own file + --selftest | gate 4, agents 8, skills 31, docs 51, tools 22 | CORE-GATE | Can our object fill every section the unit's `splits.txt` claims (size, bytes, permutation class, undefined references, `.comment` trim risks)? The gate's READY row. |
| `flipcheck_selftest.py` | 552 | 89 (18%) | 2026-09-30 / 8 | (is one) | selftest 1, tools 1 | CORE-GATE | ELF fixtures for the comment, extab map-symbol, byte/permutation and undefined-reference checks. |
| `handoff.py` | 452 | 79 (19%) | 2026-09-28 / 6 | --selftest | gate 1, docs 11, tools 10 | CORE-GATE | The outbox contract: print the skeleton/template and validate an outbox entry (owned symbols, percents, measured_with, config_requests schema). |
| `land.py` | 5172 | 1311 (28%) | 2026-09-30 / 77 | --selftest | selftest 1, pi-bin 2, agents 22, claude.md 5, docs 72, tools 66 | CORE-GATE | The land gate: `record-base`, `verify` (the ~30 PASS/FAIL rows), `land` (apply/stage/commit/release) and `resolve` for a branch. |
| `lane.py` | 385 | 77 (22%) | 2026-09-30 / 5 | --selftest | none | AGENT-PLUMBING | Teardown for a non-claim lane (`experiment/*`): rescue unlanded commits to `refs/rescue/<slug>`, then remove worktree and branch; idempotent. |
| `lanecmd.py` | 161 | 31 (22%) | 2026-09-29 / 1 | --selftest | claude.md 1, docs 5, tools 4 | AGENT-PLUMBING | The one builder of a lane's `claude --agent .. -p ..` launch line and the resume line. |
| `langcheck.py` | 1188 | 265 (24%) | 2026-09-24 / 4 | --selftest | gate 14, skills 3, docs 4, tools 11 | CORE-GATE | Decide a unit's language (C/C++) from evidence: mangled definitions, `.cpp` `__FILE__` strings, extab presence under no-exceptions libs; the gate and brief read it. |
| `ledger.py` | 554 | 63 (13%) | 2026-09-23 / 3 | own file + --selftest | gate 2, selftest 1, agents 1, claude.md 2, docs 16, tools 12 | CORE-GATE | Campaign progress from the repository: covered/registered/closed totals, `next` unclaimed symbols, one unit's rows; `Objects` maps addresses to split objects. |
| `ledger_selftest.py` | 190 | 18 (11%) | 2026-09-21 / 1 | (is one) | selftest 1, docs 2, tools 1 | CORE-GATE | Fixture text tests of coverage, closure, `next` and staleness (complementary to `ledger.py --selftest`). |
| `linkorder.py` | 758 | 78 (12%) | 2026-09-23 / 2 | own file | skills 3, docs 4, tools 2 | CORE-EVIDENCE | Reconstruct the DOL image from `main.elf` and compare it to the original slot by slot (order, bytes, header), attributing the first divergence to a unit/object. |
| `linkorder_selftest.py` | 319 | 28 (10%) | 2026-09-23 / 2 | (is one) | none | CORE-EVIDENCE | Hand-built ELF + DOL fixtures. |
| `m2cinput.py` | 727 | 179 (28%) | 2026-09-28 / 3 | own file | skills 4, claude.md 3, docs 5, tools 3 | CORE-EVIDENCE | Turn a target object's objdump into the GNU-as shape m2c accepts (relocation spelling, `loc_` labels, tail calls, `@` names, jump tables). |
| `m2cinput_selftest.py` | 388 | 43 (13%) | 2026-09-26 / 2 | (is one) | docs 1 | CORE-EVIDENCE | Real objdump text through the pure rewrites. |
| `mangle.py` | 316 | 73 (26%) | 2026-09-29 / 3 | --selftest | skills 8, docs 8, tools 6 | CORE-EVIDENCE | Derive the mangled name MWCC emits for a C++ declaration (compile a stub) or estimate it textually; prints the symedit rename. |
| `measure.py` | 559 | 91 (18%) | 2026-09-28 / 4 | own file | selftest 1, docs 28, tools 23 | CORE-EVIDENCE | Score a whole unit in one compile and one `report generate`, with per-symbol deltas against the last run, a saved baseline or MAIN's report. |
| `measure_selftest.py` | 531 | 64 (13%) | 2026-09-28 / 3 | (is one) | gate 2, selftest 1, docs 4, tools 1 | CORE-EVIDENCE | Fake-runner batching (one compile, one report), target resolution order, parsing, cache, baseline, integration. |
| `mergebranch.py` | 1583 | 292 (20%) | 2026-09-29 / 6 | --selftest | agents 2, claude.md 1, docs 8, tools 4 | AGENT-PLUMBING | Bring `main` into a held branch and resolve conflicts by class (map rows, splits/configure union, header superset, add/add by rename base), prove, commit. |
| `methodize.py` | 635 | 72 (12%) | 2026-09-29 / 3 | --selftest | gate 1, agents 6, skills 3, docs 4, tools 7 | CORE-EVIDENCE | Rule-13 planner: `Type_name(Type*)` free functions -> members, with declaration/definition/call-site edits, estimated or verified mangling, and a symedit rename-batch. |
| `playbook.py` | 731 | 80 (12%) | 2026-09-29 / 6 | own file + --selftest | gate 3, tools 5 | CORE-GATE | Turn outbox findings into playbook idea drafts under `.pi/playbook-drafts/` (the gate's knowledge-delta row calls it). |
| `playbook_selftest.py` | 264 | 19 (8%) | 2026-09-29 / 3 | (is one) | tools 1 | CORE-GATE | Fixture outboxes: classification, grouping, refusal, no docs write. |
| `poolseams.py` | 623 | 111 (20%) | 2026-09-30 / 1 | --selftest | skills 3, docs 6, tools 9 | CORE-EVIDENCE | The literal pool as TU evidence: groups of registered units that share a pool entry (fold candidates), from datagap's census. |
| `preflight_selftest.py` | 510 | 90 (20%) | 2026-10-01 / 6 | (is one) | none | CORE-EVIDENCE | Selftest for symbolpreflight with rows derived from the live tree by a private reader plus two fixed rows. |
| `promote.py` | 1223 | 227 (20%) | 2026-09-28 / 5 | own file + --selftest | tools 13 | DEAD | Move a `src/auto` unit to its final name/location across map, source, configure.py, splits.txt (the auto bucket is retired; nothing calls it but promote_batch). |
| `promote_batch.py` | 514 | 72 (15%) | 2026-09-24 / 1 | own file + --selftest | tools 5 | DEAD | Batch of promotions / language promotions with one re-split (same retirement). |
| `promote_batch_selftest.py` | 288 | 33 (13%) | 2026-09-27 / 2 | (is one) | tools 1 | DEAD | Spec, sequential apply, batch check on the promote fixture. |
| `promote_selftest.py` | 592 | 45 (8%) | 2026-09-23 / 2 | (is one) | tools 2 | DEAD | Plan, four-file edit and byte comparator on a fixture tree. |
| `queue.py` | 1569 | 324 (23%) | 2026-09-30 / 28 | own file + --selftest | gate 1, claude.md 11, docs 29, tools 38 | AGENT-PLUMBING | Hand the next pooled unit/proposal (or backlog debt) to a worker: claim, re-render the brief, print the spawn line; `--count N` strides a wave; refuses while unlanded work exists. |
| `queue_selftest.py` | 35 | 17 (61%) | 2026-09-25 / 2 | (is one) | none | AGENT-PLUMBING | Delegates to `queue.py --selftest`. |
| `recompile.py` | 1248 | 431 (39%) | 2026-09-29 / 13 | own file + --selftest | gate 5, selftest 1, pi-bin 1, agents 8, skills 1, docs 133, tools 45 | CORE-GATE | Compile one unit without ninja from any worktree with MAIN's real command line (worktree includes first), prove the object fresh, `--measure` one symbol with the official metric; resolves proposal targets. |
| `recompile_selftest.py` | 1214 | 211 (20%) | 2026-09-29 / 13 | (is one) | tools 1 | CORE-GATE | Wire test, include order, chained objalign, proposal targets, staleness, provenance, CLI. |
| `recordmerge.py` | 549 | 76 (16%) | 2026-09-28 / 2 | own file + --selftest | skills 3, claude.md 1, docs 6, tools 3 | AGENT-PLUMBING | Merge two views of one record header (splice into filler, per-struct keys, declaration compare) and refuse while unresolved. |
| `recordmerge_selftest.py` | 245 | 22 (10%) | 2026-09-26 / 1 | (is one) | tools 1 | AGENT-PLUMBING | One fixture per rule and per refusal. |
| `relocaudit-findings.md` | 220 | 0 (0%) | 2026-09-25 / 1 | none | none | DEAD | A dated snapshot of one relocaudit sweep (history, not spec). |
| `relocaudit.py` | 396 | 128 (36%) | 2026-09-25 / 1 | own file + --selftest | tools 5 | CORE-EVIDENCE | Sweep every registered unit's undefined/defined symbol sets against the target's for wrong-linkage spellings (overlaps undefrefs' per-unit check). |
| `relocaudit_selftest.py` | 253 | 52 (23%) | 2026-09-25 / 1 | (is one) | tools 1 | CORE-EVIDENCE | Linkage stem, classifier, ELF reader over a scratch tree. |
| `reportdiff.py` | 631 | 76 (13%) | 2026-09-29 / 1 | --selftest | none | CORE-EVIDENCE | Diff two `report.json` snapshots: moved unit/symbol rows, units added/removed, category denominators; exit status is the verdict (no caller today). |
| `rescue.py` | 541 | 79 (17%) | 2026-09-28 / 2 | --selftest | docs 7, tools 6 | AGENT-PLUMBING | Audit `refs/rescue/*`: derive the units each ref registers, classify redundant/landed-with-drift/unlanded/unknown, prune only redundant. |
| `sectiongap.py` | 317 | 90 (32%) | 2026-09-30 / 2 | own file + --selftest | skills 1, docs 3, tools 5 | CORE-EVIDENCE | Per-section gap of a unit's two objects: sizes, first differing byte, count and the relocation list (the extab/extabindex rows datagap skips). |
| `sectiongap_selftest.py` | 245 | 32 (15%) | 2026-09-30 / 3 | (is one) | tools 3 | CORE-EVIDENCE | ELF32 fixtures (its `build_elf` is borrowed by relocdiff's tests). |
| `sharedfiles.py` | 369 | 78 (25%) | 2026-09-23 / 1 | --selftest | docs 2, tools 11 | CORE-EVIDENCE | One owner of writes to shared files: ending-preserving text, asserted anchors, idempotent block append, overlap refusal, temp+replace Transaction. |
| `slots.py` | 3384 | 806 (26%) | 2026-09-30 / 15 | --selftest | agents 4, claude.md 4, docs 16, tools 12 | AGENT-PLUMBING | The fixed pool of reusable lane directories: init/acquire/spawn/release/reclaim/status/verify/shadow/collect, with the `.used` owner sentinel and live-session detection. |
| `stylelint.py` | 4924 | 1084 (24%) | 2026-10-01 / 29 | --selftest | gate 4, pi-bin 2, agents 18, skills 2, claude.md 3, docs 24, tools 28 | CORE-GATE | Lint `src/`+`include/` against section 6.5 rules 1-13 with `file:line`; `--diff REF` is the gate's add-only comparison with rename/move credits; `--ref` judges a branch; owns the `Ownership` index. |
| `subproc.py` | 235 | 63 (30%) | 2026-09-29 / 3 | own file + --selftest | gate 2, tools 2 | CORE-GATE | The codec rule for subprocess text (`encoding=utf-8, errors=replace`), `run()`, and the AST scan that finds violators. |
| `subproc_selftest.py` | 17 | 7 (54%) | 2026-09-28 / 1 | (is one) | none | CORE-GATE | Runs `subproc.selftest()`. |
| `symbolpreflight.py` | 412 | 33 (9%) | 2026-09-21 / 1 | none | skills 2, claude.md 2, docs 6, tools 10 | CORE-EVIDENCE | Pre-flight one symbol: boundary, owner range, configured lib/flags, collision kind/severity, draft splits/configure blocks. |
| `tooling.py` | 989 | 116 (13%) | 2026-09-28 / 2 | own file + --selftest | agents 2, docs 7, tools 10 | AGENT-PLUMBING | Cluster the reports' tooling/environment requests into the ranked `docs/tooling-requests.md`. |
| `tooling_selftest.py` | 236 | 33 (16%) | 2026-09-28 / 2 | (is one) | tools 1 | AGENT-PLUMBING | Fixture outboxes/notes: structured rows, clustering, votes, status survival. |
| `typeregistry.py` | 985 | 152 (17%) | 2026-09-29 / 2 | own file + --selftest | tools 5 | CORE-EVIDENCE | Registry of shared types/helpers under include/ and src/: where defined, who uses, duplicated debt; `relevant_headers` feeds the brief. |
| `typeregistry_selftest.py` | 33 | 15 (58%) | 2026-09-23 / 1 | (is one) | none | CORE-EVIDENCE | Delegates to `typeregistry.py --selftest`. |
| `undefrefs.py` | 743 | 169 (25%) | 2026-09-29 / 3 | own file + --selftest | gate 2, tools 14 | CORE-GATE | The gate's add-only row: a relocation our object carries that no link input can define, with the target's spelling hint; base snapshot cached per source hash; `--census`. |
| `undefrefs_selftest.py` | 420 | 59 (16%) | 2026-09-29 / 4 | (is one) | tools 3 | CORE-GATE | ELF fixtures for both incident shapes, the negative Pat shape, the clean cases and the cached index. |
| `unionguard.py` | 608 | 93 (17%) | 2026-09-29 / 4 | own file + --selftest | gate 1, pi-bin 3, tools 3 | CORE-GATE | Refuse a union of a conflicted apply unless every conflict is a disjoint addition (empty diff3 base, no delete/rename); undoes the apply on refusal. |
| `unionguard_selftest.py` | 34 | 15 (56%) | 2026-09-25 / 2 | (is one) | none | CORE-GATE | Real git repos with real unmerged indexes per case. |
| `unionprose.py` | 416 | 98 (26%) | 2026-09-29 / 1 | --selftest | tools 2 | CORE-GATE | The one union rule: code hunks union, prose hunks take the superset, mixed warns; shared by mergebranch and unionresolve. |
| `unionresolve.py` | 434 | 113 (29%) | 2026-09-29 / 2 | own file + --selftest | gate 1, docs 1, tools 5 | CORE-GATE | The landing path's append-union of splits/configure conflicts plus the four invariant assertions (`check_union`). |
| `unionresolve_selftest.py` | 25 | 11 (55%) | 2026-09-26 / 1 | (is one) | none | CORE-GATE | Runs `unionresolve.selftest()`. |
| `unwindcut.py` | 695 | 104 (17%) | 2026-09-29 / 1 | own file + --selftest | docs 2, tools 3 | CORE-EVIDENCE | Re-cut one unit at a function boundary: partition `.text`/extab/extabindex from the retail image, the `.ctors`/`.dtors` words to drop, paste-ready lines; refuses a non-boundary cut. |
| `unwindcut_selftest.py` | 295 | 41 (16%) | 2026-09-29 / 1 | (is one) | tools 2 | CORE-EVIDENCE | Synthetic DOL/splits/symbols fixtures. |
| `verifyunit.py` | 670 | 258 (43%) | 2026-09-28 / 6 | own file | gate 2, agents 2, skills 1, docs 3, tools 8 | CORE-GATE | Independent verification for the gate: registration completeness (3 axes), per-symbol re-measure that does not read the report it audits, size-gap rows, split-target drift. |
| `verifyunit_selftest.py` | 557 | 79 (16%) | 2026-09-30 / 3 | (is one) | tools 1 | CORE-GATE | Fixtures that must refuse per check; objdiff-dependent rows skipped without it. |
| `vtableaudit.py` | 1380 | 368 (30%) | 2026-09-29 / 6 | own file + --selftest | gate 3, agents 14, skills 2, claude.md 1, docs 12, tools 7 | CORE-GATE | Rule 10 audit: code-pointer runs a unit owns but neither emits nor references, `+0x00` table stores in source, section completeness; `--diff REF` is the gate's add-only row; `--at` dumps slots. |
| `vtableaudit_selftest.py` | 728 | 97 (15%) | 2026-09-29 / 5 | (is one) | tools 1 | CORE-GATE | Run rule, address resolution, verdicts, section comparison; the fixture tree is hashed before/after. |
| `vtslot.py` | 549 | 138 (27%) | 2026-09-29 / 1 | own file + --selftest | tools 4 | CORE-EVIDENCE | Reverse of `vtableaudit --at`: every DOL data word equal to an address, its owning range, the code-pointer run and RTTI base around it. |
| `vtslot_selftest.py` | 302 | 40 (15%) | 2026-09-29 / 1 | (is one) | tools 1 | CORE-EVIDENCE | Hand-built DOL/splits/symbols fixtures; CLI leaves the fixture byte-identical. |
| `worktreehook.py` | 629 | 86 (16%) | 2026-09-29 / 4 | --selftest | claude.md 3, docs 1 | AGENT-PLUMBING | PROTOTYPE: Claude Code WorktreeCreate/Remove hooks that hand out a slot against an arm token. |
| `wtsafe.py` | 290 | 67 (27%) | 2026-09-28 / 3 | --selftest | gate 2, tools 2 | AGENT-PLUMBING | Remove a worktree after unlinking its junctions (never walk a reparse point) and verify `orig/` against the pinned hashes. |

Inputs -> outputs, CLI shape, overlap family:

* `accessextent.py` - callers index, main.elf, objdump -> verdict; CLI `accessextent.py <addr|name> [--json] [--limit] [--min-offset] [--accessor] [--block] [--rebuild] [--selftest]`; overlap: reference census; decoder; imports: callees, callers
* `accessextent_selftest.py` - -; CLI `python <file>`; imports: accessextent
* `attribute.py` - tudiscover analysis, splits.txt -> attribution-queue.json; CLI `attribute.py {plan,queue,dataseams} START END [--json] [--max-total-bytes] [--replace-region] [--selftest]`; overlap: splits pipeline; proposal queue; imports: dataorder, tudiscover, langcheck, poolseams, sharedfiles
* `attribute_selftest.py` - -; CLI `python <file>`; imports: dataorder, attribute
* `attribution-queue.json` - -; CLI `(data)`; imports: -
* `backlog.py` - .pi/outbox, .pi/notes, docs/tooling-requests.md, lint -> .pi/backlog.json; CLI `backlog.py [triage [--apply]] [--set-status] [--json] [--top] [--selftest]`; overlap: outbox consumers; imports: datagap, handoff, lanecmd, stylelint, tooling, undefrefs
* `backlog_selftest.py` - -; CLI `python <file>`; imports: backlog, datagap
* `brief.py` - map, splits, report, outbox, dossier, typeregistry, plan.md -> tools/units/briefs/<slug>.md; CLI `brief.py <unit> [--task] [--stdout] [--json] | --pool [--force] | --check-promoted [--selftest]`; overlap: lane lifecycle; imports: claims, dossier, handoff, langcheck, recompile, typeregistry, unitutil
* `callees.py` - target .o, map, splits, src -> table; CLI `callees.py <unit> [--json] [--limit] [--no-shape] [--no-scan] [--selftest]`; overlap: reference census; decoder; imports: symedit, dossier, stylelint, unitutil
* `callees_selftest.py` - -; CLI `python <file>`; imports: callees
* `callers.py` - build/RMHE08/asm or obj/ -> build/tmp/callers/graph.json -> report; CLI `callers.py <addr|name> [--code|--data] [--kind] [--pointers] [--range LO HI] [--stats] [--rebuild] [--json] [--selftest]`; overlap: reference census; decoder; cache; imports: tudiscover, callees, dossier, stylelint, unitutil
* `callers_selftest.py` - -; CLI `python <file>`; imports: callers
* `checklf.py` - git blobs -> exit 0/1/2; CLI `checklf.py [paths] [--rev] [--selftest]`; overlap: line endings; imports: -
* `checklf_selftest.py` - -; CLI `python <file>`; imports: -
* `claims.py` - git, .pi/claims.json, slots -> worktree/branch; CLI `claims.py {claim,list,release,ack,status,timeout,expire} [--json] [--dry-run] [--selftest]`; overlap: lane lifecycle; imports: queue, recompile, rescue, slots, wtsafe, unitutil
* `data-queue.json` - -; CLI `(data)`; imports: -
* `dataclaim.py` - data-queue.json, obj/src objects, DOL, report, callers census -> verdicts; CLI `dataclaim.py [--risky N] [--queue-unit U] [--out F] | --unit U [--fixpoint] [--json] [--selftest]`; overlap: data closure; imports: elfsect, callers, datagap, dataqueue, dataseams, ledger, stylelint, symbolpreflight, undefrefs
* `datagap.py` - obj/src objects, map, splits, report -> rows, snapshots; CLI `datagap.py [--flip-blockers] [--mode] [--unit U] [--census] [--pool-seams] [--touched-by] [--write-snapshot] [--base-snapshot] [--json] [--selftest]`; overlap: object comparison; data closure; rule rows; imports: symedit, callers, dataseams, dossier, poolseams, undefrefs
* `dataqueue.py` - map, splits, config.json, tudiscover cache, seams -> data-queue.json, .pi/data-requests.json; CLI `dataqueue.py [--dry-run] [--out] [--request ..] [--json] [--selftest]`; overlap: data closure; imports: brief, dataseams, ledger, sharedfiles, symbolpreflight
* `dataseams.py` - dataorder -> seams; CLI `dataseams.py [START END] [--selftest]`; overlap: seam evidence; imports: dataorder, tudiscover
* `declclash.py` - src + include -> SAME/DIFFERENT list; CLI `declclash.py <file>.. [--only-different] [--fail-on-different] [--json] [--selftest]`; overlap: source scanners; imports: -
* `declclash_selftest.py` - -; CLI `python <file>`; imports: declclash
* `dossier.py` - target .o, DOL, map, dump -> page/JSON; CLI `dossier.py <unit> [--json] [--out] [--no-dump] [--selftest]`; overlap: reference census; binary readers; imports: brief, recompile
* `dossier_selftest.py` - -; CLI `python <file>`; imports: dossier
* `escape.py` - arg -> file; CLI `escape.py --escape S | --bytes S | --write F S [--append] | --edit F --old --new [--count] [--selftest]`; overlap: line endings; imports: -
* `flipcheck.py` - src/obj objects, splits, map, link inputs -> reasons, exit; CLI `flipcheck.py [unit..] [--selftest]`; overlap: object comparison; rule rows; imports: claims, dataseams, poolseams
* `flipcheck_selftest.py` - -; CLI `python <file>`; imports: flipcheck
* `handoff.py` - unit, outbox JSON -> verdict; CLI `handoff.py <unit> [--template] [--check F] [--json] [--selftest]`; overlap: outbox consumers; imports: brief, claims, recompile
* `land.py` - main tree, branch, .pi/land-base.json -> rows, commit; CLI `land.py {record-base,verify,land,resolve} [--units ..] [--branch B] [--base] [--message] [--allow-*] [--no-*] [--json] [--selftest]`; overlap: landing; rule rows; imports: prepcommit, brief, claims, datagap, handoff, recompile, stylelint, subproc, undefrefs, unionguard, unionresolve, verifyunit, vtableaudit, unitutil
* `lane.py` - git -> refs/rescue; CLI `lane.py {list,teardown B} [--dry-run] [--force] [--selftest]`; overlap: lane lifecycle; imports: wtsafe, unitutil
* `lanecmd.py` - agent, cwd, task -> shell text; CLI `(library; lanecmd.py --selftest)`; overlap: lane lifecycle; imports: -
* `langcheck.py` - target .o, DOL, map, configure.py -> verdicts; CLI `langcheck.py [--unit U] [--disagree] [--json] [--selftest]`; overlap: binary readers; imports: tudiscover
* `ledger.py` - splits, configure.py, config.json, report.json -> totals; CLI `ledger.py [--json] | next [N] | unit U [--selftest]`; overlap: score readers; imports: symbolpreflight
* `ledger_selftest.py` - -; CLI `python <file>`; imports: ledger
* `linkorder.py` - main.elf, main.dol, splits, map -> report; CLI `linkorder.py [--unit U] [--json] [--elf] [--dol] [--no-stale-check]`; overlap: binary readers; imports: symbolpreflight
* `linkorder_selftest.py` - -; CLI `python <file>`; imports: linkorder
* `m2cinput.py` - target .o -> .s; CLI `m2cinput.py <obj> [-f name..] [-o out.s] [--list] [--section] [--no-tables]`; overlap: binary readers; imports: symedit
* `m2cinput_selftest.py` - -; CLI `python <file>`; imports: m2cinput
* `mangle.py` - declaration -> mangled name; CLI `mangle.py 'decl' [--unit U] [--file F] [--json] [--selftest]`; overlap: map editing; imports: unitutil
* `measure.py` - unit source -> table/JSON, build/tmp/measure; CLI `measure.py <unit> [symbol] [--baseline F|--against-main] [--save] [--diff] [--json] [-q]`; overlap: scoring; imports: recompile, unitutil
* `measure_selftest.py` - -; CLI `python <file>`; imports: measure, recompile, unitutil
* `mergebranch.py` - git merge state -> resolved commit; CLI `mergebranch.py {resolve [--branch] [--dry-run] [--json], status, selftest}`; overlap: landing; imports: land, stylelint, unionprose
* `methodize.py` - src, include, map, object -> plan, batch file; CLI `methodize.py <Type> | --all | --map old=Class::m.. [--batch F] [--exact] [--unit U] [--json] [--selftest]`; overlap: source scanners; map editing; imports: mangle, stylelint, unitutil
* `playbook.py` - .pi/outbox, .pi/notes -> drafts; CLI `playbook.py [--print] [--outbox] [--notes] [--drafts] [--json] [--selftest]`; overlap: outbox consumers; imports: sync_playbook_index
* `playbook_selftest.py` - -; CLI `python <file>`; imports: playbook
* `poolseams.py` - datagap census, splits, map, DOL -> groups; CLI `poolseams.py [--unit U] [--json] [--top] [--selftest]`; overlap: seam evidence; imports: tudiscover, datagap
* `preflight_selftest.py` - -; CLI `python <file>`; overlap: real-tree pin; imports: symbolpreflight
* `promote.py` - four files -> four files; CLI `promote.py {plan,apply,check} <unit> --name --module [--lib] [--dry-run] [--selftest]`; overlap: retired flows; imports: symedit, brief, claims, sharedfiles, stylelint
* `promote_batch.py` - spec -> edits, manifest; CLI `promote_batch.py {plan,apply,check} <spec> [--selftest]`; overlap: retired flows; imports: langcheck, promote
* `promote_batch_selftest.py` - -; CLI `python <file>`; imports: promote, promote_batch
* `promote_selftest.py` - -; CLI `python <file>`; imports: promote
* `queue.py` - pool, queue JSON, backlog, claims -> claim + brief + spawn line; CLI `queue.py {next [--count] [--worker] [--kind] [--profile] [--allow-unlanded] [--ignore-backlog], list, debt} [--dry-run] [--json] [--selftest]`; overlap: lane lifecycle; proposal queue; imports: backlog, brief, claims, lanecmd, recompile
* `queue_selftest.py` - -; CLI `python <file>`; imports: queue
* `recompile.py` - MAIN build.ninja, src -> object, score; CLI `recompile.py <unit> [--measure SYM] [--source] [--json] [--dry-run] [--selftest]`; overlap: scoring; compile; imports: claims, unitutil
* `recompile_selftest.py` - -; CLI `python <file>`; imports: recompile, unitutil
* `recordmerge.py` - two headers -> merged header; CLI `recordmerge.py --base F --other REV:F [--out] [--take] [--dry-run] [--json] [--selftest]`; overlap: source scanners; imports: sharedfiles
* `recordmerge_selftest.py` - -; CLI `python <file>`; imports: recordmerge, sharedfiles
* `relocaudit-findings.md` - -; CLI `-`; imports: -
* `relocaudit.py` - src/obj objects -> rows; CLI `relocaudit.py [--unit U] [--no-decls] [--json] [--selftest]`; overlap: object comparison; imports: langcheck
* `relocaudit_selftest.py` - -; CLI `python <file>`; imports: relocaudit
* `reportdiff.py` - two report.json -> tables, exit; CLI `reportdiff.py <before> <after> [--changed-only] [--limit] [--eps] [--json] [--selftest]`; overlap: score readers; imports: -
* `rescue.py` - git refs -> report; CLI `rescue.py audit [--prune] [--ref R..] [--full-diff] [--json] [--selftest]`; overlap: lane lifecycle; imports: unionresolve, verifyunit
* `sectiongap.py` - src/obj objects -> rows; CLI `sectiongap.py --unit U [--all-sections] [--selftest]`; overlap: object comparison; imports: elfsect, poolseams, unitutil
* `sectiongap_selftest.py` - -; CLI `python <file>`; imports: sectiongap
* `sharedfiles.py` - text -> text; CLI `(library; --selftest)`; overlap: shared-file writes; imports: -
* `slots.py` - <repo>.slotN, .pi/slots, ~/.claude/sessions -> state; CLI `slots.py {init,acquire,spawn,release,reclaim,status,verify,shadow,collect} [--slot N] [--kind K] [--json] [--selftest]`; overlap: lane lifecycle; imports: brief, claims, dataqueue, lanecmd, recompile, unitutil
* `stylelint.py` - src, include, map, splits -> findings, budget; CLI `stylelint.py [--budget [--headers]] [--diff REF [--list-added]] [--ref B] [--json] [--selftest]`; overlap: rule rows; source scanners; map/splits parser; imports: datagap, mangle
* `subproc.py` - -; CLI `(library; --selftest via subproc_selftest)`; overlap: subprocess; imports: -
* `subproc_selftest.py` - -; CLI `python <file>`; imports: subproc
* `symbolpreflight.py` - map, splits, configure.py, config.json -> report; CLI `symbolpreflight.py <addr|name> [--json]`; overlap: map/splits parser; imports: symedit
* `tooling.py` - .pi/outbox, .pi/notes -> docs/tooling-requests.md; CLI `tooling.py [--check] [--print] [--set-status K S] [--json] [--selftest]`; overlap: outbox consumers; imports: -
* `tooling_selftest.py` - -; CLI `python <file>`; imports: tooling
* `typeregistry.py` - include, src, map -> report; CLI `typeregistry.py [--unit U] [--report F] [--no-map] [--json] [--selftest]`; overlap: source scanners; imports: -
* `typeregistry_selftest.py` - -; CLI `python <file>`; imports: typeregistry
* `undefrefs.py` - src/obj objects, map, link inputs -> hits; CLI `undefrefs.py [--main] [--snapshot-base U..] [--base-snapshot F] [--census] [--rebuild-index] [--json] [--selftest]`; overlap: object comparison; rule rows; cache; imports: dossier, recompile
* `undefrefs_selftest.py` - -; CLI `python <file>`; imports: undefrefs
* `unionguard.py` - index stages -> verdict, cleanup; CLI `unionguard.py --branch B [--base R] [--no-cleanup] [paths..] [--selftest]`; overlap: landing; imports: -
* `unionguard_selftest.py` - -; CLI `python <file>`; imports: unionguard
* `unionprose.py` - diff3 text -> text; CLI `(library; --selftest)`; overlap: landing; imports: -
* `unionresolve.py` - conflicted text -> text, verdict; CLI `(library; --selftest)`; overlap: landing; imports: unionprose
* `unionresolve_selftest.py` - -; CLI `python <file>`; imports: unionresolve
* `unwindcut.py` - splits, DOL, map, object -> lines; CLI `unwindcut.py <unit> <cut> [--json] [--splits] [--dol] [--symbols] [--object] [--selftest]`; overlap: binary readers; imports: elfsect, symedit
* `unwindcut_selftest.py` - -; CLI `python <file>`; imports: unwindcut
* `verifyunit.py` - configure.py, splits, build.ninja, objects, report -> problems; CLI `verifyunit.py [--main] (library for land.py)`; overlap: landing; score readers; imports: measure, unitutil
* `verifyunit_selftest.py` - -; CLI `python <file>`; imports: verifyunit
* `vtableaudit.py` - splits, map, DOL, objects, src -> rows; CLI `vtableaudit.py [--unit U] [--diff REF] [--at ADDR] [--runs] [--refs] [--sections] [--order] [--json] [--selftest]`; overlap: rule rows; binary readers; imports: elfsect, dossier, langcheck
* `vtableaudit_selftest.py` - -; CLI `python <file>`; imports: vtableaudit
* `vtslot.py` - DOL, splits, map -> hits; CLI `vtslot.py <addr>.. [--scan LO HI] [--json] [--selftest]`; overlap: binary readers; imports: claims, vtableaudit
* `vtslot_selftest.py` - -; CLI `python <file>`; imports: vtableaudit, vtslot
* `worktreehook.py` - hook JSON -> slot path; CLI `worktreehook.py {arm N [--slot] [--ttl], disarm, status, create, remove} [--selftest]`; overlap: lane lifecycle; imports: slots
* `wtsafe.py` - worktree -> removed; config.yml hashes -> verdict; CLI `wtsafe.py --check | --unlink | --selftest`; overlap: lane lifecycle; imports: -

## Overlap families (which tools share a concept)

* **binary readers**: `dossier.py`, `dwarfmap.py`, `elfsect.py`, `langcheck.py`, `linkorder.py`, `m2cinput.py`, `unwindcut.py`, `vtableaudit.py`, `vtslot.py`
* **build steps**: `objalign.py`, `objextab.py`
* **cache**: `callers.py`, `undefrefs.py`
* **compile**: `recompile.py`
* **compiler/linker**: `LICENSE`, `PROVENANCE.md`, `README.md`, `dissect.py`, `extract_upstream_tables.py`, `fetch_gdb.py`, `make_port.py`, `mwcc_debugger.py`, `mwlink_debugger.py`, `pass_points.py`, `verify_pcode.py`, `versions.py`
* **data closure**: `dataclaim.py`, `datagap.py`, `dataqueue.py`
* **decoder**: `accessextent.py`, `callees.py`, `callers.py`, `dataattach.py`, `infer.py`, `phantom.py`, `splitcheck.py`
* **flag search**: `frame.py`, `infer.py`, `mwcc_matrix.py`, `optsweep.py`
* **landing**: `commitlint.py`, `land.py`, `mergebranch.py`, `prepcommit.py`, `unionguard.py`, `unionprose.py`, `unionresolve.py`, `verifyunit.py`
* **lane lifecycle**: `brief.py`, `claims.py`, `lane.py`, `lanecmd.py`, `queue.py`, `rescue.py`, `slots.py`, `worktreehook.py`, `wtsafe.py`
* **lib seed**: `unitutil.py`
* **line endings**: `checklf.py`, `edit.py`, `escape.py`, `guard.py`
* **map editing**: `dumpmap.py`, `mangle.py`, `methodize.py`, `phantom.py`, `symedit.py`
* **map parser**: `symedit.py`
* **map/splits parser**: `stylelint.py`, `symbolpreflight.py`
* **object comparison**: `datagap.py`, `flipcheck.py`, `pairgap.py`, `relocaudit.py`, `relocdiff.py`, `sectiongap.py`, `undefrefs.py`
* **outbox consumers**: `backlog.py`, `handoff.py`, `playbook.py`, `tooling.py`
* **playbook**: `ideas.py`, `ideas_demo.py`, `sync_playbook_index.py`
* **profiles**: `sync_profiles.py`
* **proposal queue**: `attribute.py`, `queue.py`
* **real-tree pin**: `preflight_selftest.py`
* **reference census**: `accessextent.py`, `callees.py`, `callers.py`, `dossier.py`
* **retired flows**: `promote.py`, `promote_batch.py`
* **rso (dormant)**: `inventory.py`, `symbols.py`
* **rule rows**: `datagap.py`, `flipcheck.py`, `land.py`, `splitcheck.py`, `stylelint.py`, `undefrefs.py`, `vtableaudit.py`
* **score readers**: `freshguard.py`, `ledger.py`, `metric_selftest.py`, `reportdiff.py`, `slotmap.py`, `symdiff.py`, `unitscore.py`, `verifyunit.py`
* **scoring**: `measure.py`, `recompile.py`
* **seam evidence**: `dataorder.py`, `dataseams.py`, `dump_asm.py`, `poolseams.py`, `tudiscover.py`
* **shape search**: `shapes.py`, `shapesearch.py`, `tryvar.py`
* **shared-file writes**: `sharedfiles.py`
* **source scanners**: `declclash.py`, `methodize.py`, `recordmerge.py`, `stylelint.py`, `typeregistry.py`
* **splits pipeline**: `applysplits.py`, `attribute.py`, `dataattach.py`, `matchinggain.py`, `splitcheck.py`
* **subprocess**: `spawnretry.py`, `subproc.py`
* **test harness**: `selftest.py`, `selftests-known-failures.json`, `sitecustomize.py`

## Dead and dormant, with the evidence

* `tools/flags/infer-run.md` - callers: tools 1. A generated snapshot of `infer.py --markdown` over the tree (stale the day after it was written).
* `tools/rso/inventory.py` - callers: docs 3, tools 1. Parse RSO module headers, sections and export/import tables (dormant: the RSO splitter blocker, docs/rso-modules.md).
* `tools/rso/symbols.py` - callers: docs 1, tools 1. Seed a per-module `symbols.txt` from an RSO export table (dormant, same blocker).
* `tools/splits/gen_trk_vectors.py` - callers: docs 1. One-shot generator of the two TRK interrupt-vector units (claimed and matched 2026-09-28).
* `tools/units/checklf.py` - callers: tools 3. Report a working-tree file whose line-ending style differs from its index/HEAD blob (overlaps `edit.py check`).
* `tools/units/checklf_selftest.py` - callers: tools 1. Temp-repo test of the CRLF-over-LF invisibility.
* `tools/units/promote.py` - callers: tools 13. Move a `src/auto` unit to its final name/location across map, source, configure.py, splits.txt (the auto bucket is retired; nothing calls it but promote_batch).
* `tools/units/promote_batch.py` - callers: tools 5. Batch of promotions / language promotions with one re-split (same retirement).
* `tools/units/promote_batch_selftest.py` - callers: tools 1. Spec, sequential apply, batch check on the promote fixture.
* `tools/units/promote_selftest.py` - callers: tools 2. Plan, four-file edit and byte comparator on a fixture tree.
* `tools/units/relocaudit-findings.md` - callers: none. A dated snapshot of one relocaudit sweep (history, not spec).

## Real-tree pins in selftests (the brittleness the owner named)

These selftests read the live tree rather than a fixture, which is why four of them broke as the splits landed:

* `tools/units/preflight_selftest.py` - derives rows from the live `symbols.txt`/`splits.txt`/`configure.py` with a private reader, plus two
  fixed rows (`_rom_copy_info`, `memmove`); the fixed representative moved three times (its own docstring, rounds 1-3).
* `tools/units/accessextent_selftest.py` -> `accessextent.selftest()` runs a Q_UserData acceptance case when `main.elf` exists (a call count pinned to the tree).
* `tools/splits/dataorder.py --selftest` and `tools/units/stylelint.py --selftest` carry rows that read the real map/splits (an unclaimed-seam count, an owner module).
* `tools/flags/infer_selftest.py` - the known-case accuracy table over real split objects (parked for a detector false positive since 2026-09-27).
* `tools/agents/sync_profiles_selftest.py`, `sync_playbook_index_selftest.py`, `ideas_selftest.py` - deliberately check the real tree (that is their job: generated copies must not drift).
* `tools/objdiff/metric_selftest.py`, `tools/units/measure_selftest.py`, `recompile_selftest.py`, `verifyunit_selftest.py` - integration rows against the real build when present, skipped otherwise (the tolerant form the design keeps).
