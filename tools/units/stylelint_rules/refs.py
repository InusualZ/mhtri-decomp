"""Both sides of a comparison read from git: the changed pairs, the base/ref copies linted, the ref's map.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import os

from tools.lib.git import Git
from tools.units.stylelint_rules.common import HEADERS, HEADER_SUFFIXES, SRC, SUFFIXES, Source
from tools.units.stylelint_rules.context import Ownership
from tools.units.stylelint_rules.r01_shared_type import rule1_findings
from tools.units.stylelint_rules.r14_pragma import codegen_pragma_findings
from tools.units.stylelint_rules.r11_untyped import rule11_findings
from tools.units.stylelint_rules.r12_unclaimed_data import rule12_findings
from tools.units.stylelint_rules.r13_method import rule13_findings
from tools.units.stylelint_rules.lint import lint_source
from tools.units.stylelint_rules.diff import renames_of, rule_counts, unresolved_declarations


def load_ownership_at_ref(root: str, ref: str) -> "Ownership | None":
    """The rule-2 index **as of `ref`**: each side of a `--diff` is judged by the map it was written against
    (judging the base by the working map made a pure rename read as +62 added rule-2 findings). The ref's files
    are read through this module's `git_bytes`, the one seam its selftest stubs."""
    def show(at: str, rel: str):
        try:
            return git_bytes(root, "show", "%s:%s" % (at, rel))
        except RuntimeError:
            return None
    return Ownership.at_ref(root, ref, show)


def unresolved_declarations_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                                   ownership: "Ownership") -> dict[str, set]:
    """`{path now: gap names}` for the **ref's** copies of the changed files (the `before` side)."""
    out: dict[str, set] = {}
    for before, after in pairs:
        if before is None:
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, before)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        out[after] = unresolved_declarations(Source(before, after, text), ownership)
    return out


_TREE_TEXTS: dict = {}
_TREE_SOURCES: dict = {}


def texts_at_ref(root: str, ref: str, top: str, suffixes: tuple) -> list[tuple[str, str]]:
    """`[(path, text)]` for every file under `top` ending in one of `suffixes` at `ref`, in `ls-tree` order.

    The blobs are read in one `git cat-file --batch` (`lib.git.Git.show_many`) and kept for the process, keyed by the
    ref's tree: a comparison walks the same tree once per rule, and a `git show` per file per walk was most of a
    `--diff`'s wall time. A blob that cannot be read is skipped, exactly as a failed `git show` skipped it; a ref git
    cannot resolve raises `RuntimeError`.
    """
    tree = git(root, "rev-parse", "--verify", "%s^{tree}" % ref).strip()
    key = (os.path.normcase(os.path.abspath(root)), tree, top, tuple(suffixes))
    if key not in _TREE_TEXTS:
        paths = [path for path in git(root, "ls-tree", "-r", "--name-only", tree, "--", top).splitlines()
                 if path.endswith(tuple(suffixes))]
        blobs = Git(root).show_many(tree, paths)
        _TREE_TEXTS[key] = [(path, blob.decode("utf-8", "replace")) for path, blob in blobs.items() if blob is not None]
    return _TREE_TEXTS[key]


def headers_at_ref(root: str, ref: str, rename: dict | None = None) -> list[Source]:
    """`include/` as it was at `ref`, one `Source` per header keyed by the path the working tree spells (`rename` is
    `{path_at_ref: path_now}`, `renames_of`); built once per (tree, rename) and shared by the four header walks."""
    rename = rename or {}
    texts = texts_at_ref(root, ref, HEADERS, HEADER_SUFFIXES)
    key = (id(texts), tuple(sorted(rename.items())))
    if key not in _TREE_SOURCES:
        _TREE_SOURCES[key] = (texts, [Source(path, rename.get(path, path), text) for path, text in texts])
    return _TREE_SOURCES[key][1]


def header_pragma_findings_at_ref(root: str, ref: str, rename: dict | None = None) -> list[dict]:
    """Rule-14 findings for `include/` as it was at `ref`, keyed `(rule, path_now)` - the back side.

    `rename` is `{path_at_ref: path_now}` (see `renames_of`): a renamed header keeps its finding under the
    path the *working tree* spells, so the whole-tree walk lines up with `header_pragma_findings`'s walk -
    without it, a rename alone reported the header's existing rule-14 (then rule-10) finding as an addition.
    """
    return [f for src in headers_at_ref(root, ref, rename) for f in codegen_pragma_findings(src)]


def header_pragma_counts_at_ref(root: str, ref: str, rename: dict | None = None) -> dict:
    """Rule-14 counts for `include/` as it was at `ref`, keyed `(rule, path_now)` - the `--diff` back side."""
    return rule_counts(header_pragma_findings_at_ref(root, ref, rename))


def header_rule13_findings_at_ref(root: str, ref: str, rename: dict | None = None) -> list[dict]:
    """Rule 13 for `include/` as it was at `ref`, keyed by the path the working tree spells (the back side).

    The type registry is the working tree's (`set_rule13_context`), on both sides, so the comparison is of
    text only - see the section comment.
    """
    return [f for src in headers_at_ref(root, ref, rename) for f in rule13_findings(src)]


def header_rule11_findings_at_ref(root: str, ref: str, rename: dict | None = None) -> list[dict]:
    """Rule-11 findings for `include/` as it was at `ref`, keyed `(rule, path_now)` - the back side.

    `rename` is `{path_at_ref: path_now}`: rule 11 has no per-file walk to fall back on (it is reported
    only by `header_rule11_findings`), so an untranslated base path was the *only* key a renamed header had
    - `include/fn_80429B94.h`'s six `void *` findings read as +6 on the rename to
    `include/Network/network_pat_control.h` alone.
    """
    return [f for src in headers_at_ref(root, ref, rename) for f in rule11_findings(src)]


def header_rule11_counts_at_ref(root: str, ref: str, rename: dict | None = None) -> dict:
    """Rule-11 counts for `include/` as it was at `ref`, keyed `(rule, path_now)` - the `--diff` back side."""
    return rule_counts(header_rule11_findings_at_ref(root, ref, rename))


def header_rule12_findings_at_ref(root: str, ref: str, ownership: "Ownership | None" = None,
                                  rename: dict | None = None) -> list[dict]:
    """Rule-12 findings for `include/` as it was at `ref`, keyed `(rule, path_now)` - the back side.

    Each side of a `--diff` is judged by the map it was written against (the rule-2 precedent): a rename
    that moves a data symbol out of a registered range must not read as a rule-12 addition.

    `rename` is `{path_at_ref: path_now}` (`renames_of`). Rule 12 is also judged per changed file by
    `findings_at_ref`, which already keys by the *new* path; this whole-tree walk is merged with it, so an
    untranslated base path both split one finding into two keys and read the rename as a +1.
    """
    if ownership is None:
        ownership = load_ownership_at_ref(root, ref)
    if ownership is None:
        return []
    return [f for src in headers_at_ref(root, ref, rename) for f in rule12_findings(src, ownership)]


def header_rule12_counts_at_ref(root: str, ref: str, ownership: "Ownership | None" = None,
                                rename: dict | None = None) -> dict:
    """Rule-12 counts for `include/` as it was at `ref`, keyed `(rule, path_now)` - the `--diff` back side."""
    return rule_counts(header_rule12_findings_at_ref(root, ref, ownership, rename))


def git(root: str, *args: str) -> str:
    p = Git(root).run(*args)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), (p.stderr or "").strip()))
    return p.stdout


def git_bytes(root: str, *args: str) -> bytes:
    p = Git(root).run_bytes(*args)
    if p.returncode != 0:
        raise RuntimeError("git %s failed: %s" % (" ".join(args), p.stderr.decode("utf-8", "replace").strip()))
    return p.stdout


def changed_src_files(root: str, ref: str) -> list[tuple[str | None, str]]:
    """`(path_at_ref, path_now)` for every `src/` file and shared header the tree changed against `ref`.

    A rename is one entry carrying both names, so the file's violations are compared against its old
    copy rather than counting as new. The whole `include/` tree is included: rule 2 applies to an ordinary
    header too (a foreign declaration in `include/<module>/*.h`), so a batch that edits one must be judged
    against it - and the unsplit band is included because rule 2 applies to it as the fallback file.
    """
    out: list[tuple[str | None, str]] = []
    for line in git(root, "diff", "--name-status", "-M", "--diff-filter=d", ref, "--",
                    SRC, HEADERS).splitlines():
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        status, paths = parts[0], parts[1:]
        after = paths[-1]
        if status.startswith("R"):
            before = paths[0]
        elif status.startswith("A"):
            before = None  # added by this batch: every violation in it is an addition
        else:
            before = after
        if after.endswith(SUFFIXES):
            out.append((before, after))
    # `git diff` cannot see an untracked file, so a batch that lints BEFORE staging its new unit would be
    # checked against the wrong set - and a new unit's violations are all additions, so it is the one file
    # guaranteed to matter. This blind spot refused two units on 2026-09-25 whose own lint run had reported
    # "no new section 6.5 violation" (it had compared two headers main changed and not the unit at all).
    for path in git(root, "ls-files", "--others", "--exclude-standard", "--", SRC, HEADERS).splitlines():
        if path and path.endswith(SUFFIXES):
            out.append((None, path))
    return out


def findings_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                    ownership: "Ownership | None" = None) -> list[dict]:
    """The ref's copies of the changed files, linted, judged by the **ref's** rule-2 index.

    A file absent at the ref (`before is None`) contributes nothing: it is new, so every violation in it is
    an addition.  A rename is keyed by its new path (`Source(before, after, text)`), which keeps the two
    sides comparable.  `ownership` is the map that side was written against (`load_ownership_at_ref`), and
    the findings keep the paths they have now, so a caller can line them up with the working side's.

    The findings themselves are returned rather than their counts because a `--diff` credit is a judgement
    about a *finding*: `rename_credits` reads the `symbol` rule 2 attaches and the address it resolves to.
    """
    findings = []
    for before, after in pairs:
        if before is None:
            continue
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, before)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        findings.extend(lint_source(Source(before, after, text), ownership))
    return findings


def deleted_src_files(root: str, base: str, ref: "str | None" = None) -> list[str]:
    """Base paths of the `src/`/`include/` files the batch **deletes** (a rename is not one: `-M` pairs it).

    `changed_src_files*` exclude deletions because a deleted file has no after side, but its base findings
    are exactly what a fold gives up: a unit whose bodies moved into a neighbour leaves every identity it
    carried behind, and `apply_move_credits` can only credit a removal it can see.  `ref` None compares the
    working tree with `base` (`--diff`); a ref compares two git trees (`--ref`).
    """
    args = ["diff", "--name-status", "-M", "--diff-filter=D", base] + ([ref] if ref else []) + ["--", SRC, HEADERS]
    out = []
    for line in git(root, *args).splitlines():
        parts = line.split("\t")
        if len(parts) >= 2 and parts[-1].endswith(SUFFIXES):
            out.append(parts[-1])
    return out


def findings_of_deleted(root: str, base: str, paths: list[str],
                        ownership: "Ownership | None" = None) -> list[dict]:
    """The base copies of the deleted files, linted: every identity they carried is a removal (`before` only).

    Read from the base side, keyed by the path they had - a deleted file has no other key.  Appended to the
    `before` findings only, so it can earn a move credit but can never itself be an addition.
    """
    findings = []
    for path in paths:
        try:
            text = git_bytes(root, "show", "%s:%s" % (base, path)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        findings.extend(lint_source(Source(path, path, text), ownership))
    return findings


def changed_src_files_between(root: str, base: str, ref: str) -> list[tuple[str | None, str]]:
    """`(path_at_base, path_at_ref)` for every `src/`/`include/` file `ref` changed against `base`.

    The read-only counterpart of `changed_src_files`: both sides come from git objects, so a **held branch**
    can be judged without checking it out and without touching the working tree.  There is no untracked-file
    pass here - a commit has no untracked files.
    """
    out: list[tuple[str | None, str]] = []
    for line in git(root, "diff", "--name-status", "-M", "--diff-filter=d", base, ref, "--",
                    SRC, HEADERS).splitlines():
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        status, paths = parts[0], parts[1:]
        after = paths[-1]
        if status.startswith("R"):
            before = paths[0]
        elif status.startswith("A"):
            before = None
        else:
            before = after
        if after.endswith(SUFFIXES):
            out.append((before, after))
    return out


def findings_of_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                    ownership: "Ownership | None" = None) -> list[dict]:
    """The ref's copies of the changed files, linted - the `after` side of a comparison against a ref tree.

    Unlike `findings_at_ref` (the `before` side, where a file the base did not carry contributes nothing),
    every `after` path exists at `ref`, including one the comparison added.
    """
    findings = []
    for _before, after in pairs:
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, after)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        findings.extend(lint_source(Source(after, after, text), ownership))
    return findings


def unresolved_declarations_of_ref(root: str, ref: str, pairs: list[tuple[str | None, str]],
                                   ownership: "Ownership") -> dict[str, set]:
    """`{path: gap names}` for the **ref's** copies of the changed files (the `after` side)."""
    out: dict[str, set] = {}
    for _before, after in pairs:
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, after)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        out[after] = unresolved_declarations(Source(after, after, text), ownership)
    return out


def sources_of_ref(root: str, ref: str, pairs: list[tuple[str | None, str]]) -> list["Source"]:
    """The ref's copies of the changed files as `Source`s (for the rename-credit gap comparison)."""
    out = []
    for _before, after in pairs:
        try:
            text = git_bytes(root, "show", "%s:%s" % (ref, after)).decode("utf-8", "replace")
        except RuntimeError:
            continue
        out.append(Source(after, after, text))
    return out


def src_paths_at_ref(root: str, ref: str) -> list[str]:
    """Every `src/` path (relative, slash-separated) that exists at `ref`."""
    return [line for line in git(root, "ls-tree", "-r", "--name-only", ref, "--", SRC).splitlines()
            if line.endswith(SUFFIXES)]


def rule1_findings_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]]) -> list[dict]:
    """Rule-1 findings for the ref's whole `src/` tree, keyed by the path each file has now.

    Rule 1 is cross-file, so unlike `findings_at_ref` (which lints only the changed files) it has to see
    every file: a batch can duplicate a type that already lives in an untouched file. A rename is keyed by
    its new path so the two sides stay comparable.  Returned as findings (not only counts) so
    `--list-added` can subtract the base side's occurrences by a line-independent identity.
    """
    rename = renames_of(pairs)
    return rule1_findings([Source(path, rename.get(path, path), text)
                           for path, text in texts_at_ref(root, ref, SRC, SUFFIXES)])


def rule1_counts_at_ref(root: str, ref: str, pairs: list[tuple[str | None, str]]) -> dict:
    """Rule-1 counts for the ref's whole `src/` tree, keyed by the path each file has now."""
    return rule_counts(rule1_findings_at_ref(root, ref, pairs))
