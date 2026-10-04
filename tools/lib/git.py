"""The git calls the tools make, typed, through lib.proc, paths forward-slashed on the way out.
Spec: docs/tools/spec/lib-git.md. CLI: none (library)."""
from __future__ import annotations

import os
import subprocess
from dataclasses import dataclass
from typing import Iterable

from tools.lib import proc


class GitError(RuntimeError):
    """A git call that had to succeed failed; carries the argv, the exit code and git's stderr."""

    def __init__(self, args: tuple[str, ...], returncode: int, stderr: str, cwd: str | None = None) -> None:
        self.argv, self.returncode, self.stderr, self.cwd = args, returncode, stderr, cwd
        super().__init__("git %s failed (%d)%s: %s" % (" ".join(args), returncode,
                                                       " in %s" % cwd if cwd else "", stderr.strip()))


@dataclass(frozen=True)
class Worktree:
    """One `git worktree list --porcelain` record."""
    path: str
    head: str | None = None
    branch: str | None = None          # short name (`refs/heads/` stripped); None when detached or bare
    bare: bool = False
    detached: bool = False
    locked: bool = False
    prunable: bool = False


def slash(path: str | os.PathLike) -> str:
    """A path with forward slashes - the spelling git prints and `REF:path` needs."""
    return os.fspath(path).replace("\\", "/")


class Git:
    """The git calls, run in `cwd` (None = the process cwd) with the inherited environment."""

    def __init__(self, cwd: str | os.PathLike | None = None) -> None:
        self.cwd = os.fspath(cwd) if cwd is not None else None

    # --- the two primitives ---------------------------------------------------------------------------------
    def run(self, *args: str, input: str | None = None, timeout: float | None = None,
            check: bool = False) -> subprocess.CompletedProcess:
        """`git <args>` in text mode (UTF-8, replacement); `check` raises `GitError` on a non-zero exit."""
        p = proc.run(["git", *args], cwd=self.cwd, input=input, timeout=timeout)
        if check and p.returncode != 0:
            raise GitError(tuple(args), p.returncode, p.stderr or "", self.cwd)
        return p

    def run_bytes(self, *args: str, input: bytes | None = None, timeout: float | None = None,
                  check: bool = False) -> subprocess.CompletedProcess:
        """`git <args>` with bytes stdout/stderr; `check` raises `GitError` on a non-zero exit."""
        p = proc.run_bytes(["git", *args], cwd=self.cwd, input=input, timeout=timeout)
        if check and p.returncode != 0:
            raise GitError(tuple(args), p.returncode, (p.stderr or b"").decode("utf-8", "replace"), self.cwd)
        return p

    def out(self, *args: str, check: bool = True) -> str:
        """`git <args>`'s stdout; raises `GitError` on failure when `check`, else returns "" on failure."""
        p = self.run(*args, check=check)
        return p.stdout if p.returncode == 0 else ""

    def ok(self, *args: str) -> bool:
        """Whether `git <args>` exits 0."""
        return self.run(*args).returncode == 0

    def _line(self, *args: str) -> str | None:
        p = self.run(*args)
        value = (p.stdout or "").strip()
        return value if p.returncode == 0 and value else None

    # --- refs and history -----------------------------------------------------------------------------------
    def rev_parse(self, rev: str) -> str | None:
        """The object id `rev` names, or None when it names nothing."""
        return self._line("rev-parse", "--verify", "--quiet", rev + "^{commit}") or self._line(
            "rev-parse", "--verify", "--quiet", rev)

    def head(self) -> str | None:
        return self.rev_parse("HEAD")

    def current_branch(self) -> str | None:
        """The checked-out branch's short name; None when HEAD is detached."""
        return self._line("symbolic-ref", "--short", "-q", "HEAD")

    def toplevel(self) -> str | None:
        top = self._line("rev-parse", "--show-toplevel")
        return slash(top) if top else None

    def common_dir(self) -> str | None:
        """The absolute git common dir (MAIN's `.git` for a linked worktree)."""
        d = self._line("rev-parse", "--path-format=absolute", "--git-common-dir")
        return slash(d) if d else None

    def merge_base(self, a: str, b: str) -> str | None:
        return self._line("merge-base", a, b)

    def is_ancestor(self, a: str, b: str) -> bool:
        return self.ok("merge-base", "--is-ancestor", a, b)

    def fork_point(self, branch: str, base: str = "main") -> str | None:
        """Where `branch` left `base`: their merge base (the base `--diff main` resolves to)."""
        return self.merge_base(base, branch)

    def merge_tree(self, a: str, b: str) -> subprocess.CompletedProcess:
        """`git merge-tree --write-tree a b` (in memory; touches no worktree or index)."""
        return self.run("merge-tree", "--write-tree", a, b)

    def refs(self, prefix: str = "refs/") -> dict[str, str]:
        """`{refname: object id}` for every ref under `prefix`."""
        out = self.out("for-each-ref", "--format=%(objectname) %(refname)", prefix, check=False)
        rows = (line.split(" ", 1) for line in out.splitlines() if " " in line)
        return {name: oid for oid, name in rows}

    def update_ref(self, ref: str, value: str | None = None, delete: bool = False) -> None:
        if delete:
            self.run("update-ref", "-d", ref, check=True)
        else:
            self.run("update-ref", ref, value or "HEAD", check=True)

    def branch_exists(self, name: str) -> bool:
        return self.ok("show-ref", "--verify", "--quiet", "refs/heads/" + name)

    def branch_create(self, name: str, start: str = "HEAD") -> None:
        self.run("branch", name, start, check=True)

    def branch_delete(self, name: str, force: bool = False) -> None:
        self.run("branch", "-D" if force else "-d", name, check=True)

    # --- blobs and the index --------------------------------------------------------------------------------
    def show(self, ref: str, path: str | os.PathLike) -> bytes | None:
        """`path`'s exact bytes at `ref` (CRLF kept), or None when `ref` does not carry it."""
        p = self.run_bytes("show", "%s:%s" % (ref, slash(path)))
        return p.stdout if p.returncode == 0 else None

    def cat_index(self, path: str | os.PathLike) -> bytes | None:
        """The staged blob's bytes (`:path`), or None when the path is not in the index."""
        p = self.run_bytes("cat-file", "-p", ":" + slash(path))
        return p.stdout if p.returncode == 0 else None

    def ls_files(self, *paths: str, eol: bool = False) -> list:
        """Tracked paths; with `eol`, `[(index class, worktree class, attr, path)]` from `--eol`."""
        if not eol:
            return [slash(x) for x in self.out("ls-files", "--", *paths).splitlines() if x]
        rows = []
        for line in self.out("ls-files", "--eol", "--", *paths).splitlines():
            head, sep, path = line.partition("\t")
            parts = head.split()
            if not sep or len(parts) < 2:
                continue
            idx, wt = parts[0][2:], parts[1][2:]
            attr = head.split("attr/", 1)[1].strip() if "attr/" in head else ""
            rows.append((idx, wt, attr, slash(path)))
        return rows

    def status_porcelain(self, untracked: str = "all") -> list[tuple[str, str]]:
        """`[(two-letter code, path)]` from `git status --porcelain`; a rename reports its new path."""
        rows = []
        for line in self.out("status", "--porcelain", "-u" + untracked).splitlines():
            if len(line) < 4:
                continue
            path = line[3:]
            if " -> " in path:
                path = path.split(" -> ", 1)[1]
            rows.append((line[:2], slash(path.strip('"'))))
        return rows

    def diff_names(self, a: str, b: str | None = None, *paths: str) -> list[str]:
        """Paths that differ between `a` and `b` (or the working tree when `b` is None)."""
        args = ["diff", "--name-only", a] + ([b] if b else []) + ["--", *paths]
        return [slash(x) for x in self.out(*args).splitlines() if x]

    def renames(self, a: str, b: str) -> list[tuple[str, str]]:
        """`[(old, new)]` for every rename git detects between `a` and `b`."""
        out = self.out("diff", "--name-status", "-M", a, b)
        rows = []
        for line in out.splitlines():
            parts = line.split("\t")
            if len(parts) == 3 and parts[0].startswith("R"):
                rows.append((slash(parts[1]), slash(parts[2])))
        return rows

    def unmerged_stages(self, path: str | None = None) -> dict[str, dict[int, str]]:
        """`{path: {stage: blob id}}` from `git ls-files -u` (1 = base, 2 = ours, 3 = theirs)."""
        raw = self.run_bytes("ls-files", "-u", "-z", *(["--", slash(path)] if path else []), check=True)
        out: dict[str, dict[int, str]] = {}
        for rec in raw.stdout.decode("utf-8", "replace").split("\0"):
            if not rec or "\t" not in rec:
                continue
            meta, p = rec.split("\t", 1)
            _mode, sha, stage = meta.split()
            out.setdefault(slash(p), {})[int(stage)] = sha
        return out

    def merge_file_diff3(self, ours: str | os.PathLike, base: str | os.PathLike,
                         theirs: str | os.PathLike, labels: tuple[str, str, str] | None = None) -> tuple[bytes, int]:
        """`git merge-file -p --diff3 ours base theirs`: (merged bytes, conflict count); writes nothing.

        git's exit is the conflict count truncated to 127, or negative on error (255 once it reaches a shell):
        anything outside 0..127 raises `GitError`. `labels` names the three sides in the markers (`-L`).
        """
        names = [a for label in (labels or ()) for a in ("-L", label)]
        p = self.run_bytes("merge-file", "-p", "--diff3", *names, os.fspath(ours), os.fspath(base),
                           os.fspath(theirs))
        if not 0 <= p.returncode <= 127:
            raise GitError(("merge-file",), p.returncode, (p.stderr or b"").decode("utf-8", "replace"), self.cwd)
        return p.stdout, p.returncode

    def merge_bytes(self, ours: bytes, base: bytes, theirs: bytes,
                    labels: tuple[str, str, str] | None = None) -> tuple[bytes, int]:
        """`merge_file_diff3` over three byte strings (written to a private temp dir, removed after)."""
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            paths = []
            for name, data in (("ours", ours), ("base", base), ("theirs", theirs)):
                path = os.path.join(tmp, name)
                with open(path, "wb") as fh:
                    fh.write(data)
                paths.append(path)
            return self.merge_file_diff3(*paths, labels=labels)

    def stage(self, paths: Iterable[str]) -> None:
        """`git add -- <paths>`; an empty pathspec is refused (never `git add -A`)."""
        paths = [slash(p) for p in paths]
        if not paths:
            raise ValueError("stage: an empty pathspec stages everything - name the paths")
        self.run("add", "--", *paths, check=True)

    def unstage(self, paths: Iterable[str]) -> None:
        paths = [slash(p) for p in paths]
        if not paths:
            raise ValueError("unstage: an empty pathspec - name the paths")
        self.run("reset", "-q", "--", *paths, check=True)

    def commit(self, message_file: str | os.PathLike, pathspec: Iterable[str]) -> str:
        """Commit exactly `pathspec` with the message in `message_file`; return the new commit id."""
        paths = [slash(p) for p in pathspec]
        if not paths:
            raise ValueError("commit: an empty pathspec - name the paths")
        self.run("commit", "-q", "-F", os.fspath(message_file), "--", *paths, check=True)
        return self.head() or ""

    # --- worktrees ------------------------------------------------------------------------------------------
    def worktree_list(self) -> list[Worktree]:
        """Every worktree of the repository, MAIN first (registration order)."""
        out, cur = [], None
        for line in self.out("worktree", "list", "--porcelain").splitlines() + [""]:
            if not line:
                if cur is not None:
                    out.append(Worktree(**cur))
                cur = None
                continue
            key, _sep, value = line.partition(" ")
            if key == "worktree":
                cur = {"path": slash(value)}
            elif cur is None:
                continue
            elif key == "HEAD":
                cur["head"] = value
            elif key == "branch":
                cur["branch"] = value[len("refs/heads/"):] if value.startswith("refs/heads/") else value
            elif key in ("bare", "detached", "locked", "prunable"):
                cur[key] = True
        return out

    def worktree_add(self, path: str | os.PathLike, branch: str, start: str | None = None,
                     new: bool = True) -> None:
        if new:
            self.run("worktree", "add", "-b", branch, os.fspath(path), *([start] if start else []), check=True)
        else:
            self.run("worktree", "add", os.fspath(path), branch, check=True)

    def worktree_remove(self, path: str | os.PathLike, force: bool = False) -> None:
        self.run("worktree", "remove", *(["--force"] if force else []), os.fspath(path), check=True)
