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
for path in ("src/Pl/pl_act.cpp", "tools/units/ledger.py", "docs/plan.md", "configure.py", "AGENTS.md",
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
check("improved + AGENTS.md: no warning", pc.knowledge_delta_warning(["AGENTS.md"], True), None)
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

# --- localonly_case(): the marker must be a whole line, or rule 8's own prose matches it --------------------
check("localonly_case: no AGENTS.md staged", guard.localonly_case(["src/x.c"]), "nothing")
check("localonly_case: a real marker line",
      guard.localonly_case(["AGENTS.md"], "head\n<!-- LOCAL-ONLY-BEGIN -->\nbody\n"),
      "pull the block, re-stage AGENTS.md")
check("localonly_case: rule 8's prose quoting the marker is not a marker line",
      guard.localonly_case(["AGENTS.md"], "run `grep -c '^<!-- LOCAL-ONLY'` to check\n"), "nothing")
check("localonly_case: a marker mentioned mid-line does not match",
      guard.localonly_case(["AGENTS.md"], "the `<!-- LOCAL-ONLY-BEGIN` marker is quoted here\n"), "nothing")


# --- the new guard behaviours, end to end through a real hook in a throwaway repo ----------------------

BLOCK = ("<!-- LOCAL-ONLY-BEGIN: stripped before every commit, see Non-negotiables rule 8 -->\n"
         "live agent working state\n"
         "<!-- LOCAL-ONLY-END -->\n")


def temp_repo() -> str:
    """A throwaway repo with its own copy of the hooks and tools, so the hook runs there and not here."""
    tmp = tempfile.mkdtemp(prefix="guard-selftest-")
    for rel in ("tools/git/guard.py", "tools/git/hooks/pre-commit", "tools/git/hooks/post-commit",
                "tools/agents/localonly.py"):
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


# 1. auto-pull: a staged AGENTS.md that carries the block is fixed, committed and restored, not refused.
repo = temp_repo()
try:
    with open(os.path.join(repo, "AGENTS.md"), "w", encoding="utf-8", newline="") as fh:
        fh.write("head\n" + BLOCK + "tail\n")
    run(repo, "add", "AGENTS.md")
    check("auto-pull: the staged blob really carries the block",
          b"LOCAL-ONLY-BEGIN" in blob(repo, ":AGENTS.md"), True)
    result = run(repo, "commit", "-m", "t")
    out = result.stdout + result.stderr
    check("auto-pull: the commit succeeds", result.returncode, 0)
    check("auto-pull: the hook says it pulled and re-staged", "pulled the block and re-staged" in out, True)
    check("auto-pull: the committed AGENTS.md has no marker", b"LOCAL-ONLY" in blob(repo, "HEAD:AGENTS.md"), False)
    with open(os.path.join(repo, "AGENTS.md"), "rb") as fh:
        worktree = fh.read()
    check("auto-pull: post-commit restored the block to the worktree", b"LOCAL-ONLY-BEGIN" in worktree, True)
    check("auto-pull: post-commit removed its marker",
          os.path.exists(os.path.join(repo, ".git", "localonly-pending")), False)
    check("auto-pull: post-commit consumed the state file",
          os.path.exists(os.path.join(repo, ".pi", "local-only.state.json")), False)
finally:
    shutil.rmtree(repo, ignore_errors=True)

# 2. CRLF normalise: a staged text blob with CRs is rewritten to LF on disk and in the index.
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

# 3. binary refusal: a staged blob with a NUL byte and a CR is refused, never rewritten.
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

# 4. autocrlf: a warning, never a refusal (it is a clone-local setting).
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

# 5. localonly EOL cycle: the fixture's EOL changes between pull and push; push must still place the block.
repo = temp_repo()
try:
    agents = os.path.join(repo, "AGENTS.md")
    crlf_block = BLOCK.replace("\n", "\r\n").encode()
    with open(agents, "wb") as fh:
        fh.write(b"# AGENTS\r\n\r\nbefore the block\r\n" + crlf_block + b"after the block\r\n")
    lonly = [sys.executable, os.path.join(repo, "tools", "agents", "localonly.py")]
    pull = subprocess.run(lonly + ["pull"], cwd=repo, capture_output=True, text=True,
                          encoding="utf-8", errors="replace")
    state = os.path.join(repo, ".pi", "local-only.state.json")
    check("localonly eol: pull succeeds", pull.returncode, 0)
    check("localonly eol: pull stored the block", os.path.exists(state), True)
    with open(agents, "rb") as fh:
        data = fh.read()
    with open(agents, "wb") as fh:
        fh.write(data.replace(b"\r\n", b"\n"))  # the EOL change between pull and push
    push = subprocess.run(lonly + ["push"], cwd=repo, capture_output=True, text=True,
                          encoding="utf-8", errors="replace")
    restored = open(agents, "rb").read()
    check("localonly eol: push succeeds after the EOL change", push.returncode, 0)
    check("localonly eol: the block is back exactly once", restored.count(b"<!-- LOCAL-ONLY-BEGIN"), 1)
    check("localonly eol: the block went back as LF", b"\r" in restored, False)
    check("localonly eol: the state file is consumed", os.path.exists(state), False)
    run(repo, "add", "AGENTS.md")
    committed = run(repo, "commit", "-m", "t")
    check("localonly eol: the block commits stripped", committed.returncode, 0)
    verify = subprocess.run(lonly + ["verify", "--rev", "HEAD"], cwd=repo, capture_output=True,
                            text=True, encoding="utf-8", errors="replace")
    check("localonly eol: verify passes on the committed file", verify.returncode, 0)
finally:
    shutil.rmtree(repo, ignore_errors=True)


# --- the hook: present, executable in the index where git records modes, and covering the four refusals -------
hook = os.path.join(ROOT, "tools", "git", "hooks", "pre-commit")
check("hook exists", os.path.exists(hook), True)
body = open(hook, encoding="utf-8").read() if os.path.exists(hook) else ""
for needle in ("orig/", "build/", "build\\.sha1", "config\\.yml", "LOCAL-ONLY"):
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
