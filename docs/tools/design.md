# The tool framework: target architecture

Decided design for `tools/` after `inventory.md`, `duplication.md` and `prose-audit.md`. One package of shared concepts
(`tools/lib/`), thin tools on top, one test harness, one finding shape, and the dense prose moved out of the code into
`docs/tools/spec/`. Every CLI the profiles, skills, docs and gate call keeps working at every step (`migration.md`).

## 1. Principles

1. **One implementation per concept.** A parser, a reader, a rule, a path, a git call: written once in `tools/lib/`, imported
   everywhere. `duplication.md` is the list; a second copy of a rule is the defect this project pays for most.
2. **Lib never imports tools; tools import lib only.** `tools/lib/**` has no `import tools.units...`. A tool may import another
   *tool* only through its public API listed in the spec (today's `callees -> stylelint.Ownership` becomes `lib.project.Ownership`).
   The import cycles that exist today (`claims <-> queue <-> slots`, `brief <-> dossier`, `datagap <-> poolseams`,
   `stylelint -> datagap -> undefrefs -> dossier -> brief`) are forbidden by construction: the lib has no cycles, and a tool
   importing a tool is a lint finding.
   A file split carries the moved code's edges and nothing else (`datagap -> dataclosure`, and `dataclosure ->
   callers/poolseams` in place of datagap's own): the allow-list swaps them in the same batch and its net never grows.
   WP3c's `tools/splits/seams/` and `tools/splits/invariants/` are tool packages in section 5's sense (an `__init__.py`, imported
   as `import tools.splits.<package>.<module> as x`, so no edge names the package marker).
3. **The code keeps one header line; the spec keeps the prose.** Every tool opens with
   `"""<one-line purpose>. Spec: docs/tools/spec/<name>.md. CLI: <shape>."""` and nothing else at module level. Rules live in the
   spec's "Invariants and rules"; incidents live there as a date in parentheses at most.
4. **A finding is data.** Every check (lint rule, audit, gate row, seam invariant, flip reason) produces the same `Finding`/`Row`
   value with a stable JSON schema, so the gate *calls* a check and never parses its text.
5. **A selftest is a fixture or it is smoke.** Fixture tests build their tree with `lib.testing` and cannot see the live tree;
   smoke tests may read the live tree, are tolerant (skip, never pin a count), and run in their own tier.
6. **Stdlib only, Python 3.12, no new runner.** `tools/selftest.py` stays the one command; no package manager, no pytest
   (question 5 in `questions.md` asks whether to allow pytest later).
7. **Measure every step.** A package lands only with the before/after numbers its acceptance names (`migration.md`).

## 2. Layout

```
tools/
  lib/                     the shared concepts (a package; no CLI; no tool imports)
    __init__.py
    repo.py                trees, paths, scratch, state, ground truth
    proc.py                subprocess with the codec rule, spawn retry, kill tree
    git.py                 the git calls the tools make
    text.py                endings, byte-exact replace, atomic write, anchors, transactions
    cache.py               stamped caches
    names.py               generated-name and mangling predicates
    project/               the three project files and ownership
      symbols.py splits.py configure.py ownership.py
    binary/                the bytes: ELF, DOL, objdump text, DWARF, fixture builders
      elf.py dol.py objdump.py dwarf.py build.py
    ppc.py                 instruction decode and the reference scanner
    refs.py                the address-keyed reference index and census
    report.py              objdiff report readers, the metric, freshness, regression diff
    units.py               the Unit value type, compile command, compile
    objcompare.py          target-vs-ours comparisons (sections, bytes, symbols, relocations, undefined names)
    cscan.py               C/C++ lexical scanning (comments, braces, declarations, fields)
    findings.py            Finding, Row, Verdict, add-only diff, renderers, exit codes
    cli.py                 the tool entry point: parser, common flags, exit policy, registry
    testing.py             Checker, FixtureTree, GitFixture, tiers
    outbox.py              the outbox/notes schema and tolerant loaders
    lanes/                 slug/branch/worktree naming, registry, rescue, teardown, launch line
      __init__.py naming.py registry.py rescue.py teardown.py launch.py
  tests/                   every selftest, re-homed (section 7)
    <area>/test_<tool>.py
  units/ symbols/ splits/ objdiff/ elf/ git/ agents/ flags/   the thin tools (same paths and names as today)
  splits/seams/            the TU-seam evidence (`evidence.py`) the four seam CLIs share (WP3c)
  splits/invariants/       the `splits.txt` audit, one module per invariant; `splitcheck.py` is its CLI (WP3c)
  mwcc-debugger/ mwlink/   the compiler and linker debuggers (packages; shims keep the old paths)
  selftest.py              the runner (discovers tools/tests/**)
  project.py ninja_syntax.py download_tool.py transform_dep.py decompctx.py changes_fmt.py   template, untouched
```

**Import form.** `tools/` is already a package (`tools/__init__.py`). Every tool starts with one identical line that puts the
repository root on `sys.path` (lint-checked, replaces the 163 ad hoc inserts), and imports `from tools.lib import ...` and
`from tools.units import claims`. `python -m tools.units.land ...` works from the root without the line; the path form
`python tools/units/land.py ...` keeps working (it is what every doc and profile spells).

## 3. The lib modules (responsibility and public API)

Names are indicative; the spec of each module (`docs/tools/spec/lib-<module>.md`) is the contract. "From" names the code that
becomes it (`duplication.md` has the line numbers).

### `lib/repo.py` - trees and paths

* `Tree` value: `root`, `main` (the MAIN tree of a worktree or slot, via `git worktree list --porcelain`), `is_worktree`,
  `is_slot`, `build(version)`, `obj_dir`, `src_obj_dir`, `asm_dir`, `orig_dol`, `config_yml`, `report_json`, `objdiff_json`.
* `repo_root(start=None)`: the `cwd`-is-a-tree rule from `unitutil.repo_root` (the invocation's tree wins; a fixture is a tree).
* `main_tree(root, honour_env=False)` (the parent of `git rev-parse --git-common-dir`, not the first `git worktree list` row,
  which is registration order), `resolve_input(rel, root, probe, honour_env=False)` (worktree first, MAIN fallback -
  `unitutil.resolve_input`, `recompile.main_root`, `splitcheck.main_root`).
* `guard(path, what)`: the fixture tier's choke point for what the audit hook cannot see (`os.stat` probes, roots bound at
  import time): every root `lib.repo` resolves or is handed is refused under `TIER="fixture"` when it is the live tree. A module
  never resolves the live tree at import time (`unitutil.ROOT` is resolved on first use) - WP0 gaps 1-2, closed in WP1a.
* `scratch(tool) -> build/tmp/<tool>/`, `session_tmpdir()` (unique, removed at exit), `state(name) -> .pi/<name>`
  (one place names `claims.json`, `backlog.json`, `land-base.json`, `data-requests.json`, `slots/`, `lanes/`, `outbox/`, `notes/`).
* `ground_truth(tree)`: the DOL path and the pinned hashes from `config.yml` (`wtsafe.ground_truth`, `prepcommit.ground_truth_error`).
* From: `unitutil`, `recompile`, `splitcheck`, `wtsafe`, `tudiscover`, `callers`, 20 root resolvers.
* As built (WP3b): `main_checkout(root)` is `main_tree` with the first-worktree fallback a tool that must have a MAIN
  needs (`recompile.main_root`, `measure.py`).

### `lib/proc.py` - subprocess

* `run(args, cwd=None, check=False, input=None, timeout=None) -> Completed` with `encoding="utf-8", errors="replace"` always
  (F34); `run_bytes` for binary output; `kill_tree(proc)`; `install_spawn_retry()` - explicit, not on import (importing a lib
  module must not patch `subprocess`); `unitutil` and `selftest_site/sitecustomize.py` call it, as they did `spawnretry.install`.
* `trap_sites(root)` (the AST scan) becomes a test in `tests/lib/test_proc.py`.
* From: `subproc`, `spawnretry`, `selftest._kill_tree`, `land.run`.

### `lib/git.py` - git

* `Git(cwd)` with the calls the tools make, typed: the primitives `run`/`run_bytes`/`out`/`ok` (each wrapper keeps its own
  failure policy and message, only the process call is shared), `rev_parse`, `head`, `current_branch`, `toplevel`,
  `common_dir`, `merge_base`, `is_ancestor`, `fork_point`, `merge_tree`,
  `show(ref, path) -> bytes`, `cat_index(path)`, `ls_files(eol=False)`, `status_porcelain`, `diff_names(a, b)`,
  `renames(a, b)`, `worktree_list`, `worktree_add/remove`, `branch_exists/create/delete`, `refs(prefix)`, `update_ref`,
  `stage(paths)`, `unstage`, `commit(message_file, pathspec)`, `merge_file_diff3`, `unmerged_stages`.
* Everything text goes through `lib.proc`; a path is always forward-slash on the way out.
* From: the 16 wrappers; `stylelint._fork_point`, `rescue.merge_base`, `slots.merge_tree_of`, `mergebranch.blob`, `guard.index_blob`.

### `lib/text.py` - bytes, endings, atomic writes

* `endings(data)` (LF/CRLF/mixed/lone-CR), `to_lf`, `with_ending`, `dominant` (`edit.py`); `replace_bytes(data, old, new, count)`
  (CRLF/LF-agnostic needle, count asserted, from `edit.replace_bytes`); `atomic_write(path, data)`; `Transaction`
  (temp + `os.replace`, exact restore - `sharedfiles.Transaction`); `insert_after_anchor`, `append_blocks`, `missing_anchors`.
* From: `edit`, `escape`, `sharedfiles`, `symedit._write_text`, `dataqueue.write_queue`, `backlog/dataclaim.write_atomic`,
  `sync_profiles.write`, `mergebranch.newline_of`.

### `lib/cache.py` - stamped caches

* `Stamped(path, inputs: list[path] | callable) -> load() | None, save(obj)`, `state() in {missing, stale, fresh, unstamped}`;
  signature = sha of each input's bytes or (path, size, mtime) for large dumps (the `callers.dump_signature` form).
* From: `callers.load_index`, `tudiscover.asm_stamp_status`, `undefrefs.link_symbol_index`, `dataattach.load_analysis`,
  `verifyunit.target_object_snapshot`.

### `lib/names.py` - names

* `is_generated(name, scheme)` - the tools ask four different questions (`default`, `rule7`, `map`, `ledger`), so each is a
  named scheme written once rather than one predicate that would change three tools' answers; `address_of`, `is_mangled`,
  `linkage_stem`, `estimate_member_mangling`, `estimate_static_mangling`, `peel_tokens` (the `<digits><chars>` peel).
* From: `callees.is_generated`, `dumpmap.is_generated`, `relocaudit.linkage_stem`, `undefrefs.linkage_stem`, `langcheck.mangled`,
  `mangle.estimate_*`, `typeregistry.tokens_of_name`, `stylelint` rule 7/9 regexes.

### `lib/project/` - the three project files and ownership

* `symbols.py`: `SymbolMap(path)` streaming over `config/RMHE08/symbols.txt` (never whole into a string a caller can print):
  `rows()`, `by_name`, `by_section()`, `at(address, count, section)`, `in_range(lo, hi, section)`, `find(regex, section, type)`,
  `plan_rename(pairs, force)`, `apply(plan, write=None)`, `plan_merge(rows, scan_refs)`, `check()` (duplicates, aliases,
  unparsed). Line parser is `parse_line` (was `symedit.parse_line`); the write is `write_text` over `lib.text.Transaction`.
  Non-negotiable 7 becomes structural: no API returns the file text.
* `splits.py`: `Splits.parse(text)` / `render()` round-trip, byte for byte for every unedited block (`render_splits` was
  already retired with the splits program, so there was no second renderer to absorb), `units`, `ranges`, `covering(section,
  address) -> Range | None`, `claims(unit)`, `overlap(section, lo, hi)`, `add_block`, `rename_unit`, `remove_block`,
  `text_ranges()`, `by_section()`, `by_unit()`; one return shape (`Range(unit, section, start, end, attrs)`).
* `configure.py`: `Configure.load(path)` by **evaluating** `configure.py`'s AST, never importing it (WP1c: importing runs
  argparse on the caller's argv and the build generator; the evaluator models the statements the file uses and is pinned
  to the executed file by a smoke test): `objects()` (`Object(flag, path, **kw)` rows with their lib), `libs()`,
  `cflags(group or lib)` with spreads, filters and appends resolved (one rule; `langcheck`, `backlog` and `infer` had three),
  `matching_units()`, `object_line_text`; plus `object_calls(text)` for fragments (a diff line, a conflict side).
* `ownership.py`: `Ownership(symbols, ranges, auto=None, root=None)` built by `load(root)` (map + splits, + dtk `config.json`
  when `auto=True`) or `at_ref(root, ref, show=None)` (the two files at a ref, read through `lib.git`, or through the
  caller's `show` - stylelint keeps its stubbed seam): `owner_of(section, address) -> Owner(state in {reconstructed,
  registered, auto, unsplit}, unit, section, range, band)`, `resolve(name)`, `unit_of_symbol(name)`,
  `symbols_of_unit(unit, section)`, `band_of(section, address)`. WP1c: the index is a value built from data rather than
  `Ownership(tree, ref)`, because `stylelint` (and `land`'s base-side check) build it from texts they already hold;
  `reconstructed` means "the unit's source exists", the vocabulary `callers`/`callees` used.
* From: 24 + 18 + 14 parsers, `stylelint.Ownership`, `symbolpreflight`, `ledger.Objects`, `sharedfiles.parse_ranges`.

### `lib/binary/` - the bytes

* `elf.py`: `Elf.read(path|bytes)`: `sections` (name, type, addr, offset, size, data, align), `symbols` (name, section, value,
  size, bind, type), `relocs(section)` (offset, symbol, type, addend, type name), `comment` (the CodeWarrior bytes and flags),
  `section_bytes(name)`, `defined/undefined/global sets`; the writer for the two ninja steps (`objalign`, `objextab`) is
  `ElfEditor(elf)` (`set_section_align`, `set_symbol_info`, `rename_symbols`, `write`): a parsed `Elf` stays an immutable
  view, and the edit is the splice those steps already proved byte-neutral.
* `dol.py`: `Dol.read(path)`: `segments` (text0..6, data0..10, bss), `bytes_at(address, n)`, `section_of(address)`, `text_ranges`,
  `data_ranges`, `words(address, n)`.
* `objdump.py`: `locate(root)`, `disassemble(objdump, obj|elf, sections)`, `dtk_disasm(obj, dtk)`, and one tokenizer for the
  objdump / dtk / asm-dump line shapes (`mnemonic`, `operands`, `address`, `relocation`), so `accessextent`, `m2cinput`, `callees`,
  `callers` and `verify_pcode` parse one way.
* `dwarf.py`: `dwarfmap`'s core (uleb/sleb, `.debug_info` with `.rela.debug_info` applied).
* `build.py`: `ElfBuilder` (sections, symbols, relocs, comment, version) and `DolBuilder` for fixtures - the 13 + 7 builders.
* From: 26 ELF readers, 10 DOL readers, the objdump parsers, `dwarfmap`.

### `lib/ppc.py` - instructions

* `decode(word, address) -> Insn` (opcode class, fields, `is_branch`, `target`, `is_call`, `is_load/store`, `d_form`, `rlwinm alias`,
  record form, fused multiply-add); `branch_target(address, word)`; `materialisations(insns)` (`lis`+`addi`/`ori`, sda bases);
  `scan_refs(code, base, sda13, sda2, fn_starts) -> refs` (the `splitcheck.scan_refs` semantics: loads, stores, calls, pool literals
  by loads only); `looks_like_prologue`, `is_dead_epilogue` (phantom); `find_sda_bases`.
* From: `splitcheck` (the reference), `phantom.index_refs`, `infer.Insn`, `dossier.decode_li`, `vtableaudit.is_code_pointer`,
  `callers.branch_target`.

### `lib/refs.py` - who references what

* `RefIndex.build(tree, source in {objects, asm, both})` address-keyed, cached with `lib.cache`: `query(address) -> [Ref(site,
  kind in {call, branch, addr, load, store, data-word, pointer}, func, unit)]`, `readers_of(section, lo, hi)`, `runs_over(range, step)`
  (the `.sdata2` seam view), `census(units) -> [Record(unit, section, address, name, size, sites)]` (`datagap.census`, the one
  relocation census over target objects), `callers_of(func)`, `callees_of(unit)`.
* Names are resolved per query through `lib.project.SymbolMap` (the dump's labels are never trusted - `callers`' rule).
* From: `callers.build_index/build_elf_index/query`, `datagap.census`, `splitcheck.Ctx` readers, `tudiscover.build_graph`,
  `accessextent.object_sites/repair_sites`.
* As built (WP2a): the index stays the cache's JSON dict with functions over it (`build_dump_index`, `build_object_index`,
  `load_index`, `rows_at`, `runs_over`), because four tools read the dict's keys and a typed `RefIndex` would change the
  cache format; the class arrives with 3c, which rewrites those readers. `spec/lib-refs.md` "Known gaps" lists the rest.
* As built (WP3c): `RefIndex.load(root)` wraps that dict (the dump when the tree reads one, else the objects) with the map
  (`RefMap`) and `callers`' query, readers and runs, moved verbatim; `accessextent`, `dataclaim` and `dataclosure` read it
  instead of importing `callers`. Names resolve through `RefMap` over `Ownership` with
  `lib.project.ownership.owner_label` (was `callees.classify_owner`). The asm dump's stamp is `DumpStamp` here, not on
  `lib.cache.Stamped`: the dump is this module's input, and `Stamped`'s envelope would turn every existing `.stamp.json`
  `unstamped`. The tree-level census (`census_claims`, `census_symbols`, `tree_census`) moved here from `dataclosure`, so
  `poolseams` stopped importing `datagap` (the cycle). `Ref` per row and `callers_of`/`callees_of` are not built: no reader
  asked for them.

### `lib/report.py` - scores

* `Report.load(path)`, `unit(name)`, `functions(unit) -> {symbol: score}` with the rule *no `fuzzy_match_percent` key = 0 %*,
  `measures(unit)`, `denominators()`, `arithmetic_check(unit)` (the sum reproduces the unit percent).
* `score(target, base, unit_name, tmpdir) -> Report` = `objdiff report generate` on a one-unit project (`unitutil.report_functions`,
  the one metric); `diff_rows(target, base, symbol)` for instruction rows (never a score); `objdiff_cli(tree)`.
* `Freshness` (`freshguard`): `unit_reasons(src, obj, tree)`, `report_reasons`.
* `regression(before, after, allow, eps) -> (unauthorised, authorised)` over two `snapshot`s - **one** regression rule
  replacing `land.report_regressions` (WP2b: the gate's `allow` split is the return shape the gate already consumes);
  `reportdiff.diff_*` became `diff_units/diff_symbols/diff_denominators` here, and folding `reportdiff`'s verdict and
  `measure.moved_summary` onto `regression` is WP3b (it changes their output; `applysplits` retired).
* From: 16 readers, `unitutil.report_*`, `freshguard`, `reportdiff`.
* As built (WP3b): `reportdiff`'s verdict is `compare` and `measure`'s baseline count reads the same `moved`/`direction`;
  `regression` stays the gate's own policy over `snapshot`s (folding the two tools onto it would import its blind spots,
  `spec/lib-report.md` Known gaps). `project_diff` (a tree's `diff -p . -u`) and `retry_transient` joined the scoring
  half.

### `lib/units.py` - the unit

* `Unit.resolve(spec, tree)` accepting every spelling in `duplication.md` (g); properties `key` (`Pl/pl_act`), `source`
  (ext inferred from `src/`, MAIN fallback), `obj_target`, `obj_ours`, `report_name`, `splits_key`, `lib`, `flag`, `slug`,
  `module`, `language`; `list(tree)`; `compile_command(unit, tree)` (`unitutil.compile_command` + `recompile.unit_tokens` with the
  worktree-includes-first rewrite); `compile(unit, tree, dry_run) -> CompileResult` (fresh-object assertion - `recompile.compile_unit`);
  `proposal_target(unit, symbol)` (`recompile.proposal_target`).
* From: the 30 name functions, `unitutil`, `recompile`.
* As built (WP3b): `compile` returns `CompileResult`, a frozen value that still reads as the old mapping; the flag tools'
  command-line helpers (`split_command`, `override_flags`, `with_compiler_version`, ...) and an object's function frames
  (`frames`, `function_names`, `text_size`) moved here from `unitutil`, which delegates.

### `lib/objcompare.py` - target vs ours

* `sections(target, ours) -> [SectionGap]` (sizes, first byte, count, permutation class - `datagap.compare_sections`,
  `sectiongap.compare_objects`, `flipcheck.section_byte_problems/mislaid_*`); `symbols(target, ours, threshold) -> size-gap/missing/extra`
  (`pairgap.compare`); `relocs(target, ours, by_owner) -> four classes` (`relocdiff.diff_relocs/compare_by_owner`);
  `undefined(ours, target, providers, map, linker) -> hits` (`undefrefs.unresolved_names`, `relocaudit.audit_sets`);
  `fingerprint(obj)` (`datagap.object_fingerprint`, `verifyunit.target_object_fingerprint`).
* Each returns `Finding`s; the CLIs (`datagap`, `sectiongap`, `pairgap`, `relocdiff`, `flipcheck`, `undefrefs`) render them.
* As built (WP3a): the comparisons return value types (`SectionGap`, `SymbolGap`) and the dict shapes the tools already
  rendered (`reloc_facts`, `object_sections`, the four relocation classes); `Row`s arrive with the gate (WP4). `fingerprint`
  is two functions because it is two questions (`fingerprint`: target drift, rename-insensitive; `touch_fingerprint`:
  TOUCHED, external targets by address), both byte-compatible with the stored snapshots. The link-input index
  (`link_index`) moved here too, so `flipcheck` and `undefrefs` share one cached read of the link (and `flipcheck` no
  longer runs binutils).

### `lib/cscan.py` - C/C++ text

* `strip_comments` (length-preserving), `match_brace`, `match_paren`, `statements`, `declarations(text)` (function prototypes,
  `extern`, with declarator parsing), `type_definitions(text)` (struct/class/union/enum, fields with offsets, size comment),
  `includes(text)`, `include_closure(path, roots)`, `pragmas`, `calls(text, name)`.
  As built (WP2c): one lexer (`spans`) behind `strip`/`strip_comments`/`remove_comments`; `declarations` is two
  functions, `function_declarations(Text)` (stylelint's scope walk) and `declared_names(clean)` (typeregistry's names),
  because their callers ask different questions; `include_closure` takes a `resolve(name, includer)` callable rather
  than roots, since declclash and stylelint search different bases. `statements`, `type_definitions`, `pragmas` and
  `calls` wait for 3b/3d (the spec's Known gaps).
* From: `stylelint` (`strip`, `match_brace`, `struct_defs`, `iter_fields`, `function_declarations`, `_resolve_include`),
  `typeregistry` (`strip_comments`, `extract_decls`, `_match_brace`), `vtableaudit.type_definitions/_members`, `declclash.closure/shape`,
  `recordmerge.parse`, `methodize._call_sites`, `shapes.strip_comments/match_brace/split_statements`, `freshguard.source_closure`.

### `lib/findings.py` - the one shape

* `Finding(rule, file, line, token, detail, remedy=None)` with `identity()` (file-independent for the add-only diff),
  `Row(name, status in {PASS, FAIL, UNKNOWN, SKIP}, detail, evidence, remedy, kind)`, `Verdict(rows) -> ok, failed_kinds, summary`,
  `added(before, after, credits)` (the `stylelint.added_identities` + rename/move credit model), `render_table`, `render_json`
  (one schema: `{"tool", "rows": [...], "ok", "summary"}`), `exit_code(verdict)` with the one convention: 0 ok, 1 findings, 2 could not run.
  As built (WP2c): `Finding` also carries `text` (the source line - stylelint's JSON always had it), and `credits` is one
  `credit(f) -> other spellings` callable, so the rename map and the file map stay with the tool that owns them.
* From: `land.check`, `stylelint._finding/diff_deltas`, `vtableaudit.violation_rows`, `undefrefs.check_object`,
  `datagap.strict_verdict`, `splitcheck.Results`, `flipcheck.check`, `verifyunit.*_problems`, `symbolpreflight.severity_for`,
  `dataclaim.classify`, `handoff.validate`, `unionresolve.check_union`.

### `lib/cli.py` - the entry point

* `Tool(name, spec)`: builds the parser with the common flags (`--json`, `--root`, `--main`, `--limit`, `--dry-run`, `--quiet`),
  maps a `Verdict`/exception to the exit convention, prints JSON on `--json`, and registers the tool for `tools/selftest.py`'s
  inventory (`tool.tests = "tools/tests/units/test_land.py"`). No tool defines `--selftest`; the shim keeps the flag forwarding to
  the test module until the docs are swept.

### `lib/testing.py` - the harness

* `Checker`: `check(name, got, want)`, `expect`, `raises`, `contains`; prints one shape (`ok - N checks` / `FAIL: name: got != want`)
  so the runner needs one regex, not five.
* `FixtureTree(tmp)`: a fake repository the lib accepts as a tree - `configure.py` (real Python), `config/RMHE08/{symbols,splits}.txt`,
  `src/`, `include/`, `build/RMHE08/{obj,src}/` objects from `ElfBuilder`, a `report.json`, an optional asm dump; helpers
  `add_unit(...)`, `add_symbol(...)`, `claim(...)`. It is the only way a fixture test builds a tree, and it is created outside the
  repository (the `unitscore_selftest` incident).
* `GitFixture(tmp)`: `init`, `commit(files, message)`, `branch`, `worktree`, `conflict(a, b)` for the landing/merge/lane tests.
* Tiers: a test module declares `TIER = "fixture"` (default; importing `lib.repo.repo_root()` without `start=` fails the run) or
  `TIER = "smoke"` (may read the live tree; every check must be of the form "skipped when absent" or "shape, not count").
* From: 25 `check()`s, ~22 fixture builders, the delegation-pair logic in `selftest.py`.

### `lib/outbox.py` - the outbox

* `CONFIG_REQUEST_SCHEMA` (from `handoff`), `load_outboxes(dir)`, `load_notes(dir)`, `Entry` with tolerant accessors
  (`text(field)`, `requests()`, `units()`), `validate(entry, owned) -> [Finding]`.
* From: `handoff`, `backlog.build_items/_add_request`, `tooling.load_sources/candidates_for`, `playbook.extract_findings`,
  `slots._merge_data_requests`, `land.outbox_units`, `splitcheck.seam_requests`.

### `lib/lanes/` - the lane model

* `naming.py`: `slug(unit)`, `branch_for`, `worktree_for`, `norm_unit` (one copy; `claims.norm_unit` and `promote.norm_unit` today).
* `registry.py`: `.pi/claims.json` records, `.pi/slots/*.json`, locks, the `.used` marker; `live_runs()` from `~/.claude/sessions`.
* `rescue.py`: `rescue_ref(branch)`, `make_rescue(branch, base)`, `verdict(ref)` (`rescue.classify_ref`).
* `teardown.py`: **one** teardown sequence (rescue -> unlink reparse points -> worktree remove -> branch delete -> prune ->
  registry) with step reporting, used by `claims release`, `lane teardown`, `slots release/reclaim`.
* `launch.py`: `lane_call`, `resume_call`, `profile_for_kind` (`lanecmd`, `slots.profile_for_kind`, `queue._profile_for_kind`).

## 4. The thin tools (what each becomes)

The CLI names, subcommands and flags the profiles/skills/docs use stay (section 8). "Lib" lists what the tool now imports;
"keeps" is what stays tool-specific.

| tool | verdict | lib it stands on | keeps / notes |
| --- | --- | --- | --- |
| `units/land.py` | SPLIT | findings, git, project, report, units, objcompare, outbox, lanes | a ~400-line CLI over `tools/units/landing/` (section 5) |
| `units/stylelint.py` | SPLIT | cscan, project.Ownership, findings, git | `landing`-independent; rules as `tools/units/stylelint_rules/r01..r13.py` |
| `units/vtableaudit.py` | KEEP (thin) | binary, project, cscan, findings | the run rule and the `+0x00` store scan |
| `units/undefrefs.py` | MERGE (absorbs `relocaudit`) | objcompare.undefined, cache, findings | `--census` is relocaudit's sweep |
| `units/datagap.py` | SPLIT | objcompare, refs.census, project, report, findings | CLI `datagap` (section gap) + `dataclosure` module for the strict/span/fold/snapshot rows the gate uses |
| `units/sectiongap.py`, `objdiff/pairgap.py`, `objdiff/relocdiff.py` | KEEP (thin) | objcompare, findings | renderers only |
| `units/flipcheck.py` | KEEP (thin) | objcompare, project, binary, findings | the `   - ` reason lines stay verbatim (lanes parse them) |
| `units/verifyunit.py` | KEEP (thin) | report, project, binary, findings | registration, independent re-measure, drift |
| `units/langcheck.py` | KEEP (thin) | binary, names, project.configure | the three-signal classifier |
| `units/ledger.py` | KEEP (thin) | project, report | totals / next / unit |
| `units/recompile.py`, `units/measure.py`, `objdiff/unitscore.py`, `objdiff/symdiff.py` | KEEP (thin) | units, report, repo | one metric already; drop their private resolvers |
| `units/callers.py`, `units/callees.py`, `units/accessextent.py`, `units/dossier.py` | KEEP (thin) | refs, ppc, binary, project, names | the abstract interpreter stays in `accessextent` |
| `splits/tudiscover.py`, `splits/dataorder.py`, `units/dataseams.py`, `units/poolseams.py` | MERGE into `tools/splits/seams/` | refs, binary, project, cache, findings | the four evidence kinds computed once; CLIs kept as entry points. As built (WP3c): the entry points stay at their paths and import `tools/splits/seams/evidence.py` (the data order, the pool literal and value-witness rules, the source-name rule, `components`, the readers); the evidence *readers* stay per tool (dump graph, retail decode, census) - `spec/seams.md` |
| `splits/splitcheck.py` | SPLIT | ppc (its scanner moves out), project, findings | keeps `--baseline` (the phase-5 audit) as invariants over `Row`s; proposal rendering/lint retired with the program. As built (WP3c): `tools/splits/invariants/` (13 modules), `splitcheck.py` 108 lines, `Results.rows()` renders `lib.findings.Row`s - `spec/invariants.md` |
| `splits/applysplits.py`, `splits/dataattach.py`, `splits/matchinggain.py` | RETIRE | - | the program is applied (`retired.md`) |
| `units/attribute.py` | RETIRE (question 1) | - | its `queue` is superseded by split-proven units in `splits.txt` |
| `symbols/symedit.py` | KEEP (thin) | project.symbols, text, git | the CLI over `SymbolMap` |
| `symbols/dumpmap.py`, `symbols/phantom.py`, `units/mangle.py`, `units/methodize.py` | KEEP (thin) | project, binary, ppc, names, cscan | - |
| `units/symbolpreflight.py` | KEEP (thin) | project.Ownership | its parsers go; `preflight_selftest` becomes a fixture test |
| `units/dataclaim.py`, `units/dataqueue.py` | KEEP (thin) | project, refs, binary, report, text | - |
| `units/unwindcut.py`, `units/vtslot.py`, `units/linkorder.py`, `units/m2cinput.py` | KEEP (thin) | binary, project | - |
| `units/typeregistry.py`, `units/declclash.py`, `units/recordmerge.py` | KEEP (thin) | cscan | - |
| `units/claims.py`, `units/slots.py`, `units/lane.py`, `units/rescue.py`, `units/wtsafe.py`, `units/queue.py`, `units/lanecmd.py`, `units/worktreehook.py` | SPLIT into `lib/lanes` + thin CLIs | lanes, git, repo, text | `herdr` pane code deleted; one teardown; `worktreehook` stays a prototype (question 4) |
| `units/brief.py` | SPLIT | project, report, outbox, units, lanes | `brief/render.py`, `brief/proposal.py`, `brief/pool.py` |
| `units/backlog.py`, `units/tooling.py`, `units/playbook.py`, `units/handoff.py` | KEEP (thin) | outbox, findings, text | - |
| `units/mergebranch.py`, `units/unionresolve.py`, `units/unionguard.py`, `units/unionprose.py` | MERGE into `tools/units/merge/` | git, text, project | one union rule, one check_union, three entry points. As built (WP3f): one module per tool spec (`merge/unionprose.py` the rule, the diff3 hunk reader and the superset `cover`; `merge/unionresolve.py`; `merge/unionguard.py`; `merge/mergebranch.py`), so each module's header names an existing spec; the four old files are entry-point shims forwarding every name; the package is imported as `import tools.units.merge.<m> as x` (no edge to `merge/__init__.py`) |
| `git/commitlint.py`, `git/guard.py`, `git/prepcommit.py`, `git/hooks/pre-commit` | KEEP (thin) | git, text, report | - |
| `agents/edit.py`, `units/escape.py`, `units/checklf.py` | MERGE into `agents/edit.py` | text, git | `checklf`'s check = `edit.py check --blob`; `escape --edit` = `edit replace --old/--new`. As built (WP3f): `escape.encode/decode` are `lib.text.c_escape/c_unescape` (what `--old/--new` read), `escape --edit` is `lib.text.replace_bytes`, `checklf.py` a shim importing `edit` |
| `agents/ideas.py`, `agents/ideas_demo.py`, `agents/sync_playbook_index.py`, `agents/sync_profiles.py`, `agents/profileprobe.py`, `agents/install.sh` | KEEP | text, proc, binary (demo evaluate) | - |
| `elf/objalign.py`, `elf/objextab.py` | KEEP (thin) | binary.elf (read/write), project.splits | ninja steps; byte-neutral by construction stays a test |
| `elf/elfsect.py`, `elf/dwarfmap.py` | KEEP (thin) | binary | `elfsect.sections` becomes a one-line wrapper (the skill imports it) |
| `flags/*` | KEEP (thin) | units, report, binary, ppc | `infer`'s decoder moves to `lib.ppc`; `variants/camellia.py` stays as data |
| `objdiff/slotmap.py`, `objdiff/freshguard.py` | KEEP / ABSORB | report | `freshguard` becomes `lib.report.Freshness`; `freshguard.py` is a shim then gone |
| `units/reportdiff.py` | ABSORB into `lib.report.regression` + a `reportdiff` CLI kept | report | no caller today; the gate's regression row uses the lib |
| `units/subproc.py`, `spawnretry.py`, `selftest_site/` | ABSORB into `lib.proc` | - | `selftest_site` stays (the runner's PYTHONPATH shim) |
| `units/sharedfiles.py` | ABSORB into `lib.text` + `lib.project.splits` | - | - |
| `unitutil.py` | ABSORB into `lib.repo` + `lib.units` + `lib.report` | - | kept as a shim re-exporting the old names while `mt.py` and 40 importers migrate |
| `selftest.py` | KEEP (rewritten discovery) | proc, repo, testing | discovers `tools/tests/**`, keeps `--changed`, `--json`, `--list`, the park list |
| `mwlink_debugger.py` | SPLIT into `tools/mwlink/` | binary (elf, pe), proc, project, units | a package: `catalogue.py`, `mapfile.py`, `anchors.py`, `trace.py`, `link.py`, `align.py`, `records.py`, `cli.py`; `mwlink_debugger.py` stays as a shim |
| `mwcc-debugger/*` | KEEP | proc, binary (pe) | `versions.py` is already the data/mechanism split; `make_port.py` stays for provenance (and regenerates the port byte for byte - a test) |
| `rso/*` | KEEP (dormant) | binary, project | spec marks it dormant |
| `project.py` and the template scripts | UNTOUCHED | - | dtk-template; only the `objalign`/`objextab` chain is ours |
| `units/promote.py`, `units/promote_batch.py`, `splits/gen_trk_vectors.py`, `flags/infer-run.md`, `units/relocaudit-findings.md`, `units/relocaudit.py`, `units/checklf.py` | RETIRE | - | `retired.md` |

## 5. Decomposition of the seven large files

**`units/land.py` (5 172 lines) -> `tools/units/landing/`** (the CLI `land.py` keeps the four subcommands):

* `base.py` - `record-base`: the snapshot (`.pi/land-base.json`: head, dirty paths, report snapshot, target fingerprints,
  undefrefs base, data-closure snapshot) through `lib.report`, `lib.objcompare.fingerprint`, `dataclosure.snapshot`.
* `rows/` - one function per gate row returning `Row`, grouped: `tree.py` (ground truth, main moved, batch paths, conflict markers,
  foreign paths), `batch.py` (outbox validates, commits on branch, units named, registration), `rules.py` (stylelint diff, rule 10
  diff, rule 12 verdict, band-ownership warning, rule-7 defer growth), `build.py` (configure, ninja scoped compile, `ok` recreated),
  `objects.py` (undefined references, flipcheck READY, target drift, per-symbol re-measure, size gaps), `data.py` (data-closure
  rows), `regression.py` (one lib rule), `selftests.py` (the suite row), `subject.py` (commitlint), `knowledge.py` (playbook delta).
  Each row *calls* the lib or the tool's API; none shells out and parses text.
* `stage.py` - `land_stageable`, `looks_already_applied`, `stage_batch`, `commit_pathspec`, `land_decision`.
* `branch.py` - `apply_branch`, `units_from_branch`, `resolve_conflicts` (through `tools/units/merge/`), the resolve-helper refs.
* `renames.py` - `--unit-rename` pairs and snapshot-key renaming.
* `release.py` - the release plan through `lib.lanes.teardown`.
* Tests: `tools/tests/units/test_landing_*.py` on `GitFixture` + `FixtureTree`; the 1 875-line `selftest()` is split along the rows.

**`units/stylelint.py` (4 924) -> `tools/units/stylelint.py` + `stylelint_rules/`**: `rules/r01_shared_type.py` ...
`r13_method.py` (each `findings(source, ctx) -> [Finding]` with its exemption marker logic), `context.py` (sources, ownership,
rule-13 include context), `diff.py` (`--diff REF`: added/removed identities, rename credits, move credits, deleted files), `refs.py`
(at-ref loading via `lib.git`), `report.py` (budget, rule-2 report). Lexing is `lib.cscan`; ownership is `lib.project.Ownership`.

**`units/slots.py` (3 384) -> `lib/lanes/registry.py` + `tools/units/slots.py`**: pool manifest, locks, markers, live runs and
`slot_state` move to the lib (they are read by `claims`, `queue`, `worktreehook`, `selftest`); `seed.py` (the build seeding that
`claims.seed_worktree_build` also implements - one copy) and `reclaim.py` (merge-tree verdicts) become modules; `spawn` uses
`lib.lanes.launch`; the 1 132-line selftest becomes fixture tests on `GitFixture`.

**`units/claims.py` (2 847) -> `tools/units/claims.py`**: `claim`/`list`/`release`/`ack`/`status`/`timeout`/`expire` over
`lib.lanes`; `release` is `lanes.teardown.run(steps)`; the herdr pane probing (50 references) is deleted; `_seed_*`/`_ninja_deps_*`
(220 lines) go to `lanes/seed.py`; `rescue_verdict` is `lanes.rescue.verdict`.

**`units/datagap.py` (2 816) -> `datagap.py` + `tools/units/dataclosure.py`**: the section-size gap and `--flip-blockers` stay in
`datagap.py` on `lib.objcompare`; the census is `lib.refs.census`; `strict_report`/`judge_blocks`/`splits_plan`/`fixpoint_plan`
(the span/fold/strict verdicts), `snapshot_orphans`/`batch_orphans`/`touch_verdicts`/`fold_snapshot` (the gate's rows) become
`dataclosure.py` with `Row` outputs; `poolseams.py` stays a sibling on the same census.

**`units/brief.py` (2 479) -> `tools/units/brief/`**: `sources.py` (map rows, scores, header comment, flags, shared headers,
plan sections - all through the lib), `render.py` (the six parts), `proposal.py` (the proposal brief), `pool.py` (`--pool`,
stamps, prune, promoted litter), `cli.py`. `dossier` stops importing `brief` (the cycle) by rendering through `render.py`'s
public function.

**`mwlink_debugger.py` (3 743) -> `tools/mwlink/`**: the PE reader shared with `mwcc-debugger` is `lib.binary.pe`, not a
module of the package (WP5: `versions.py` importing `tools/mwlink/pe.py` would be a new tool->tool edge, and the
`mwcc-debugger` spec already named `binary (pe)`); `catalogue.py` (RT_STRING messages, phases), `mapfile.py` (the map
parser, `verify`, the order check - shared by `trace` and `verify`, so not inside either), `anchors.py` (derive/prove,
the phase table), `trace.py` (`MwObject`, `build_trace`, `render_trace`), `link.py` (rsp derivation, `run_link`,
`report_link_failure`), `align.py`, `records.py`, `cli.py`; ELF reading via `lib.binary.elf`; the 336-line selftest
becomes fixture tests in `tools/tests/mwlink/` plus the real-linker checks in `tools/tests/smoke/test_mwlink_live.py`.
An import inside one tool package is not a tool->tool edge (`test_layering.package_of`), and a package module's header
names the package's spec (`test_headers`).

Also split: **`splits/splitcheck.py` (3 578)** -> `lib.ppc.scan_refs` + `lib.refs` (its `Ctx`), `tools/splits/invariants/*.py`
(order, coverage, text-cut, extab, ctors, pool, data-order, vtable, jumptable, bss, local-static - each a `Row` producer), the
`--baseline` CLI; proposal loading/linting/rendering (`load_proposal` ... `render`, ~900 lines) retired with the program.
As built (WP3c): each invariant module keeps its `check_<name>(ctx, res)` signature and accumulates into the one `Results`
store, which renders the `Row`s (`Results.rows()`, written under `rows` by `--json`); `Ctx` lives in `invariants/context.py`;
`audit.py` is the package's API; the in-file selftest is `tools/tests/splits/test_splitcheck.py`.

## 6. The rule / finding / gate-row framework

* `Finding` is a lint-style item (`rule`, `file`, `line`, `token`, `detail`, `remedy`); `Row` is a gate-style item (`name`, `status`,
  `detail`, `evidence`, `remedy`, `kind`). A tool returns a list of either; `Verdict` folds them.
* **Add-only comparisons** (`stylelint --diff`, `vtableaudit --diff`, `undefrefs` base subtraction, `datagap --touched-by`) use one
  `added(before, after, credits)` with identity functions per rule and the rename/move credit model `stylelint` already has.
* **Exit codes**: 0 = ok, 1 = findings/refusal, 2 = could not run (missing input, no build, usage). `--json` prints the one schema.
* **The gate** composes rows by calling functions; `land.py verify` output stays the same table (profiles read it) and gains the JSON.
* `splitcheck --baseline` invariants, `flipcheck` reasons, `datagap` strict verdicts, `dataclaim` verdicts and `symbolpreflight`
  severities all become `Row`s with their existing wording preserved as `detail` (the lanes' parsers keep working).

## 7. Test strategy

* **Re-homing.** Every `*_selftest.py` and every in-file `selftest()` moves to `tools/tests/<area>/test_<tool>.py` (89 runner entries
  -> ~95 test modules, one per tool plus the lib modules). A module is a `Checker` script: `python tools/tests/units/test_land.py`
  runs it alone; `tools/selftest.py` discovers the directory (no `--selftest` flag scan, no delegation regexes, one count regex).
  `tools/selftest.py --changed` maps `tools/lib/<m>.py` -> `tests/lib/test_<m>.py` + every tool test that imports the module
  (an import scan, not a hand table) and keeps `SOURCE_ENTRIES` for the docs sources.
* **What becomes a lib contract test**: the parsers (`symbols`, `splits`, `configure`, `Ownership`), `binary.elf/dol` round-trips
  with the builders, `ppc.decode` against hand-assembled words, `refs` on a `FixtureTree` with an asm dump and objects,
  `report` (the 0 % rule, arithmetic, regression), `units` (every spelling), `findings.added` (the credit model), `text` (endings,
  replace, Transaction restore), `git` on a `GitFixture`, `proc` (codec, trap scan over `tools/`), `cache` states, `names`.
* **What becomes a fixture test of a tool**: everything that today builds its own fixture (the 22 builders) - re-expressed on
  `FixtureTree`/`GitFixture`/`ElfBuilder`/`DolBuilder`; the four breakages become impossible because a fixture test cannot
  resolve the live tree (`lib.repo.repo_root()` without `start=` raises under `TIER="fixture"`).
* **The smoke tier** (`TIER="smoke"`, `tools/tests/smoke/`): `preflight`'s derived rows (they *are* a check of the live data, but as
  "every derived pair agrees with Ownership", never a named symbol), `accessextent`'s acceptance run, `dataorder`'s whole-DOL
  scan (`> 0` seams, not `> 30`), `stylelint`'s real-map resolution ("resolves to *a* registered unit"), `infer`'s accuracy table
  (reported, not asserted, until the parked detector is fixed), the sync tools' real-tree drift checks (these must stay strict:
  they are the point), `metric`/`measure`/`verifyunit` integration rows. Smoke runs in the gate with "skip if the input is absent"
  and never pins a count; `selftest.py --tier fixture` is the fast run.
* **The park list** stays as is (one entry today).

## 8. Compatibility promises

Every invocation found in `.claude/agents/*.md`, `.claude/skills/**`, `CLAUDE.md`, `docs/pipeline.md`, `docs/plan.md`
(the list in `migration.md`, section "CLI compatibility") keeps its path, subcommand, flags, exit code and - where a lane parses
it - its text (`flipcheck`'s `   - ` lines, `land.py verify`'s table, `claims.py` step lines, `queue.py next`'s spawn block,
`symedit rename`'s one-line diff). Additions are allowed; removals wait for `migration.md` WP6's reference sweep and happen in
one commit with the docs. `--selftest` keeps working as a forwarding shim until that sweep. `unitutil` keeps its names as a
shim because `mt.py` (the skill) and 40 files import it.

## 9. Naming and style

* Module names are nouns for concepts (`report`, `refs`, `findings`); tool names stay what the docs call them.
* Value types are `@dataclass(frozen=True)`; public lib functions carry type hints and a one-paragraph docstring that states the
  contract (not the history); private helpers may have none.
* No module-level `ROOT` constants in tools; a tool receives a `Tree` from `lib.cli` (`--root`/`--main`) and passes it down -
  fixtures then never need to monkeypatch a global (`dump_asm_selftest` today).
* The header template: `"""<one-line purpose>. Spec: docs/tools/spec/<name>.md. CLI: <shape>."""`; a lint
  (`tools/tests/lib/test_headers.py`) refuses a tool whose module docstring has more than three lines or that lacks the spec path.
* Line endings: lib writes LF; `lib.text` preserves a file's own ending on edit.

## 10. Risks and rollback

| risk | mitigation / rollback |
| --- | --- |
| the gate is refactored by the gate | `land.py` is decomposed last (WP4); until then the old file is the gate and the new rows are tested against the same fixtures; a package lands only after `land.py verify --dry-run` on a no-op batch and a real trivial landing both pass |
| a lib behaviour differs from one of its N sources | each absorption lists its sources' selftests and keeps them green on the lib before the source is deleted (extract-and-delegate: the old function becomes `return lib.x(...)` first) |
| Windows: paths, `cp1252`, junctions, `WinError 5` | `lib.proc`/`lib.text`/`lanes.teardown` carry those rules once with fixture tests on Windows; the suite runs on this host |
| a shim is forgotten and a doc breaks | the compatibility list is a test (`tests/smoke/test_cli_compat.py` runs every documented invocation with `--help` or `--dry-run`) |
| import cycles reappear | `tests/lib/test_layering.py` fails on `lib -> tools` or `tool -> tool` imports outside the allowed list (imports inside one tool package - a `tools/<pkg>/` with `__init__.py`, such as `tools/mwlink/` - are that tool's own structure, not edges) |
| performance regressions (the gate budget is ~30 s for selftests) | the runner prints per-test durations; a package's acceptance records the suite time before/after |
| the splits-program tools are needed again | retired code stays in git history; `retired.md` names the commit and the replacement |
| rollback | each work package is one landed batch with its own commit; reverting it restores the shims because the old entry points are only deleted in WP6 |

## 11. Effort and order

Sizes: S = under a lane-day, M = one to two, L = three or more. Dependencies and the parallel groups are in `migration.md`;
the summary:

| WP | content | size | depends on |
| --- | --- | --- | --- |
| 0 | package layout, the one-line prologue + lint, `lib.testing`, runner discovery of `tools/tests/`, header lint | M | - |
| 1a | `lib.repo`, `lib.proc`, `lib.git`, `lib.text`, `lib.cache`, `lib.names` | M | 0 |
| 1b | `lib.binary` (elf, dol, objdump, dwarf, builders) | M | 0 |
| 1c | `lib.project` (symbols, splits, configure, ownership) | M | 0 |
| 2a | `lib.ppc`, `lib.refs` | L | 1b, 1c |
| 2b | `lib.report`, `lib.units` | M | 1a, 1c |
| 2c | `lib.findings`, `lib.cli`, `lib.outbox`, `lib.cscan` | M | 1a |
| 3a | object-comparison family (`objcompare` + datagap/sectiongap/pairgap/relocdiff/flipcheck/undefrefs+relocaudit/verifyunit) | L | 1b, 2b, 2c |
| 3b | score family (recompile/measure/unitscore/symdiff/freshguard/reportdiff; flags/*) | M | 2b, 2c |
| 3c | seams + refs family (callers/callees/accessextent/dossier; tudiscover/dataorder/dataseams/poolseams; splitcheck --baseline) | L | 2a, 2c |
| 3d | symbols family (symedit/dumpmap/phantom/mangle/methodize) + source scanners (stylelint split, typeregistry, declclash, recordmerge, vtableaudit) | L | 1c, 2c |
| 3e | lanes family (`lib.lanes` + claims/slots/lane/rescue/wtsafe/queue/lanecmd/worktreehook) + brief split | L | 1a, 2b, 2c |
| 3f | merge family (mergebranch/unionresolve/unionguard/unionprose) + git tools (commitlint/guard/prepcommit) + edit/escape/checklf | M | 1a |
| 4 | the landing gate decomposition | L | 3a, 3b, 3d, 3e, 3f |
| 5 | debuggers as packages (`mwlink/`, `mwcc-debugger`) | M | 1b |
| 6 | retirements, `--selftest` shim removal, `unitutil` shim removal, docs/profile/skill reference sweep, headers everywhere | M | 4 |

Parallel groups: {1a, 1b, 1c}; {2a, 2b, 2c}; {3a, 3b, 3c, 3d, 3e, 3f, 5}; then 4; then 6.
