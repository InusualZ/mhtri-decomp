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
* refs and history: `rev_parse(rev) -> sha | None`, `head()`, `current_branch() -> name | None` (detached), `toplevel()`,
  `common_dir()` (absolute), `merge_base(a, b)`, `is_ancestor(a, b)`, `fork_point(branch, base="main")`, `merge_tree(a, b)`
  (in memory), `refs(prefix) -> {ref: sha}`, `update_ref(ref, value | delete=True)`, `branch_exists/create/delete`.
* blobs and the index: `show(ref, path) -> bytes | None` (CRLF kept), `cat_index(path) -> bytes | None`,
  `ls_files(*paths, eol=False)` (paths, or `(index class, worktree class, attr, path)` rows), `status_porcelain(untracked)`
  (`[(XY, path)]`, a rename's new path), `diff_names(a, b=None, *paths)`, `renames(a, b)`, `unmerged_stages(path=None)`
  (`{path: {stage: sha}}`), `merge_file_diff3(ours, base, theirs) -> (bytes, conflicts)` (writes nothing).
* writes: `stage(paths)`, `unstage(paths)`, `commit(message_file, pathspec) -> sha`.
* worktrees: `worktree_list() -> [Worktree(path, head, branch, bare, detached, locked, prunable)]` (MAIN first),
  `worktree_add(path, branch, start=None, new=True)`, `worktree_remove(path, force=False)`.
* `slash(path)`.

## Invariants and rules

* **No `git add -A` anywhere**: `stage`, `unstage` and `commit` refuse an empty pathspec (`ValueError`).
* Every path a call returns is forward-slashed; every path argument to `show`/`cat_index` is forward-slashed first.
* The environment is inherited, never scrubbed: the hook's `GIT_INDEX_FILE`/`GIT_DIR` must reach the call.
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
`worktree_list` puts MAIN first; `merge_tree` and `merge_file_diff3` report a conflict; `unmerged_stages` lists 1/2/3.

## Known gaps

* `guard.index_blob` reads `cat-file -p :path` through `guard._git`, i.e. already through here; it becomes
  `Git.cat_index` when `guard` is thinned (WP3f).
* `recompile.main_worktree_list` and `queue._file_lines`, `land._is_ancestor`, `checklf.blob_of` still spell their git
  question through their wrapper rather than the typed method; their families' packages switch them.
