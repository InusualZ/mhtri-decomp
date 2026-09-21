#!/usr/bin/env python3
"""A review gate in front of every commit: show what is about to be committed, and refuse until the
human has approved *that exact staged content*.

Why: `AGENTS.md` rule 6 says nothing is committed without explicit approval, and the review matters
most exactly where it is most tempting to skip it - a huge `symbols.txt` churn, a `configure.py` flag
change, a `build/` file caught by `git add -f`. Habit is not enforcement, so this is a `pre-commit`
hook plus a state file.

How it works:

* `check` is what the hook runs. It prints the review sheet and exits non-zero (blocking the commit)
  unless an approval exists that matches the **currently staged tree** - so approving a diff and then
  changing it invalidates the approval. Some things block regardless of approval (see below).
* `approve -m "<who said what>"` records the human's approval of the staged tree, in
  `.pi/handoff/commit-approval.json` (gitignored scratch).
* `review` prints the same sheet without blocking, for looking at a change before staging everything.

Hard blocks, whatever the approval says (`AGENTS.md` non-negotiables):

* a staged path under `build/`, `orig/`, `.pi/`, `.lavish/`, `.vscode/`, or a `*.dol`/`*.rel`/`*.elf`/
  `*.o`/`*.map`/`*.pyc` file (rule 2: build output and original files never get committed);
* a staged `AGENTS.md` that still contains a `LOCAL-ONLY` marker line (rule 8);
* `localonly.py verify` failing for a staged AGENTS.md.

Warnings (loud, but the human can still approve): a staged `symbols.txt`/`splits.txt` with more
changed lines than `--max-generated-lines`, a staged `configure.py`/`config/**` (rule 3: flag or tool
version changes need evidence; also the other window's area), and staged source under `src/`.

Usage:

    python tools/agents/commitgate.py review [--diff-lines N]
    python tools/agents/commitgate.py check                    # what the hook runs
    python tools/agents/commitgate.py approve -m "user: ok"    # after the human said yes
    python tools/agents/commitgate.py status
    python tools/agents/commitgate.py install | uninstall      # the .git/hooks/pre-commit shim

Escape hatch for a human in a hurry: `COMMITGATE=off git commit ...` (it prints a warning and lets it
through).
"""
import argparse
import hashlib
import json
import os
import stat
import subprocess
import sys
import time

SCHEMA = 1
STATE_REL = os.path.join(".pi", "handoff", "commit-approval.json")
BLOCKED_PREFIX = ("build/", "orig/", ".pi/", ".lavish/", ".vscode/", "__pycache__/")
BLOCKED_SUFFIX = (".dol", ".rel", ".elf", ".o", ".map", ".pyc", ".d", ".a")
APPROVAL_MINUTES = 120


def repo_root(start=None):
    d = os.path.abspath(start or os.path.dirname(os.path.abspath(__file__)))
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise SystemExit("repo root (configure.py) not found")
        d = parent


ROOT = repo_root()
STATE = os.path.join(ROOT, STATE_REL)
HOOK = os.path.join(ROOT, ".git", "hooks", "pre-commit")
HOOK_BODY = '#!/bin/sh\n# installed by tools/agents/commitgate.py - see the commit-review-gate skill\nexec python "$(git rev-parse --show-toplevel)/tools/agents/commitgate.py" check\n'


def git(*args, check=True):
    r = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if check and r.returncode:
        raise SystemExit("git %s failed: %s" % (" ".join(args), (r.stderr or "").strip()))
    return r.stdout


def staged_diff():
    """The bytes an approval binds to: `--binary` so a binary change cannot be approved without
    noticing, and no colour so the hash is stable."""
    return git("diff", "--cached", "--binary", "--no-color", "-M")


def staged_diff_text():
    """The diff shown to the human: without `--binary`, so a binary blob is not printed as base85."""
    return git("diff", "--cached", "--no-color", "-M")


def utf8_streams():
    """A diff of arbitrary files will contain non-ASCII; a gate must never die printing it (on
    Windows stdout defaults to cp1252 and would raise)."""
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError):
            pass


def staged_sha():
    """Hash of the staged tree: any later staging invalidates an approval."""
    return hashlib.sha1(staged_diff().encode("utf-8", "replace")).hexdigest()


def staged_paths():
    return [p for p in git("diff", "--cached", "--name-only").splitlines() if p.strip()]


def numstat():
    out = {}
    for line in git("diff", "--cached", "--numstat").splitlines():
        parts = line.split("\t")
        if len(parts) == 3:
            add, rem, path = parts
            out[path] = (-1 if add == "-" else int(add)) + (-1 if rem == "-" else int(rem))
    return out


def staged_blob(path):
    return git("show", ":" + path, check=False)


def index_note():
    """A pathspec commit (`git commit -- <paths>`) runs the hook against a restricted temporary index,
    so the sheet it reviews is not the staged tree the human approved - and an approval can never
    match. Say that, instead of failing with a hash mismatch nobody can explain."""
    tmp = os.environ.get("GIT_INDEX_FILE")
    if not tmp:
        return None
    real = git("rev-parse", "--git-path", "index").strip()
    if not os.path.isabs(real):
        real = os.path.join(ROOT, real)
    if os.path.abspath(tmp) != os.path.abspath(real):
        return ("this is a pathspec commit (`git commit -- <paths>`): the hook sees a restricted "
                "temporary index, so what is reviewed is not what the approval covers. Stage one "
                "concern at a time and commit the whole staged tree instead.")
    return None


def hard_blocks(paths, changes):
    """Reasons the commit must not happen at all (AGENTS.md rules 2 and 8)."""
    bad = []
    note = index_note()
    if note:
        bad.append(note)
    for p in paths:
        if p.startswith(BLOCKED_PREFIX) or p.endswith(BLOCKED_SUFFIX):
            bad.append("rule 2: staged build/original/scratch path `%s`" % p)
    if "AGENTS.md" in paths:
        text = staged_blob("AGENTS.md")
        markers = [i for i, ln in enumerate(text.splitlines(), 1) if ln.startswith("<!-- LOCAL-ONLY")]
        if markers:
            bad.append("rule 8: staged AGENTS.md still has LOCAL-ONLY marker line(s) %s "
                       "(run `python tools/agents/localonly.py pull` first)" % markers)
    return bad


def warnings(paths, changes, max_generated_lines):
    out = []
    for p in paths:
        if p in ("config/RMHE08/symbols.txt", "config/RMHE08/splits.txt"):
            n = changes.get(p, 0)
            if n > max_generated_lines:
                out.append("rule 2/'keep generated churn separate': `%s` has %d changed lines "
                           "(limit %d) - generated churn belongs in its own commit" % (p, n, max_generated_lines))
            else:
                out.append("`%s`: %d changed line(s) - check the diff is exactly what you meant" % (p, n))
        elif p == "configure.py":
            out.append("rule 3: `configure.py` staged - a flag/tool-version change needs the "
                       "instruction or size evidence in a comment next to it")
        elif p.startswith("config/"):
            out.append("`%s` staged - config is also the other window's area" % p)
        elif p.startswith("src/") or p.startswith("include/"):
            out.append("`%s` staged - source change: is it measured (objdiff-verify)?" % p)
    return out


def sheet(args, blocking=True):
    paths = staged_paths()
    changes = numstat()
    if not paths:
        print("commitgate: nothing staged.")
        return 0, []
    print("=" * 78)
    print("commit review - %d path(s), staged tree %s" % (len(paths), staged_sha()[:12]))
    print("=" * 78)
    print(git("status", "--short").rstrip() or "(clean)")
    print("\n-- diffstat --")
    print(git("diff", "--cached", "--stat", "-M").rstrip())
    diff = staged_diff_text()
    lines = diff.splitlines()
    show = args.diff_lines
    if show and show > 0:
        print("\n-- diff%s --" % ("" if len(lines) <= show else " (first %d of %d lines)" % (show, len(lines))))
        print("\n".join(lines[:show]))
    blocks = hard_blocks(paths, changes)
    warns = warnings(paths, changes, args.max_generated_lines)
    print("\n-- gate --")
    for b in blocks:
        print("  BLOCK  %s" % b)
    for w in warns:
        print("  warn   %s" % w)
    print("  check  ninja build/RMHE08/ok  must pass for anything touching the linked DOL")
    print("  check  no file under build/ or orig/ staged (rule 2), no compiler flags smuggled in (rule 3)")
    return (1 if blocks else 0), blocks


def approval():
    if not os.path.exists(STATE):
        return None
    try:
        st = json.load(open(STATE, encoding="utf-8"))
    except (OSError, ValueError):
        return None
    if st.get("schema") != SCHEMA or st.get("staged") != staged_sha():
        return None
    if time.time() - st.get("at", 0) > APPROVAL_MINUTES * 60:
        return None
    return st


def cmd_check(args):
    if os.environ.get("COMMITGATE") == "off":
        print("commitgate: BYPASSED by COMMITGATE=off", file=sys.stderr)
        return 0
    code, blocks = sheet(args)
    if code:
        print("\ncommitgate: refused (the block list above is not approvable).")
        return 1
    st = approval()
    if st:
        print("\ncommitgate: approved by %s at %s - %s"
              % (st.get("by", "?"), time.strftime("%H:%M", time.localtime(st["at"])), st.get("note", "")))
        return 0
    print("\ncommitgate: NOT approved - review the diff above, then:\n"
          "  python tools/agents/commitgate.py approve -m \"<who said what>\"\n"
          "and re-run the commit. (Changing anything staged invalidates an approval.)")
    return 1


def cmd_review(args):
    code, _ = sheet(args, blocking=False)
    return code


def cmd_approve(args):
    if os.environ.get("COMMITGATE") == "off":
        print("commitgate: BYPASSED", file=sys.stderr)
        return 0
    paths = staged_paths()
    if not paths:
        print("commitgate: nothing staged - nothing to approve.", file=sys.stderr)
        return 1
    _code, blocks = sheet(args, blocking=False)
    if blocks:
        print("\ncommitgate: refusing to approve - the block list above is not approvable.",
              file=sys.stderr)
        return 1
    os.makedirs(os.path.dirname(STATE), exist_ok=True)
    json.dump({"schema": SCHEMA, "staged": staged_sha(), "at": time.time(),
               "by": args.by, "note": args.note, "paths": paths},
              open(STATE, "w", encoding="utf-8"), indent=2)
    print("commitgate: approved %d staged path(s), tree %s - %s"
          % (len(paths), staged_sha()[:12], args.note))
    print("           (any further staging invalidates this; %d min validity)" % APPROVAL_MINUTES)
    return 0


def cmd_status(args):
    paths = staged_paths()
    print("staged: %d path(s), tree %s" % (len(paths), staged_sha()[:12] if paths else "-"))
    st = approval()
    if st:
        print("approval: valid, by %s, note %r" % (st.get("by"), st.get("note")))
    elif os.path.exists(STATE):
        old = json.load(open(STATE, encoding="utf-8"))
        print("approval: stale (recorded for tree %s, staged tree is %s)"
              % (str(old.get("staged"))[:12], staged_sha()[:12]))
    else:
        print("approval: none")
    print("hook: %s" % ("installed" if os.path.exists(HOOK) and "commitgate" in open(
        HOOK, encoding="utf-8", errors="replace").read() else "NOT installed (run `install`)"))
    return 0


def cmd_install(args):
    os.makedirs(os.path.dirname(HOOK), exist_ok=True)
    if os.path.exists(HOOK):
        cur = open(HOOK, encoding="utf-8", errors="replace").read()
        if "commitgate" not in cur:
            print("commitgate: %s already exists and is not ours - leaving it alone.\n%s"
                  % (HOOK, cur[:200]), file=sys.stderr)
            return 1
    with open(HOOK, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(HOOK_BODY)
    os.chmod(HOOK, os.stat(HOOK).st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)
    print("commitgate: installed %s" % os.path.relpath(HOOK, ROOT))
    return 0


def cmd_uninstall(args):
    if os.path.exists(HOOK) and "commitgate" in open(HOOK, encoding="utf-8", errors="replace").read():
        os.remove(HOOK)
        print("commitgate: removed %s" % os.path.relpath(HOOK, ROOT))
    else:
        print("commitgate: no hook of ours to remove.")
    return 0


def main():
    utf8_streams()
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name, fn in (("check", cmd_check), ("review", cmd_review), ("approve", cmd_approve),
                     ("status", cmd_status), ("install", cmd_install), ("uninstall", cmd_uninstall)):
        p = sub.add_parser(name, help={
            "check": "what the pre-commit hook runs: review sheet, then allow or block",
            "review": "print the review sheet without blocking",
            "approve": "record the human's approval of the staged tree",
            "status": "staged tree, approval state and hook state",
            "install": "write the .git/hooks/pre-commit shim",
            "uninstall": "remove our hook shim",
        }[name])
        p.add_argument("--diff-lines", type=int, default=150, help="how much of the diff to print")
        p.add_argument("--max-generated-lines", type=int, default=200,
                       help="changed-line limit before a symbols.txt/splits.txt edit is called churn")
        if name == "approve":
            p.add_argument("-m", "--note", required=True, help="who approved what (recorded)")
            p.add_argument("--by", default="user", help="who is approving")
        p.set_defaults(func=fn)
    args = ap.parse_args()
    sys.exit(args.func(args) or 0)


if __name__ == "__main__":
    main()
