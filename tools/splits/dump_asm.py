#!/usr/bin/env python3
"""Regenerate the split disassembly dump that `tudiscover` and `callers` read, replacing the old one, and stamp it.
Spec: docs/tools/spec/dump_asm.md. CLI: dump_asm.py [--root DIR] [--check | --dry-run]."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import re
import shutil
import subprocess
import time
from dataclasses import dataclass

from tools.lib import refs, repo

GAME = repo.VERSION
#: The split's `out_dir` while it runs: `build/<game>/<STAGE>`; its `asm/` replaces the dump only after a clean exit.
STAGE = ".dump_asm-stage"
#: The old dump while it is being replaced: `build/<game>/<RETIRED>.<pid>`, deleted once the new one is in place.
RETIRED = ".dump_asm-retired"


@dataclass(frozen=True)
class Tree:
    """One tree's dump: its config (read), its toolchain, and the one directory this tool may replace."""
    root: str

    @property
    def config(self) -> str:
        return os.path.join(self.root, "config", GAME, "config.yml")

    @property
    def build_dir(self) -> str:
        return os.path.join(self.root, "build", GAME)

    @property
    def asm_dir(self) -> str:
        return os.path.join(self.build_dir, "asm")

    @property
    def stage(self) -> str:
        return os.path.join(self.build_dir, STAGE)

    @property
    def tmp_config(self) -> str:
        """A copy of `config.yml` with `write_asm: true`: never the repo's own file."""
        return os.path.join(self.build_dir, "dump_asm.yml")

    @property
    def dtk(self) -> str:
        return os.path.join(self.root, "build", "tools", "dtk.exe" if os.name == "nt" else "dtk")

    def stamp(self, asm_dir: str | None = None) -> refs.DumpStamp:
        """The stamp of this tree's OWN dump (never MAIN's fallback) over this tree's map, splits and DOL."""
        d = asm_dir or self.asm_dir
        return refs.DumpStamp(d, os.path.join(self.root, "config", GAME, "symbols.txt"),
                              os.path.join(self.root, "config", GAME, "splits.txt"),
                              os.path.join(self.root, "orig", GAME, "sys", "main.dol"), GAME, d, self.root)

    def rel(self, path: str) -> str:
        try:
            return os.path.relpath(path, self.root)
        except ValueError:
            return path


def tree_for(root: str | None) -> Tree:
    """The tree the dump is made for: `--root`, else the invocation's tree (never this file's). `SystemExit` unless
    it is a tree with a `config/<game>/config.yml` - its config and its output always come from the same place."""
    if root is None:
        found = repo.repo_root()
    else:
        found = os.path.abspath(root)
        if not os.path.isfile(os.path.join(found, repo.MARKER)):
            raise SystemExit("--root %s is not a tree (no %s)" % (root, repo.MARKER))
    t = Tree(found)
    if not os.path.isfile(t.config):
        raise SystemExit("%s has no %s" % (found, t.rel(t.config)))
    return t


def _is_link(path: str) -> bool:
    return os.path.islink(path) or bool(getattr(os.path, "isjunction", lambda _p: False)(path))


def _same(a: str, b: str) -> bool:
    return os.path.normcase(os.path.realpath(a)) == os.path.normcase(os.path.realpath(b))


def removal_refusal(tree: Tree, path: str) -> str | None:
    """Why `path` must not be deleted by this tool, or None. The only deletable paths are the tree's own
    `build/<game>/asm`, the stage, the stage's `asm/` and a retired dump - each a real directory (not a link or
    junction) under the tree's real `build/<game>/`."""
    norm = os.path.normpath(os.path.abspath(path))
    base, parent = os.path.basename(norm), os.path.dirname(norm)
    staged = base == "asm" and os.path.basename(parent) == STAGE
    if base not in ("asm", STAGE) and not base.startswith(RETIRED + "."):
        return "%s is not a directory dump_asm owns (asm, %s, %s/asm, %s.*)" % (path, STAGE, STAGE, RETIRED)
    if not _same(os.path.dirname(parent) if staged else parent, tree.build_dir):
        return "%s is not under this tree's build/%s (%s)" % (path, GAME, tree.build_dir)
    chain = (tree.root, os.path.join(tree.root, "build"), tree.build_dir) + ((parent,) if staged else ()) + (path,)
    for p in chain:
        if os.path.lexists(p) and _is_link(p):
            return "%s is a link or junction - dump_asm deletes only real directories it owns" % p
    real_root = os.path.normcase(os.path.realpath(tree.root))
    if not os.path.normcase(os.path.realpath(path)).startswith(real_root + os.sep):
        return "%s resolves outside the tree %s" % (path, tree.root)
    return None


def remove_owned(tree: Tree, path: str) -> None:
    """Delete one directory this tool owns; `SystemExit` (deleting nothing) for any other path."""
    why = removal_refusal(tree, path)
    if why:
        raise SystemExit("REFUSED: " + why)
    if os.path.isdir(path):
        shutil.rmtree(path)


def leftovers(tree: Tree) -> list[str]:
    """A retired dump or a staged `asm/` an interrupted run left in `build/<game>/` (the stage itself is kept between
    runs: its objects are dtk's to skip when unchanged)."""
    try:
        names = os.listdir(tree.build_dir)
    except OSError:
        return []
    out = [os.path.join(tree.build_dir, n) for n in names if n.startswith(RETIRED + ".")]
    if os.path.isdir(os.path.join(tree.stage, "asm")):
        out.append(os.path.join(tree.stage, "asm"))
    return sorted(out)


def temp_config(tree: Tree) -> str:
    """`config.yml` as text with `write_asm: true` - the repo carries no YAML dependency.

    Read and written with `newline=""` so the copy keeps the repo file's line endings: the value is the only change.
    """
    with open(tree.config, "r", encoding="utf-8", newline="") as fh:
        text = fh.read()
    new, n = re.subn(r"(?m)^(write_asm:[ \t]*)\S+", r"\g<1>true", text)
    assert n == 1, "config.yml: expected exactly one `write_asm:` line, found %d" % n
    return new


def split_command(tree: Tree) -> list[str]:
    return [tree.dtk, "dol", "split", "--no-update", tree.tmp_config, tree.stage]


def replace_dump(tree: Tree, staged: str) -> None:
    """Swap the staged `asm/` in for the dump: retire the old directory (its stamp goes with it), move the new one in,
    then delete the retired one. A failed second move puts the old dump back. `OSError` when the swap cannot run."""
    retired = os.path.join(tree.build_dir, "%s.%d" % (RETIRED, os.getpid()))
    had_old = os.path.isdir(tree.asm_dir)
    if had_old:
        os.replace(tree.asm_dir, retired)
    try:
        os.replace(staged, tree.asm_dir)
    except OSError:
        if had_old:
            os.replace(retired, tree.asm_dir)
        raise
    if had_old:
        remove_owned(tree, retired)


def dump(tree: Tree, runner=subprocess.run, out=None, err=None) -> int:
    """One dump: split into the stage, swap its `asm/` in for the old dump, stamp it last. A failed split (or one
    that wrote no `.s`) leaves the old dump and its stamp untouched; the stamp records the inputs as read BEFORE the
    split, so an edit made while dtk runs reads as stale."""
    out = out if out is not None else sys.stdout
    err = err if err is not None else sys.stderr
    for p in leftovers(tree):
        print("removing           %s (left by an interrupted run)" % tree.rel(p), file=out)
        remove_owned(tree, p)
    os.makedirs(tree.build_dir, exist_ok=True)       # a fresh worktree has no build/<game> yet
    try:
        pre = tree.stamp().current()
    except OSError as exc:
        print("cannot read the split's inputs in this tree (%s) - copy the DOL, never junction it" % exc, file=err)
        return 2
    before = refs.dump_files(tree.asm_dir)
    with open(tree.tmp_config, "w", encoding="utf-8", newline="") as fh:
        fh.write(temp_config(tree))
    print("split              into %s; %d .s file(s) in %s now"
          % (tree.rel(tree.stage), len(before), tree.rel(tree.asm_dir)), file=out)
    t0 = time.time()
    try:
        proc = runner(split_command(tree), cwd=tree.root)
    finally:
        try:
            os.unlink(tree.tmp_config)
        except OSError:
            pass
    staged = os.path.join(tree.stage, "asm")
    written = refs.dump_files(staged)
    if proc.returncode != 0 or not written:
        why = ("`dtk dol split` failed (exit %d)" % proc.returncode if proc.returncode != 0
               else "`dtk dol split` wrote no .s file under %s" % tree.rel(staged))
        remove_owned(tree, staged)
        print("%s; the dump and its stamp are unchanged" % why, file=err)
        return proc.returncode or 1
    try:
        replace_dump(tree, staged)
    except OSError as exc:
        remove_owned(tree, staged)
        print("could not replace %s (%s); the dump and its stamp are unchanged"
              % (tree.rel(tree.asm_dir), exc), file=err)
        return 2
    tree.stamp().write({k: pre[k] for k in ("symbols", "splits", "dol")})
    after = refs.dump_files(tree.asm_dir)
    gone = len({tree.rel(p) for p in before} - {tree.rel(p) for p in after}) if before else 0
    print("dumped             %d .s file(s) in %.1f s (was %d; %d stale file(s) removed)"
          % (len(after), time.time() - t0, len(before), gone), file=out)
    return 0


def main(argv=None, runner=subprocess.run):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", help="the tree to dump (its config, its toolchain, its build/; default: the "
                    "invocation's tree)")
    ap.add_argument("--check", action="store_true",
                    help="report the dump's age and stamp; run nothing (exit 1 unless fresh)")
    ap.add_argument("--dry-run", action="store_true",
                    help="print the split command and the temp config it would write; run nothing")
    args = ap.parse_args(argv)
    tree = tree_for(args.root)

    try:
        state, msg = tree.stamp().status()
    except OSError as exc:
        state, msg = "unknown", "the stamp could not be checked (%s)" % exc
    print("asm dump           %s" % msg)
    if args.check:
        return 0 if state == "fresh" else 1
    if not os.path.isfile(tree.dtk):
        print("%s is missing - run `ninja tools` to download the pinned toolchain" % tree.rel(tree.dtk),
              file=sys.stderr)
        return 2
    if args.dry_run:
        print("would write        %s (config.yml with write_asm: true)" % tree.rel(tree.tmp_config))
        print("would run          %s" % " ".join(split_command(tree)))
        print("then replace       %s with %s/asm, and stamp it" % (tree.rel(tree.asm_dir), tree.rel(tree.stage)))
        return 0
    rc = dump(tree, runner)
    if rc == 0:
        print("asm dump           %s" % tree.stamp().status()[1])
    return rc


if __name__ == "__main__":
    sys.exit(main())
