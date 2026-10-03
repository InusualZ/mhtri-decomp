# Duplication and duplicate-work analysis

Every claim below cites a function with its line range (from `tools/**` at `ec2609b46`) or a command. Line
counts are the cited function bodies; "saving" is the cited lines minus one shared implementation of the
same size class, so it is an estimate of what one lib module replaces, not a promise.

Method: `grep -n "^def <name>("` over the stems named in the brief's seed list, the intra-`tools/` import
graph (`from X import` / `import X` / `spec_from_file_location`), and the tokenizer pass in `inventory.md`.
The import graph is in `inventory.md` per file; the figure that matters for the design is that
**`unitutil.py` is imported by 40 files and nothing else is imported by more than 11** - the project already
has a lib, it is just one 739-line file that stops at compile commands and the metric.

## Ranked payoff (top 10)

| # | concept | re-implementations | lines today | lib target | est. saving | why it ranks here |
| ---: | --- | ---: | ---: | --- | ---: | --- |
| 1 | ELF/DOL readers + fixture ELF/DOL builders | 26 readers, 13 ELF builders, 10 DOL readers, 7 DOL builders | ~2 400 | `lib/binary` (one reader, one builder) | ~1 900 | the largest raw duplication; every reader re-derives `.rela<sec>` -> section pairing, symbol-index -> name, `sh_info` locals |
| 2 | selftest scaffolding (check helpers, fixture trees, delegation pairs, real-tree pins) | 25 `check()`s, ~22 fixture builders, 11 wrapper pairs, 6 live-tree pins | ~2 000 | `lib/testing` (`Checker`, `FixtureTree`, `GitFixture`, tiers) | ~1 200 | this is where the four breakages came from; a fixture policy is only enforceable with one fixture builder |
| 3 | `splits.txt` / `symbols.txt` / `configure.py` parsers and the ownership index | 24 / 18 / 14 | ~1 100 | `lib/project` (one parse, one `Ownership`) | ~800 | three answers to "who owns this address" is how the lint and a tool can disagree (callees.py already imports the lint's index to avoid exactly that) |
| 4 | PowerPC instruction decoders and disassembly-text parsers | 5 word decoders, 6 text parsers over 3 disassembler formats | ~900 | `lib/ppc` (one decoder over bytes, one reference scanner) | ~550 | each decoder covers a different opcode subset; a bug fixed in one (splitcheck's `scan_refs` sda bases) is absent from the others |
| 5 | report.json readers and the "missing `fuzzy_match_percent` = 0 %" rule | 16 readers | ~700 | `lib/report` | ~550 | the one rule the campaign measures by is written in at least 6 places (ledger, verifyunit, unitscore, land, reportdiff, pairgap) |
| 6 | repo / MAIN / worktree / scratch path resolution + `sys.path` shims | 20 root resolvers, 163 `sys.path.insert` sites in 122 files | ~600 | `lib/repo` + a real package | ~480 | the `cwd`-is-a-tree rule (`unitutil.repo_root`) was added after a fixture scored the real tree; the other 19 resolvers never got it |
| 7 | git wrappers and the subprocess codec rule | 16 `git()`/`run()` wrappers, 176 `text=True` sites | ~400 | `lib/git` + `lib/proc` | ~300 | the codec rule (F34) is correct today (`subproc.trap_sites` = 0) only because `encoding=` was pasted 176 times |
| 8 | CLI boilerplate: argparse, `--json`, `--selftest`, `--root/--main`, exit codes | 97 parsers, 73 `--selftest`, 5 exit-code conventions | ~900 | `lib/cli` | ~450 | the `--selftest` flag in 73 tools exists only so `selftest.py` can discover them; a registry removes the flag and the discovery heuristics |
| 9 | unit/name/path resolution | 30 functions | ~350 | `lib/units` | ~280 | `norm_unit` (claims), `normalize_unit` (measure, recompile), `unit_stem` (verifyunit, promote_batch, stylelint, undefrefs, recompile), `resolve_unit` (unitutil, dossier, unwindcut) answer one question |
| 10 | the check -> PASS/FAIL/UNKNOWN + evidence + remedy row | 12 shapes | ~500 (renderers) | `lib/findings` | ~300 | not a line saving so much as one JSON schema and one renderer for the gate, the lint, the audits and the seam checker |

Below the top 10 but worth one module each: atomic shared-file writes (8 writers, `sharedfiles.py` names two
it was meant to absorb and never did), line-ending handling (9), caches and stamps (5 stamp schemes), outbox
readers (8), mangling/generated-name predicates (6).

## (a) `splits.txt`, `symbols.txt` and `configure.py` parsers

**`splits.txt` - 24 parsers.** The format is one unit header line and indented `section start:end` rows, yet:

* `tools/elf/objalign.py:68` `parse_splits` (16 lines), `tools/splits/splitcheck.py:145` `parse_splits` (21) + `render_splits`,
  `tools/units/datagap.py:205` `parse_splits_text` (17), `tools/units/stylelint.py:569` `_parse_splits` (16) feeding the
  `Ownership` class at `:474-554` (81), `tools/units/vtableaudit.py:181` `parse_splits` (23), `tools/units/symbolpreflight.py:75`
  `load_splits` (22), `tools/units/unwindcut.py:139` `read_splits` (19), `tools/units/sharedfiles.py:112` `parse_ranges` (11),
  `tools/units/unionresolve.py:130,142` `split_units`/`split_ranges` (25), `tools/units/land.py:1453,1529` `_split_rows`/
  `_ranges_by_section` (28), `tools/units/mergebranch.py:285` `text_ranges` (12), `tools/units/queue.py:95` and
  `tools/units/brief.py:1103` `registered_text_ranges` (9, 13) + `brief.py:284` `splits_range` (20), `tools/units/attribute.py:191`
  `claimed_text` (8), `tools/splits/dataorder.py:276` `unit_data_ranges` (12), `tools/objdiff/unitscore.py:164` `split_claims` (20),
  `tools/units/flipcheck.py:126` `claims` (17), `tools/units/backlog.py:1419` `_splits_ranges` (18), `tools/units/dataclaim.py:919`
  `unit_ranges` (9), `tools/mwlink_debugger.py:2437` `_splits_starts` (22), `tools/units/promote.py:307` `splits_block` (16),
  `tools/units/verifyunit.py:153` `splits_unit_names` (15), `tools/splits/tudiscover.py:1439` `claimed_units` (12),
  `tools/units/ledger.py:132` inside `Ledger`, and `tools/units/preflight_selftest.py:85` `load_repo` (a deliberate second opinion).
* Three different return shapes for the same data: `{unit: {section: (start, end)}}` (objalign, flipcheck), a flat
  `[(unit, section, start, end)]` (sharedfiles, land, unionresolve), `{section: [(start, end, unit)]}` (datagap, stylelint).
  Every consumer that needs "which unit covers (section, address)" writes its own interval search
  (`symbolpreflight.covering:128`, `dataorder.unit_of:290`, `datagap.classify_address:263`, `stylelint._covering_range:2236`,
  `vtslot.containing_range:151`, `poolseams.owner_of:80`, `linkorder.covering_any:293`, `attribute.claimed_overlap:201`).

**`symbols.txt` - 18 parsers, one of them canonical.** `tools/symbols/symedit.py:68` `parse_line` is the parser the skill
calls canonical and `symbolpreflight`, `dumpmap`, `callees`, `m2cinput` and `unwindcut` import it. The others re-parse the
`name = section:addr; // type:.. size:..` line with their own regex: `splitcheck.parse_symbols:182`, `dataorder.load_symbols:112`,
`stylelint._parse_symbols:557`, `vtableaudit.parse_symbols:206`, `recompile.text_symbol_addresses:794`, `mergebranch.map_symbols:377`,
`flipcheck.map_symbols:549`, `undefrefs.map_rows:159`, `callers.Map:713-777` (65 lines, with its own streaming rule),
`typeregistry._map_rows:556`, `datagap.load_data_symbols:1222`, `tudiscover.load_map:207`, `brief.map_rows:309`,
`methodize.map_rows:111`, `promote.symbols_in_range:593`, `dataclaim.symbol_lookup:489`, `preflight_selftest` (private).
Non-negotiable 7 ("never load the map into context") is enforced by convention in each; a lib reader that only ever
yields rows makes it structural.

**`configure.py` - 14 parsers.** `Object(Matching|NonMatching, "path", ...)` and `cflags_*` lists are parsed by regex in
`infer.registered_units:711`, `brief.registered_units:177`, `langcheck.registered_units:372` + `cflags_tokens:394` +
`cflags_exceptions:443`, `relocaudit.registered_units:206`, `applysplits.parse_configure:465`, `promote.configure_libs:214`
+ `flag_units:184`, `verifyunit.configure_object_names:148`, `unionresolve.configure_objects:159`, `matchinggain.matching_units:33`,
`symbolpreflight.load_configure:99`, `backlog._cflags_groups:1323` + `_lib_groups:1364` + `_resolve_group:1334`, `land.flips_objects:2004`,
`rescue.main_registration:194`, `ledger.Ledger`. Two of them resolve `cflags` group inheritance (`langcheck`, `backlog`) with
different rules; `promote.expand_group:270` is a third. `configure.py` is Python: one `lib/project.configure` that imports
it (or parses it once with `ast`) ends the regex family.

**Ownership is answered three ways today.** `stylelint.Ownership` (map + splits, the lint's authority, which `callees`,
`callers`, `dataclaim`, `methodize`, `promote`, `backlog`, `mergebranch` import), `symbolpreflight.report` (map + splits +
configure, which `ledger`, `dataqueue`, `linkorder`, `dataclaim` import), and `ledger.Objects` (dtk's `config.json`, which
`dataqueue` and `dataclaim` import for the auto-object fallback). The design keeps one `Ownership` with all three sources.

## (b) Instruction decoders and reference scanners

Five decoders over raw instruction words:

* `tools/splits/splitcheck.py:263-440` `written_reg`, `scan_refs` (138 lines), `scan_calls`, `find_sda_bases:232` - the most complete
  (lis/addi/ori/load/store with r13/r2 bases, pool literals by loads, `.text` calls); `dataattach.py` reuses it through `splitcheck.Ctx`.
* `tools/symbols/phantom.py:114-152` `index_refs` (39) - `bl`/`b`/`bc` targets, `lis`+`addi`/`ori` materialisations, data words.
* `tools/flags/infer.py:183-293` field helpers + `Insn` (110) + `rlwinm_alias` - record forms, fmadd, pool shape, stmw/lmw.
* `tools/units/dossier.py:150` `decode_li` and `panic_calls:160` - the `li r4, line` before a `Panic`.
* `tools/units/vtableaudit.py:337` `is_code_pointer` and `tools/units/vtslot.py:115` `find_word_hits` - data words that are code addresses.

Six parsers over disassembler *text*, in three formats:

* dtk asm dump (`build/RMHE08/asm/*.s`): `tools/units/callers.py:260-332` `parse_dump_file` + `branch_target:191` (decodes the
  displacement from the hex bytes, not the label) and `tools/splits/tudiscover.py:408-471` `asm_files`/`rel_owners` (regex over the same files).
* `powerpc-eabi-objdump -d`: `tools/units/accessextent.py:210-303` (`split_insn`, `parse_mem`, `parse_disassembly`, `disassemble`),
  `tools/units/m2cinput.py:202-319` (`disassemble`, `parse`, `fold_psq_indexed`), `tools/mwcc-debugger/locate/verify_pcode.py:132` `read_objdump`.
* `dtk elf disasm`: `tools/units/callees.py:168-382` `parse_disasm`, `decode_rw` (78), `call_shape`.

Measured consequence: `accessextent.py`'s docstring (lines 1915-1918) explains that the two censuses it can read (callers'
dump graph and callers' object fallback) are "not equally detailed" and it repairs sites by re-decoding - a third decoder
layered on two. The reference *graph* is also built twice: `callers.build_index:350` (from asm text, cached) and
`callers.build_elf_index:557` (from object relocations, cached), with `datagap.census:1253` a third relocation census over
target objects that `poolseams`, `dataclaim` and the gate's data-closure rows read.

Target: `lib/ppc` with one `decode(word) -> Insn` (fields, mnemonic class, branch target, d-form base/displacement) and one
`scan(text_bytes, base, sda_bases) -> refs` (the `splitcheck.scan_refs` semantics, since that is the one dataattach proved
on the whole DOL), plus `lib/binary.refs` building the address-keyed reference index once from objects (relocations) with
the asm dump as an optional text source. Tools that need disassembler *text* for humans (`m2cinput`, `callees` shape) keep
their format-specific parser but share the mnemonic/operand tokenizer.

## (c) Repo, MAIN, orig and asm-dump path resolution

`unitutil.py:112-184` has the current rule (`caller_worktree`, `_cwd_tree`, `repo_root(start=)`, `main_tree`,
`resolve_input`) and its selftest pins the incident (`unitscore_selftest` scored the real tree from a fixture). Nineteen other
resolvers predate or bypass it: `recompile.worktree_root:115` / `main_root:124` / `main_worktree_list:148` (`git worktree list`),
`splitcheck.tree_root:90` / `main_root:95` (the DOL fallback), `infer.repo_root:677`, `methodize.repo_root:70`,
`guard.repo_root:40`, `commitlint.find_root:264`, `sync_playbook_index.find_root:84`, `sync_profiles.repo_root:108`,
`backlog.default_main:2021`, `rescue.default_repo:97`, `slots._main_root:3162`, `worktreehook._main_root:68`,
`measure_selftest._main_root:503`, `wtsafe.main_worktree:39`, `langcheck._root:81`, `unwindcut.tree_root:494`, and the
`ROOT = os.path.dirname(os.path.dirname(HERE))` constant in `callers`, `tudiscover`, `dataorder`, `symbolpreflight`, `ledger`,
`flipcheck`, `datagap`, `selftest`, `accessextent` (module-level, so a fixture has to monkeypatch it: `dump_asm_selftest.py`
"points those at a temp tree" by reassigning module globals).

`sys.path.insert(` appears 163 times in 122 files (`grep -c`), because `tools/` is not a package on the path: every tool
inserts `tools/`, and often `tools/units`, `tools/symbols`, `tools/splits`, before it can import a sibling. A package layout
(`tools/lib`, tools importing `lib.*`) removes all of them.

The asm dump path and its freshness are known to `tudiscover` (`dump_files`, `dump_stamp`, `asm_stamp_status:376`,
`use_local_dump`, `dump_is_main_fallback`), `callers` (`asm_dir_of:138`, `dump_signature:158`, `is_scaffolding:173`),
`dump_asm.py` and `dataqueue.read_graph_cache:467`; the DOL path and its hash are known to `wtsafe.ground_truth:123`,
`prepcommit.ground_truth_error:70`, `land.verify` row 1, `splitcheck.Dol`, `tudiscover.Dol`, `m2cinput.load_dol`, `dossier.dol_blob`.

## (d) git helpers and the subprocess codec rule

Sixteen wrappers: `claims.git:84` + `_git_quiet:999`, `land.run:305` + `git:309`, `lane.git:55`, `mergebranch.git:116`,
`promote.git:146`, `rescue._run:81` + `git:85`, `slots.git:396`, `stylelint.git:2571` + `git_bytes:2578`, `unionguard._git:58`,
`vtableaudit._git:765`, `wtsafe._git:176`, `checklf.git:39`, `guard._git:31` + `_git_text:35`, `prepcommit.git:43`,
`recompile.git:108`, `subproc.run:43`. They differ in signature (`git(args, cwd)` vs `git(root, *args)`), in whether `check`
raises, and in return type (str, bytes, `CompletedProcess`, `(rc, bytes, str)`).

The codec rule (F34, `tools/units/subproc.py`) is enforced by an AST scan and today reports **0** trap sites
(`subproc.trap_sites('.', 'tools', True)` run on this tree) - across 176 `text=True` call sites in 52 files, i.e. the kwargs
were copied 176 times. `merge-base`, `ls-files`, `show REF:path`, `worktree list --porcelain`, `rev-parse --show-toplevel`
and `status --porcelain` are each spelled in several of the wrappers' callers (`stylelint._fork_point:4591`,
`rescue.merge_base:123`, `land._is_ancestor:1084`, `slots.merge_tree_of:1261`, `queue._file_lines:431`, `mergebranch.blob:124`,
`stylelint._tree_text`/`git_bytes`, `checklf.blob_of:45`, `guard.index_blob:52`).

## (e) Report readers and score comparisons

Sixteen readers of `build/RMHE08/report.json` (or of a one-unit `report generate`), each re-walking
`units[].functions[]` and `measures`: `land.report_snapshot:2064` / `report_regressions:2112` / `regression_rows:2038` /
`ledger_numbers:2150` / `unit_grew:2094`, `applysplits.function_scores:1403` / `regression_rows:1413` (a second regression
rule), `reportdiff.load_report:107` / `unit_measures:121` / `symbol_measures:131` / `denominators:145` (a third, with no caller),
`verifyunit.report_unit:428` / `report_functions:437` / `_score:447` / `arithmetic_crosscheck:461`, `unitscore.rows_of:115` /
`measures_of:148`, `pairgap.report_scores:332` / `units_from_report:263`, `datagap.units_from_report:96`, `brief.report_scores:356`,
`dataclaim.unit_stats:473`, `promote_batch.report_score:290`, `ledger.Ledger`, `measure.aggregates:149` / `load_baseline:217`,
`prepcommit._report_measures:125`, `slots.report_matches:849`, `mwcc_matrix.summarize:72`, `tryvar.match_pcts:64`.

The rule "a function entry without `fuzzy_match_percent` is 0 %, not 100 %" is implemented independently in `ledger`,
`verifyunit._score`, `unitscore.rows_of`, `land.report_snapshot`, `reportdiff.num`, `pairgap.report_scores`. The unit
naming used to key those readers is itself duplicated (`ledger.report_name:60`, `verifyunit.report_unit_name:135`,
`measure.normalize_unit:69`, `unitscore.spec_of:100`, `project.objdiff_unit_name:1619`).

The *metric* is already one implementation (`unitutil.report_functions:600` / `report_measure:623` / `measure_project:583`),
and `metric_selftest.py` is the contract test that keeps `symdiff`, `tryvar`, `mwcc_matrix`, `recompile`, `measure` on it.
That is the model: one lib function, one contract test, thin callers.

Staleness is also checked three ways: `freshguard.freshness:59` (the shared rule `symdiff`/`unitscore`/`relocdiff` use),
`ledger.stale:70` (report vs. four source files), `datagap.tree_freshness:1081` (whole tree with tolerance), plus
`linkorder.staleness:602` and `recompile.object_is_fresh:587`.

## (f) Object, ELF and DWARF readers

Twenty-six ELF readers (section headers, symtab, rela): `unitutil.read_elf:456`, `elfsect.sections:2`, `objalign.read_sections:101`,
`objextab.read_sections:85` (a copy of objalign's), `infer.Elf:72-178`, `dossier.parse_elf:81`, `linkorder.parse_elf:109`,
`flipcheck.sections:92` / `elf_sections:377` / `object_symbols:437` / `comment_symbols:409` / `code_references_in:467`,
`langcheck._elf_sections:218` / `elf_symbols:243`, `relocaudit._read_symtab:153` / `object_sets:179`, `sectiongap.read_object:100`,
`undefrefs.load_object:131`, `vtableaudit.read_object:438`, `verifyunit.symbol_locations:346` / `object_symbols:368`,
`datagap.section_sizes:68` / `object_data_refs:281` / `object_reloc_sources:304` / `object_fingerprint:1281`,
`dataclaim.read_object_sections:427`, `pairgap.read_symbols:129`, `relocdiff.read_relocs:74`, `callees.code_references:95`,
`callers.build_elf_index:557`, `mwlink_debugger.Elf:559` / `MwObject:1161`, `promote.Elf:414`, `recompile.section_sizes:615`.
Each one reimplements: `e_shoff`/`e_shentsize` walking, `.shstrtab` names, `.symtab` + `.strtab`, `.rela.<sec>` -> `<sec>`
pairing, RELA addend, `R_PPC_*` type names (`relocdiff.type_name:99`, `sectiongap._type_name:96`, `mwlink_debugger.reloc_name:1157`).

Thirteen fixture ELF *writers* in tests: `objextab.build_elf:378`, `pairgap._elf32:637`, `infer_selftest.build_obj:106`,
`flipcheck_selftest.build_obj:55`, `undefrefs_selftest.build_obj:43`, `sectiongap_selftest.build_elf:45` (which `relocdiff_selftest`
imports, the one reuse), `linkorder_selftest.build_elf:36`, `verifyunit_selftest.build_object:273`, `vtableaudit_selftest.build_object:56`,
`relocaudit_selftest.write_elf:47`, `promote_selftest.write_elf:148`, `callees._fixture_elf:653`, `mwlink_debugger._fixture_elf:3122` + `_fixture_elf2:3496`.

Ten DOL readers (`dataclaim.dol_sections:167`, `dossier.dol_sections:242` - identical, `vtableaudit.dol_segments:223`,
`vtslot.data_segments:98`, `linkorder.parse_dol:158`, `splitcheck.Dol:200`, `tudiscover.Dol:181`, `m2cinput.Dol:322`,
`unwindcut.Dol:105`, `phantom` via `in_text`/`bytes_at`) and seven DOL fixture builders (`splitcheck._make_dol:2785`,
`linkorder_selftest.build_dol:96`, `unwindcut_selftest.build_dol:66`, `vtslot_selftest.fixture_dol:58`, `vtableaudit_selftest.table_dol:220`,
`m2cinput_selftest.make_dol:243`, `dataorder._FakeDol:362`).

DWARF has one reader (`elf/dwarfmap.py`) and no duplication; it belongs in the same package for discoverability only.

## (g) Unit / path / name resolution

Thirty functions turn a unit spelling into another spelling: `unitutil.resolve_unit:281` (the documented spec grammar),
`claims.norm_unit:269` (which `vtslot`, `brief`, `promote`, `land` import), `promote.norm_unit:154` (a copy),
`measure.normalize_unit:69`, `recompile.normalize_unit:166` / `unit_source:162` / `resolve_unit_source:185` / `_unit_stem:259` /
`target_rel:911`, `verifyunit.unit_stem:108` / `src_object_rel:125` / `target_object_rel:130` / `report_unit_name:135`,
`promote_batch.unit_stem:305`, `stylelint._unit_stem:2381`, `undefrefs._unit_stem:461`, `ledger.report_name:60` / `bare:65`,
`queue.registered_norm:91` / `is_proposal:128`, `flipcheck.unit_name_for:145`, `datagap.object_path:1350`,
`relocaudit.object_paths:216`, `vtableaudit.object_paths:431` / `_same_unit:1059`, `dossier.resolve_unit:884`,
`unwindcut.resolve_unit:178`, `infer.obj_path_for:705` / `resolve_object:732`, `objalign.unit_key:86`, `pairgap._stem_of:280` /
`resolve_specs:293`, `unitscore.spec_of:100`, `brief.source_name:132` / `brief_unit:1576`, `applysplits.stem_of:169`,
`dataattach.stem_of:689`, `mwlink_debugger.unit_of_object:2264`, `project.objdiff_unit_name:1619`.
The spellings are: `Pl/pl_act`, `src/Pl/pl_act.cpp`, `main/Pl/pl_act`, `build/RMHE08/{src,obj}/Pl/pl_act.o`, a report key, a
`splits.txt` key with extension, and a claim slug. One `Unit` value type with those as properties (plus the extension inferred
from `src/`, which `recompile.resolve_unit_source` already does with a MAIN fallback) replaces all of them.

## (h) Selftest scaffolding

* **25 files define their own assertion helper** (`def check(name, got, want)` / `_ok` / `expect` / `eq` / `_truthy` /
  `_contains`): `grep -l "^def (check|_check|_ok|expect|eq|_truthy|contains|_contains)("`. They print in at least four
  shapes, which is why `selftest.py:87-93` carries five regexes to count them (`ok - N checks`, `N/N checks passed`, ...).
* **73 tools expose `--selftest`** and **51 standalone `*_selftest.py`** exist; **11 are delegating pairs** (one file imports
  the other), which `selftest.py:207-220` detects with four regexes so it does not run both. `pairgap_selftest.py` and
  `subproc_selftest.py` exist *only* to satisfy the discovery (their docstrings say so).
* **Fixture builders**: ~12 real-git fixtures (`commitlint_selftest.init_repo`, `guard_selftest.temp_repo`, `checklf_selftest`,
  `unionguard_selftest._init`, `rescue.selftest`, `lane.selftest`, `claims.selftest`, `land.selftest` (1 875 lines),
  `mergebranch.selftest` (709), `slots.selftest` (1 132), `stylelint.selftest` (1 698), `queue.selftest` (631)) and ~10 fake
  project trees (`unitscore_selftest.Fixture`, `measure_selftest._fake_main`/`_fake_worktree`, `recompile_selftest` (the same
  pair, copied), `promote_selftest.build_fixture`, `callers._fixture_root`, `callees` fixtures, `vtableaudit_selftest.make_tree`,
  `vtslot_selftest.make_tree`, `ledger_selftest.fixture`, `backlog/tooling/playbook_selftest.outbox`).
* **Real-tree pins** (the breakage class): `preflight_selftest` (derived rows + two fixed symbols that moved three times),
  `accessextent.selftest` acceptance run, `dataorder.selftest:520` (`> 30` seams in the real DOL), `stylelint.selftest:3365-3375`
  (the real map names an enemy unit), `infer_selftest.test_known_objects` (parked since 2026-09-27), plus the deliberate
  real-tree checks in `sync_profiles`/`sync_playbook_index`/`ideas` (generated copies must not drift - those stay).
* The selftest bodies are **~27 % of all tool code**: 14 812 lines in standalone files plus the in-file `selftest()` functions
  (`land.py:3138-5013`, `stylelint.py:2879-4577`, `slots.py:2025-3157`, `claims.py:1785-2602`, `brief.py:1788-2368`,
  `splitcheck.py:2802-3544`, `datagap.py:1797-2620`, `symedit.py:737-1190`, ...), roughly 12 000 more.

## (i) CLI scaffolding, JSON IO, scratch and caches

* `argparse.ArgumentParser(` 97 times in 96 files; `--json` in 64 of the 168 Python files (metrics pass); `--selftest` in 73;
  `--root`/`--main`/`--repo` spelled three ways (`backlog --main`, `rescue --repo`, `declclash --root`).
* Exit-code conventions documented in at least five places with different meanings for `2`: `checklf` (usage), `reportdiff`
  (nothing comparable), `verify_pcode` (error), `declclash` (only with `--fail-on-different`), `commitlint` (0/1/2), `methodize`
  (2 = refused plan).
* `build/tmp/<tool>/` scratch is chosen per tool (`unitutil`, `tudiscover`, `callers`, `undefrefs`, `measure`, `matrix`, `shapes`,
  `applysplits`, `dataattach`); `.pi/` state files are named in 40 places across 30 tools (`claims.json`, `backlog.json`,
  `land-base.json`, `data-requests.json`, `merge-state.json`, `slots/*.json`, `lanes/`, `outbox/`, `notes/`).
* Five cache-stamp schemes: `callers.dump_signature:158` (path+size+mtime over files), `tudiscover.dump_stamp:349` /
  `asm_stamp_status:376` (sha of three inputs, five states), `undefrefs._sig:201`, `dataattach.stamp_of:1366`,
  `verifyunit.sha256_file:241` / `target_object_snapshot:287`.
* Atomic writes: `sharedfiles.Transaction:157` (the documented owner), `symedit._write_text:409`, `dataqueue.write_queue:314`,
  `backlog.write_atomic:1058`, `dataclaim.write_atomic:385`, `escape.atomic_write:96`, `sync_profiles.write:286`,
  `sync_playbook_index.write:79`, `edit._write:37`. `sharedfiles.py:21-23` names `symedit` and `dataqueue` as the next writers
  to move there; neither moved.
* Line-ending handling: `edit.py` (bytes, CRLF/LF-agnostic needle), `escape.py --edit` (bytes), `checklf.py` (tree vs blob),
  `guard.py` (index blob), `sharedfiles.line_ending:65`, `mergebranch.newline_of:210`, `sync_profiles.nl_of:276` / `to_lf:282`,
  `tryvar --apply`, `promote` (preserve). One `lib/text.endings` serves all nine.

## (j) Rule / violation / gate-row machinery

Twelve shapes for "a check produced a verdict with evidence and a remedy":

| producer | shape | renderer |
| --- | --- | --- |
| `land.verify:2470` | `(name, good, detail, info, kind, remedy)` tuples, ~30 rows | `land.py` table + JSON + `failure_summary` |
| `stylelint._finding:1220` | `{file, line, rule, detail, token}` dicts; `finding_identity:2223` for diffs | `print_findings`, `print_budget`, `--json` |
| `vtableaudit.violation_rows:1064` | rows + `violation_keys` for `--diff` | `render:1226` |
| `undefrefs.check_object:339` | hits with spelling hint; base-snapshot subtraction | `render_hits:330` |
| `datagap.strict_report:679` / `strict_verdict:756` | `REFUSE` / `deferred <class>` per pair | `render_strict:722`, `render_census` |
| `splitcheck.Results:788` | PASS/FAIL/UNKNOWN/`-` per unit per invariant with an address | `print_table`, `report_json` |
| `flipcheck.check:719` | `   - ` reason lines "add-only, lanes parse them" | inline |
| `verifyunit.*_problems` | lists of strings | consumed by `land` |
| `symbolpreflight.severity_for:147` | kind + severity (`escalate`/`approve`/`proceed`/`never touch`) | `render` |
| `dataclaim.classify:224` | `safe` / `lowers-score` / `overlap` / `unowned` + effect text | `render`, `summary` |
| `handoff.validate:170` | problem strings | inline |
| `unionresolve.check_union:203`, `mergebranch` classes, `reportdiff` exit | ad hoc | ad hoc |

None of them share a type, so the gate re-renders each tool's output (`land.py` shells out to `flipcheck`, `langcheck`,
`stylelint`, `ledger`, `playbook`, `selftest`, `commitlint` and parses their text/JSON; `selftest_detail:518`,
`command_detail:490`, `flipcheck_problems:2019` are the parsers). A single `Finding` / `Row` type with a stable JSON schema
is what lets the gate *call* a check instead of parsing it, and lets every audit print the same table.

## (k) Outbox and notes readers

`.pi/outbox/*.json` and `.pi/notes/*.md` are loaded by `backlog.build_items:416` + `_add_request:491`,
`tooling.load_sources:497` + `candidates_for:421`, `playbook.load_outboxes:627` + `extract_findings:346` + `extract_note_findings:445`,
`handoff.validate:170` + `request_content:89`, `brief` (residuals), `queue.selftest`, `slots._merge_data_requests:1821` / `collect:1842`,
`land.outbox_units:2226`, `splitcheck.seam_requests:1633`. Each tolerates schema drift its own way (`backlog.asstr:192`,
`tooling._flatten:363`, `handoff.FREE_TEXT_FIELDS`, `playbook` "the schemas drift between rounds"). The schema is defined once
(`handoff.CONFIG_REQUEST_SCHEMA`) and read seven times.

## (l) Mangling and generated-name predicates

`relocaudit.linkage_stem:95` and `undefrefs.linkage_stem:187` are the same function; `langcheck.mangled:126`,
`stylelint` rule 9's regex, `typeregistry.tokens_of_name:144`, `mangle.estimate_member_mangling:154` peel MWCC manglings;
`callees.is_generated:87`, `dumpmap.is_generated:77`, `stylelint` rule 7, `brief.text_has_bodies:158`, `phantom` classify the
`fn_`/`lbl_`/`loc_`/`@etb_` generated spellings. One `lib/names` module.

## Duplicate *work* (the same question answered by several tools)

* "Is this unit the target object?" - `datagap` (section sizes), `sectiongap` (bytes + relocs per section), `flipcheck` (sizes,
  bytes, permutation, undefined refs), `pairgap` (symbol pairing), `relocdiff` (relocations), `undefrefs` (undefined names),
  `relocaudit` (linkage spellings), `verifyunit` (per-symbol re-measure), `linkorder` (the link). Eight tools, one object pair.
* "Who references this address?" - `callers` (dump + object fallback), `datagap.census` (target-object relocations),
  `splitcheck.Ctx` (retail text decode), `tudiscover.build_graph` (dump regex), `accessextent` (callers + objdump re-decode),
  `symedit refs` (source text), `callees` (target relocations + disasm shape), `vtslot` (data words). Five independent censuses.
* "Where does this TU end?" - `tudiscover`, `dataorder` + `dataseams`, `poolseams`, `splitcheck` invariants, `dataattach`
  chains, `attribute.segments`, `unwindcut`. The evidence kinds (pool, `__FILE__`, data order, ctors closure, extab) are each
  computed in two places (`splitcheck.unit_sinit_closure:1235` vs `tudiscover.extab_runs:1183`; `splitcheck.check_pool:1161` vs
  `poolseams.pool_edges:107` vs `tudiscover.pool_dedupe_cuts:771`).
* "What is the score?" - settled (one metric), but "did it regress?" is `land.report_regressions`, `applysplits.regression_rows`,
  `reportdiff.diff_symbols`, `measure.moved_summary` - four regression rules with four thresholds (`eps`, `unit_grew`, 0.01, exact).
* "Can I union this conflict?" - `unionguard` (index stages, applybranch.sh path), `unionresolve` (land path) and `mergebranch`
  (lane path) with `unionprose` shared by two of the three.
* "Tear this lane down safely" - `claims.release`, `lane.teardown`, `slots.release`/`reclaim`, `wtsafe.remove_worktree`, `rescue`:
  each with its own rescue-ref and worktree-removal sequence; `claims.py:1490-1702` `release` is 213 lines and `slots.release:1736`
  another 60 with the same steps.
