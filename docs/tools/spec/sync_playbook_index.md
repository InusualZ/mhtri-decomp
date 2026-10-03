# `sync_playbook_index` - Generate `docs/matching/index.md` from idea front matter; `--check` is a docs-batch gate row; its parser is the one every playbook tool imports

<!-- generated from the module docstring of `tools/agents/sync_playbook_index.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

sync_playbook_index.py - generate docs/matching/index.md from the front matter of docs/matching/NNN-slug.md.

## Users

the selftest runner (18); profiles (`.claude/agents`) (1); skills (8); CLAUDE.md (1); docs (5); imported by `ideas`, `playbook`

## CLI

```
python tools/agents/sync_playbook_index.py             # write docs/matching/index.md
python tools/agents/sync_playbook_index.py --check     # exit 1 when index.md is stale; write nothing
python tools/agents/sync_playbook_index.py --print     # print the generated index
python tools/agents/sync_playbook_index.py --where N   # print idea N's path (exit 1 when unknown)
python tools/agents/sync_playbook_index.py --json      # the parsed index as JSON
python tools/agents/sync_playbook_index.py --selftest  # fixtures: duplicates, bad keys, slug mismatch, ...
```
Flags: `--check`, `--json`, `--print`, `--repo`, `--selftest`, `--where`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: docs/matching/NNN-*.md -> index.md.

## Invariants and rules

* The matching playbook is one file per idea, `docs/matching/NNN-slug.md` (three-digit zero-padded, permanent id + a short slug), each opening with a front-matter block:
```
---
id: 43
title: <the idea's title>
status: works            # works | ruled-out | todo | superseded
problem: <the Problem sentence, single line>
tags: [flags, source-shape]
applies: []              # compiler versions/libs it is known to apply to, e.g. [Wii/1.3]
demo:                    # NNN-slug.cpp when a compilable demonstration exists
---
```
* `docs/matching/index.md` is GENERATED from those blocks (id, title, status, tags, problem; sorted by id, the problem capped at 220 characters). The tool refuses loudly - and writes nothing - on a duplicate id, a missing or malformed key, an unknown tag or status, a file name whose id/slug disagrees with its front matter, and a `demo` that names a missing file. Ids are permanent: 600+ citations of "playbook N" / "row N" exist in the tree.

## Lib dependencies

text, repo.

## Test contract

Tier: fixture; smoke: the real index is in sync (strict, by design).
Today's selftest (`tools/agents/sync_playbook_index_selftest.py`): The index is generated from the front matter of docs/matching/NNN-slug.md, so the tests are about the classes that must refuse (a duplicate id, a missing or malformed key, an unknown tag or status, a file-name/front-matter mismatch, a demo naming a missing file), about ordering and escaping in the generated table, about `--where` and `--json`, and about the real tree: it must be clean and index.md must be in sync.
Target: `tools/tests/agents/test_sync_playbook_index.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
