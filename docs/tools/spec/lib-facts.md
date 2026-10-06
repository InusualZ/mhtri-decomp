# `lib.facts` - Fact tokens and where they survive: the judgement factscheck and the comment sweep share

## Purpose

Extracts the fact tokens of a text (addresses, names, sizes), indexes the corpora a fact may live in once, and
answers, for a removed text, where each token survives - or that it survives nowhere. One implementation for
`factscheck.py` (a whole diff) and `sweepcomments.py --history` (one inherited header block).

## Users

`tools/units/factscheck.py`, `tools/units/sweepcomments.py`.

## CLI

None (library).

## Inputs and outputs

`Tree(root, ref=None)` reads the working tree (tracked plus untracked-not-ignored files) or a commit (`git ls-tree`,
`Git.show_many`). `Corpora(tree)` indexes `configure.py`, `config/RMHE08/splits.txt`, `config/RMHE08/symbols.txt`
(names, addresses and sizes through `lib.project.symbols`, never printed) and every text file under `docs/`. Writes
nothing.

## Invariants and rules

* `fact_tokens(text, tree)` -> `[(token, inert reason or "")]`: a hex value of four or more digits, a word with an
  underscore or camel humps and at least six characters, a decimal of three or more digits. A token inside a path
  the tree has is `path exists`; inside a date, a path glob, a name glob or a dropped provenance path
  (`auto/<hex>_`, `proposal/`, `src/auto/`, `docs/splits/phase4`) it is inert with that reason; `INERT_WORDS` likewise.
* `Tree.unit_text(rel)`: the new text of `rel` and its same-stem siblings (`UNIT_SUFFIXES`: a source and its own
  header), the "new text" a unit's removed fact may move to.
* `Corpora.index_changed(rels)`: one extra corpus per file the diff changed (its new text; the fixed corpora and
  `docs/` skipped, a deleted file has none) - where a token moved verbatim to another file of the same diff survives.
* `judge(text, new_text, corpora, own=None)` -> `[(token, where)]`, `where` the first holder in the order: the new text of the
  unit, `configure.py`, `splits.txt`, `symbols.txt`, `docs/**`, then `moved to <file>` for every changed file whose
  stem is not `own`'s; a generated stem whose eight-digit address any holder has
  (or the runtime dump's `zz_<7 hex>_`, the address less 0x80000000) is `derivable`; `""` means lost.
  `unmatched(...)` is the lost tokens, each once.
* Hex tokens compare by value; every eight-hex-digit run of a corpus (the address inside `fn_802D44F4`) is indexed as
  a value too.
* `Tree.exists(path)` accepts a path as written, below `src/`, or through `lib.repo.moved_header` for `include/...`.
* `parse_diff(text)` -> `[Span(file, line, text)]`: one span per run of removed lines of a `git diff -U0` hunk, the old
  file's line; an added file has none.
* `parse_added(text)` -> `{file: [(new line, text)]}`: every added line of a `git diff -U0` text (`lanecheck` reads it).

## Lib dependencies

`lib.git`, `lib.project.symbols`, `lib.repo`.

## Test contract

Tier: fixture, through `tools/tests/units/test_factscheck.py` (token classes, `parse_diff`, the corpora and the
mutation that moves a lost fact into a doc) and `tools/tests/units/test_sweepcomments.py` (the inherited-header
decision).

## Known gaps

* `src/` as a whole is not a corpus: a fact repeated in an unchanged unit's comment does not count as surviving;
  only the files the same diff changed are (`index_changed`).
