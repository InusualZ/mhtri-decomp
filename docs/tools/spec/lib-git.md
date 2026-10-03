# `lib/git` - The git calls the tools make, typed, through `lib.proc`, paths forward-slashed

## Purpose

The git calls the tools make, typed, through `lib.proc`, paths forward-slashed.

## Users

The tools listed under this concept in `docs/tools/README.md`.

## Public API

* `Git(cwd)`: `rev_parse`, `head`, `current_branch`, `merge_base`, `is_ancestor`, `fork_point`, `show(ref, path) -> bytes`, `cat_index(path)`, `ls_files(eol=False)`, `status_porcelain`, `diff_names(a, b)`, `renames(a, b)`, `worktree_list`, `worktree_add`, `worktree_remove`, `branch_exists/create/delete`, `refs(prefix)`, `update_ref`, `stage(paths)`, `unstage`, `commit(message_file, pathspec)`, `merge_file_diff3(ours, base, theirs)`, `unmerged_stages(path)`
* no `git add -A` anywhere: `stage` takes an explicit pathspec

## Absorbs (today's implementations)

the 16 `git()`/`run()` wrappers; `stylelint._fork_point`, `rescue.merge_base`, `slots.merge_tree_of`, `mergebranch.blob`, `guard.index_blob`

## Test contract

Tier: fixture (a lib test never reads the live tree). on a `GitFixture`: each call's shape; `show` returns the blob bytes unchanged (CRLF kept); `stage` refuses an empty pathspec

## Known gaps

None until implemented; `migration.md` names the package.
