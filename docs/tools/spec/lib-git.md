# `lib/git` - The git calls the tools make, typed, through `lib.proc`, paths forward-slashed

## Purpose

One place that runs git: the codec rule, the inherited environment (so a pre-commit hook still sees its `GIT_INDEX_FILE`),
the forward-slash path spelling, and typed answers to the questions the tools ask.

## Users

The 16 wrappers delegate their process call here: `claims.git/_git_quiet`, `land.git`, `lane.git`, `mergebranch.git/blob`,
`recompile.git`, `rescue._run/merge_base`, `slots.git/merge_tree_of`, `stylelint.git/git_bytes/_fork_point`,
`unionguard._git`, `vtableaudit._git`, `wtsafe._git`, `checklf.git`, `guard._git`, `prepcommit.git`; `edit.py check`;
`lib.repo`.

## Public API

`Git(cwd=None)`:

* primitives: `run(*args, input, timeout, check) -> CompletedProcess` (text), `run_bytes(...)` (bytes), `out(*args, check=True)`
  (stdout; `""` on failure when not checking), `ok(*args) -> bool`. `check=True` raises `GitError(argv, returncode, stderr, cwd)`.
* `run_paths(args, paths, check=False, timeout=None)`: `git <args> -- <paths>` for a path list of any length. Within
  `proc.ARGV_BUDGET` (8 KB of arguments) the argv is exactly that; over it a `PATHSPEC_FILE_COMMANDS` command (`add`,
  `checkout`, `commit`, `reset`, `restore`, `rm`) reads the list from stdin (`--pathspec-from-file=- --pathspec-file-nul`,
  git >= 2.26), and any other (`diff`, `status`, `ls-files` refuse the option on git 2.43) runs in `proc.run_chunked`
  chunks, outputs concatenated, the first non-zero exit kept (a `diff --stat` prints one summary per chunk).
* refs and history: `rev_parse(rev) -> sha | None`, `head()`, `current_branch() -> name | None` (detached), `toplevel()`,
  `common_dir()` (absolute), `merge_base(a, b)`, `is_ancestor(a, b)`, `fork_point(branch, base="main")`, `merge_tree(a, b)`
  (in memory), `refs(prefix) -> {ref: sha}`, `update_ref(ref, value | delete=True)`, `branch_exists/create/delete`.
* blobs and the index: `show(ref, path) -> bytes | None` (CRLF kept), `show_many(ref, paths) -> {path: bytes | None}`
  (one `git cat-file --batch` for many blobs - a tree, not a file at a time), `cat_index(path) -> bytes | None`,
  `ls_files(*paths, eol=False)` (paths, or `(index class, worktree class, attr, path)` rows), `status_porcelain(untracked)`
  (`[(XY, path)]`, a rename's new path), `diff_names(a, b=None, *paths)`, `renames(a, b)`, `unmerged_stages(path=None)`
  (`{path: {stage: sha}}`), `merge_file_diff3(ours, base, theirs, labels=None) -> (bytes, conflicts)` (writes nothing; `labels` are the three `-L` names), `merge_bytes(ours, base, theirs, labels=None)` (the same over three byte strings, through a private temp dir - `mergebranch.merge_file` and `unionguard.three_way_overlap` both called their own copy).
* writes: `stage(paths)`, `unstage(paths)`, `commit(message_file, pathspec) -> sha` (all three through `run_paths`).
* worktrees: `worktree_list() -> [Worktree(path, head, branch, bare, detached, locked, prunable)]` (MAIN first),
  `worktree_add(path, branch, start=None, new=True)`, `worktree_remove(path, force=False)`.
* `slash(path)`.

## Invariants and rules

* **No `git add -A` anywhere**: `stage`, `unstage` and `commit` refuse an empty pathspec (`ValueError`).
* **No path list on the command line past the budget** (2026-10-05): Windows refuses a command line over 32,767
  characters (`[WinError 206]`), and `git commit -- <687 paths>` crashed a landing after its gate passed. `stage`,
  `unstage`, `commit`, `ls_files` and `diff_names` go through `run_paths`, and so does every caller that passes a
  variable-length list; the stdin pathspec has the command-line semantics (`commit` commits only the named paths and
  leaves another staged path staged; the pre-commit hook still runs), and a commit is never chunked.
* Every path a call returns is forward-slashed; every path argument to `show`/`cat_index` is forward-slashed first.
* The environment is inherited, never scrubbed: the hook's `GIT_INDEX_FILE`/`GIT_DIR` must reach the call.
* **`merge-file` exits with the conflict count** (truncated to 127), negative on error (255 once it passes a shell): `merge_file_diff3` raises `GitError` outside 0..127 and returns the count otherwise. `unionguard` read any exit but 0/1 as a failure, so two conflict hunks raised (WP3f).
* A wrapper keeps its own failure policy (its message, `SystemExit` vs `RuntimeError` vs `None`); only the process call is
  shared, so each tool's output and exit code are unchanged.

## Absorbs (today's implementations)

The process call of the 16 wrappers listed above (`duplication.md` (d)); `stylelint._fork_point`, `rescue.merge_base`,
`slots.merge_tree_of`, `mergebranch.blob` whole.

## Lib dependencies

`lib.proc`.

## Test contract

Tier: fixture (`tools/tests/lib/test_git.py`, on a `GitFixture`). Each call's shape; `show` returns the blob bytes unchanged
(CRLF kept) and takes a backslashed path; `stage`/`commit` refuse an empty pathspec and commit exactly the pathspec;
`worktree_list` puts MAIN first; `merge_tree` and `merge_file_diff3` report a conflict; `merge_bytes` counts two hunks as 2 (not an error), puts the labels in the markers and returns 0 for a clean merge; a missing input raises; `unmerged_stages` lists 1/2/3.
`run_paths`: a small list keeps the exact argv; over a patched budget `add`/`commit` read stdin (an unrelated staged file
is not committed and stays staged) and `ls-files`/`diff --name-only` run in several chunks with one run's answer; 3,000
paths stage and commit under `testing.argv_limit()` while the old `git add -- <paths>` is refused by the same limit.

## Known gaps

* `recompile.main_worktree_list` and `queue._file_lines`, `land._is_ancestor` still spell their git
  question through their wrapper rather than the typed method; their families' packages switch them.
