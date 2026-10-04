# `doclinks` - Check that every relative markdown link in the docs resolves, and flag root-absolute ones

## Purpose

Reads every markdown file under `docs/`, `.claude/` and `CLAUDE.md`, finds each inline link, image and reference
definition outside code, and reports a relative target that does not exist or a root-absolute (`/path`) target, with
file and line.

## Users

The suite (`tools/tests/smoke/test_doclinks_live.py`); anyone moving or renaming a doc.

## CLI

```
python tools/agents/doclinks.py                 # the whole tree
python tools/agents/doclinks.py docs/plan.md    # named files only (relative to --root)
python tools/agents/doclinks.py --root <tree> --json
```
Flags: `--root`, `--json`. Exit codes: 0 every link resolves, 1 at least one finding.
`--json`: `{files, links, findings: [{file, line, target, reason}]}`.

## Inputs and outputs

Reads the markdown files and stats the link targets. Writes nothing.

## Invariants and rules

* A target with a scheme (`https:`, `mailto:`), a protocol-relative `//host` or only an `#anchor` is not a repository
  path and is not checked; the `#anchor` and `?query` of a path are dropped (anchors are not verified), `<...>` and
  `%20` spellings are decoded.
* A path resolves relative to the linking file's directory; a target starting with `/` is a finding whatever it points
  at (GitHub and the editors resolve it differently - write it relative).
* Fenced blocks and single-line code spans carry no links: a path quoted as code is prose, and most of this
  repository's docs quote paths in backticks rather than linking them (228 links in 375 files on 2026-10-04, 0 broken).
* `.claude/worktrees/` is skipped: it holds other checkouts of the repository, not its docs.

## Lib dependencies

`lib.repo` (the default root).

## Test contract

Tier: fixture (`tools/tests/agents/test_doclinks.py`: a `FixtureTree` with good, broken, root-absolute, anchor, URL,
mailto, titled image, `<spaced>`, `%20`, `../`, code-span, fence and reference-definition links, the worktrees
exclusion and both CLI exits; treating every path as resolving fails 2 checks) and smoke
(`tools/tests/smoke/test_doclinks_live.py`: the live tree has no finding - a broken link added to a doc fails it).

## Known gaps

* Anchors are not checked against the target's headings, and links in HTML (`<a href>`) are not read.
