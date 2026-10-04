# `commitlint` - Lint a commit subject against the CLAUDE.md convention; member sets derived from the tree; the gate lints its own subject with it

<!-- generated from the module docstring of `tools/git/commitlint.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Lint commit messages against the convention CLAUDE.md defines ("Commit messages follow one convention").

## Users

the landing gate (10); profiles (`.claude/agents`) (1)

## CLI

```
python tools/git/commitlint.py <message-file>         # the `commit-msg` hook shape git hands over
python tools/git/commitlint.py --message "<subject>"  # a one-liner
python tools/git/commitlint.py --last 20              # score the recent history
python tools/git/commitlint.py --install-hook [--force]
python tools/git/commitlint.py --selftest
```
Flags: `--force`, `--install-hook`, `--last`, `--message`, `--root`, `--selftest`.
Exit codes: Exit status follows the house convention (`--diff`'s 0/1/2): **0** clean (a warning alone does not fail - an unknown family is legitimate), **1** one or more violations, **2** nothing was checked (no mode given, `--last 0`, or a history with no commits), so the tool can be wired into a gate later.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: message -> findings, exit 0/1/2.

## Invariants and rules

* The convention is `<category>: <message>`, then an optional long description. CLAUDE.md is the specification; this tool checks only the part of it that is **mechanically** checkable:
| # | check | how |
| - | ----- | --- |
| 1 | subject shape | the first line is `<category>: <message>` - a category token, a colon, a space, then the message |
| 2 | category membership | the category names a place in the tree (`game/<module>`, `tools/<area>`, `agents/<profile|policy>`, `config/<what>`, `docs/<topic>`, `repo/<area>`) - see `derive_members` |
| 3 | unknown member / family | a member not in a known family is an **error** (a typo or an invented module is exactly what that catches); an unknown **family** is a **warning** (the convention calls the list open, so a new family is added to CLAUDE.md deliberately) |
| 4 | message length | at most 120 characters, counted *after* `<category>: `, so the category and its separator are not charged against it |
| 5 | subject line, not a wall | the first line is not empty or all whitespace, and a body is separated from it by a blank line |
* **What it deliberately does not check.** The long description's *content* is a review matter, not a mechanical one. The convention bans *why* (reasoning, alternatives considered, an account of the work) from the description, but "no why" cannot be decided by a regular expression, and a heuristic that guessed wrong would flag messages a reviewer would accept - which is how a lint becomes hated and gets switched off. So the description's content is left to review; only its *shape* is checked here (check 5).
* **The membership sets are derived from the tree, never hard-coded** (`derive_members`), so the lint cannot drift from the tree it describes:
* `game/<module>` - the directories under `src/`. A member is matched **case-insensitively**: the convention's own examples lower-case the prose (`game/network`), while the tree directory is `src/Network`, and a lint that rejected the spec's example would be a false positive.
* `tools/<area>` - a directory under `tools/` (the grouping: `tools/units`, `tools/git`, `tools/flags`), or the **stem of any script at any depth** (`tools/units/land.py` -> `tools/land`, `tools/units/stylelint.py` -> `tools/stylelint`).
* `agents/<name>` - the profiles in `.claude/agents/*.md`, plus `policy` for CLAUDE.md.
* `config/{flags,symbols,splits}` - the three inputs the convention names (configure.py -> `flags`, symbols.txt -> `symbols`, splits.txt -> `splits`).
* `docs/<topic>` - the documents under `docs/`.
* `repo/<area>` - read from the files at the root: `readme`, `license`, `ci` (a `.github*` directory), `gitignore`.
* Exit status follows the house convention (`--diff`'s 0/1/2): **0** clean (a warning alone does not fail - an unknown family is legitimate), **1** one or more violations, **2** nothing was checked (no mode given, `--last 0`, or a history with no commits), so the tool can be wired into a gate later.
* **`--install-hook` is a convenience, not the enforcement path.** It writes an `sh`-compatible `commit-msg` hook to `.git/hooks/commit-msg` (mode 0755) that calls this tool with `"$1"`. That hook is untracked and therefore **per-clone**: it never travels with the repository, and it is not what stops a bad message - the enforcement is a gate that runs this tool directly (the same `0/1/2` it already speaks). Note also that when `core.hooksPath` is configured - `git config core.hooksPath tools/git/hooks` in this repo - git does **not** consult `.git/hooks`, so the installer says so rather than pretend the hook is active.

## Lib dependencies

git, repo.

## Test contract

Tier: fixture (fixture tree + fixture history).
Today's selftest (`tools/git/commitlint_selftest.py`): Every check runs against a **fixture tree** and a **fixture git history** built in a temp directory, never against this repository's working tree or HEAD, so the result cannot move when the tree does. The two mechanisms that decide the lint - the derived member sets and the message checks - are exercised both as pure functions and through the CLI, so the exit codes (0/1/2) are pinned too.
Now: `tools/tests/git/test_commitlint.py` (83 checks, re-homed; `--selftest` forwards; `commitlint_selftest.py` is deleted). The tool's git calls go through `lib.git` (`toplevel`, `log`, `rev-parse`, `config`).

## Known gaps

`docs/<topic>` members are top-level `docs/*.md` stems only (questions.md 11)
