# Tools

Entry page for the tooling design and specs, which live one directory down (the same shape as
`docs/matching.md` -> `docs/matching/`):

* `docs/tools/README.md` - the map: concept -> lib module -> tools, the spec index, how to add a tool, the
  in-code header template.
* `docs/tools/inventory.md` - every tracked file under `tools/`, measured, with a verdict.
* `docs/tools/duplication.md` - the repeated concepts and code, quantified and ranked.
* `docs/tools/prose-audit.md` - the documentation embedded in code and where each block belongs.
* `docs/tools/design.md` - the target architecture (the `tools/lib/` package and the thin tools on it).
* `docs/tools/migration.md` - the ordered work packages, dependencies, acceptance per package.
* `docs/tools/retired.md` - what is retired, its replacement, the history worth keeping.
* `docs/tools/questions.md` - the decisions the owner is asked to make.
* `docs/tools/spec/<tool>.md` - one spec per kept or new tool.

A commit touching this material is `docs/tools: <message>` (`tools/git/commitlint.py` derives the `docs/`
members from the `docs/*.md` stems, which is why this page exists).
