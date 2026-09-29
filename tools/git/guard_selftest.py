"""Self-test for the ground-truth guard (docs/plan.md 7.18): the refusals, the DOL hash cross-check and the hook.

This is the one gate whose failure mode is silent and total - a rewritten `build.sha1` would make every later
`ninja build/RMHE08/ok` meaningless - so it is tested rather than trusted.

    python tools/git/guard_selftest.py
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "git"))

import prepcommit as pc  # noqa: E402
import guard  # noqa: E402

FAIL: list[str] = []
CHECKS = 0


def check(name: str, got, want) -> None:
    global CHECKS
    CHECKS += 1
    if got != want:
        FAIL.append(f"{name}: got {got!r}, want {want!r}")


# --- classify(): the ground truth and the original data are refused ------------------------------------------
for path in ("config/RMHE08/build.sha1", "config/RMHE08/config.yml", "orig/RMHE08/sys/main.dol",
             "build/RMHE08/main.dol", "build.ninja", "objdiff.json", ".pi/notes/x.md", ".lavish/a.html",
             "src/main.o"):
    check(f"refuse {path}", pc.classify(path)[0], "refuse")

# the ground truth gets its own reason, not the generic "build output" one
check("ground truth reason", "ground truth" in pc.classify("config/RMHE08/build.sha1")[1], True)

# --- classify(): real work is still staged -------------------------------------------------------------------
for path in ("src/Pl/pl_act.cpp", "tools/units/ledger.py", "docs/plan.md", "configure.py", "CLAUDE.md",
             "config/RMHE08/splits.txt", "config/RMHE08/symbols.txt", "include/types.h"):
    check(f"stage {path}", pc.classify(path)[0], "stage")

# --- ground_truth_error(): passes here, catches a rewritten hash, tolerates a missing DOL ---------------------
check("ground truth on this tree", pc.ground_truth_error(), [])

with tempfile.TemporaryDirectory() as tmp:
    dol = os.path.join(tmp, "main.dol")
    sha = os.path.join(tmp, "build.sha1")
    open(dol, "wb").write(b"the original dol bytes")
    import hashlib
    real = hashlib.sha1(b"the original dol bytes").hexdigest().upper()

    open(sha, "w", encoding="utf-8").write(f"{real}  build/RMHE08/main.dol\n")
    check("matching hash", pc.ground_truth_error(dol, sha), [])

    open(sha, "w", encoding="utf-8").write("0" * 40 + "  build/RMHE08/main.dol\n")
    wrong = pc.ground_truth_error(dol, sha)
    check("tampered hash detected", len(wrong), 1)
    check("tampered hash names both files", ("build.sha1" in wrong[0] and "main.dol" in wrong[0]), True)

    open(sha, "w", encoding="utf-8").write("\n")
    check("empty hash file detected", len(pc.ground_truth_error(dol, sha)), 1)
    check("missing dol tolerated", pc.ground_truth_error(os.path.join(tmp, "nope.dol"), sha), [])

# --- knowledge delta (7.10): an improved batch must carry a document, header or flag comment ----------------
check("improved + src header: no warning", pc.knowledge_delta_warning(["src/Mod/unit.c"], True), None)
check("improved + a .cp header: no warning", pc.knowledge_delta_warning(["src/Gecko/Gecko_ExceptionPPC.cp"], True), None)
check("improved + docs: no warning", pc.knowledge_delta_warning(["docs/plan.md"], True), None)
check("improved + CLAUDE.md: no warning", pc.knowledge_delta_warning(["CLAUDE.md"], True), None)
check("improved + configure.py: no warning", pc.knowledge_delta_warning(["configure.py"], True), None)
delta = pc.knowledge_delta_warning(["config/RMHE08/splits.txt"], True)
check("improved with no knowledge: warns", delta is not None, True)
check("the warning names 7.10", "7.10" in (delta or ""), True)
check("no improvement: no warning", pc.knowledge_delta_warning(["config/RMHE08/splits.txt"], False), None)
check("an improved batch with no knowledge path warns",
      pc.knowledge_delta_warning([], True) is not None, True)

# ledger_improved() is land.py verify's rule, with the report's string byte counts coerced
check("ledger improved: closed",
      pc.ledger_improved({"closed": 10, "matched": 5, "bytes": "100"},
                         {"closed": 11, "matched": 5, "bytes": "100"}), True)
check("ledger improved: bytes as strings", pc.ledger_improved({"bytes": "100"}, {"bytes": "200"}), True)
check("a flat ledger is not improved",
      pc.ledger_improved({"closed": 10, "matched": 5, "bytes": "100"},
                         {"closed": 10, "matched": 5, "bytes": "100"}), False)
check("missing sides are not improved", pc.ledger_improved({}, {}), False)
check("improved_since_base returns a bool", isinstance(pc.improved_since_base(), bool), True)

# --- the new guard behaviours, end to end through a real hook in a throwaway repo ----------------------

def temp_repo() -> str:
    """A throwaway repo with its own copy of the hooks and tools, so the hook runs there and not here."""
    tmp = tempfile.mkdtemp(prefix="guard-selftest-")
    for rel in ("tools/git/guard.py", "tools/git/hooks/pre-commit"):
        dst = os.path.join(tmp, rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy(os.path.join(ROOT, rel), dst)
        os.chmod(dst, 0o755)
    for args in (("init", "-q"),
                 ("config", "core.hooksPath", "tools/git/hooks"),
                 ("config", "core.autocrlf", "false"),
                 ("config", "user.email", "guard@selftest"),
                 ("config", "user.name", "guard selftest")):
        subprocess.run(["git", *args], cwd=tmp, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return tmp


def run(repo: str, *args: str):
    return subprocess.run(["git", "-C", repo, *args], capture_output=True, text=True,
                          encoding="utf-8", errors="replace")


def blob(repo: str, spec: str) -> bytes:
    return subprocess.run(["git", "-C", repo, "cat-file", "-p", spec], capture_output=True).stdout


# 1. CRLF normalise: a staged text blob with CRs is rewritten to LF on disk and in the index.
repo = temp_repo()
try:
    os.makedirs(os.path.join(repo, "src"))
    path = os.path.join(repo, "src", "note.txt")
    with open(path, "wb") as fh:
        fh.write(b"alpha\r\nbeta\r\ngamma\r\n")
    run(repo, "add", "src/note.txt")
    check("crlf: the staged blob really has CRs", b"\r" in blob(repo, ":src/note.txt"), True)
    result = run(repo, "commit", "-m", "t")
    out = result.stdout + result.stderr
    check("crlf: the commit succeeds", result.returncode, 0)
    check("crlf: the hook says it normalised the file", "normalised" in out and "re-staged" in out, True)
    check("crlf: the committed blob is LF", b"\r" in blob(repo, "HEAD:src/note.txt"), False)
    with open(path, "rb") as fh:
        check("crlf: the worktree file is LF too", b"\r" in fh.read(), False)
finally:
    shutil.rmtree(repo, ignore_errors=True)

# 2. binary refusal: a staged blob with a NUL byte and a CR is refused, never rewritten.
repo = temp_repo()
try:
    path = os.path.join(repo, "blob.bin")
    with open(path, "wb") as fh:
        fh.write(b"\x00\x01\r\x02\x00")
    run(repo, "add", "blob.bin")
    check("binary: the staged blob has a CR", b"\r" in blob(repo, ":blob.bin"), True)
    result = run(repo, "commit", "-m", "t")
    out = result.stdout + result.stderr
    check("binary: the commit is refused", result.returncode != 0, True)
    check("binary: the refusal names the file", "blob.bin" in out, True)
    check("binary: the refusal says why", "binary" in out, True)
    check("binary: no commit was made", run(repo, "rev-parse", "--verify", "HEAD").returncode != 0, True)
finally:
    shutil.rmtree(repo, ignore_errors=True)

# 3. autocrlf: a warning, never a refusal (it is a clone-local setting).
repo = temp_repo()
try:
    run(repo, "config", "core.autocrlf", "true")
    with open(os.path.join(repo, "plain.txt"), "w", encoding="utf-8") as fh:
        fh.write("hello\n")
    run(repo, "add", "plain.txt")
    result = run(repo, "commit", "-m", "t")
    out = result.stdout + result.stderr
    check("autocrlf: the commit still succeeds", result.returncode, 0)
    check("autocrlf: the hook warns", "core.autocrlf=true" in out, True)
finally:
    shutil.rmtree(repo, ignore_errors=True)

# --- the hook: present, executable in the index where git records modes, and covering the four refusals -------
hook = os.path.join(ROOT, "tools", "git", "hooks", "pre-commit")
check("hook exists", os.path.exists(hook), True)
body = open(hook, encoding="utf-8").read() if os.path.exists(hook) else ""
for needle in ("orig/", "build/", "build\\.sha1", "config\\.yml"):
    check(f"hook covers {needle}", needle in body, True)
check("hook is a shell script", body.startswith("#!/bin/sh"), True)

tracked = subprocess.run(["git", "ls-files", "-s", "--", "tools/git/hooks/pre-commit"], cwd=ROOT,
                         capture_output=True, text=True, encoding="utf-8", errors="replace").stdout.split()
if tracked:
    check("hook mode in the index is 100755", tracked[0], "100755")
    check("hook has no CRLF", "\r" in body, False)

if FAIL:
    print(f"FAIL ({len(FAIL)})")
    for line in FAIL:
        print("  " + line)
    sys.exit(1)
print(f"ok - {CHECKS} checks")
