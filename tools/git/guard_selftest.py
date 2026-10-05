"""Self-test for the ground-truth guard (docs/plan.md 7.18): the refusals, the DOL hash cross-check and the hook.

This is the one gate whose failure mode is silent and total - a rewritten `build.sha1` would make every later
`ninja build/RMHE08/ok` meaningless - so it is tested rather than trusted.

    python tools/git/guard_selftest.py
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.git import prepcommit as pc
from tools.git import guard

FAIL: list[str] = []
CHECKS = 0


def check(name: str, got, want) -> None:
    global CHECKS
    CHECKS += 1
    if got != want:
        FAIL.append(f"{name}: got {got!r}, want {want!r}")


# --- classify(): the ground truth and the original data are refused ------------------------------------------
for path in ("config/RMHE08/build.sha1", "orig/RMHE08/sys/main.dol",
             "build/RMHE08/main.dol", "build.ninja", "objdiff.json", ".pi/notes/x.md", ".lavish/a.html",
             "src/main.o"):
    check(f"refuse {path}", pc.classify(path)[0], "refuse")

# the ground truth gets its own reason, not the generic "build output" one
check("ground truth reason", "ground truth" in pc.classify("config/RMHE08/build.sha1")[1], True)

# --- config_change(): config.yml may change only in its relocation-analysis keys and comments -----------------
CFG = ("# Path to the main.dol file.\n"
       "object: orig/RMHE08/sys/main.dol\n"
       "hash: BF4850739478CAAEDFE675949EB7C28595A7FDE9\n"
       "\n"
       "selfile: orig/RMHE08/files/mh3.sel\n"
       "# (optional) gap symbols.\n"
       "fill_gaps: true\n")
BLOCK = ("\n# NETWORK_ERROR_* immediates are constants, not addresses.\n"
         "block_relocations:\n"
         "- target: extabindex:0x80020000\n"
         "  end: extabindex:0x80020010\n")
CFG_BLOCKED = CFG + BLOCK
CFG_BLOCKED2 = CFG_BLOCKED + "- target: extabindex:0x80030000\n  end: extabindex:0x80030050\n"

ALLOWED = {
    "add block_relocations": (CFG, CFG_BLOCKED),
    "add a block_relocations entry": (CFG_BLOCKED, CFG_BLOCKED2),
    "remove block_relocations": (CFG_BLOCKED, CFG),
    "add add_relocations": (CFG, CFG + "add_relocations:\n- source: 0x80001000\n  target: fn_80002000\n"),
    "edit a comment": (CFG, CFG.replace("# Path to the main.dol file.", "# The original DOL.")),
    "add a blank line": (CFG, CFG.replace("fill_gaps", "\nfill_gaps")),
    "CRLF vs LF only": (CFG, CFG.replace("\n", "\r\n")),
    "unchanged": (CFG, CFG),
}
REFUSED = {
    "change object": (CFG, CFG.replace("object: orig/RMHE08/sys/main.dol", "object: orig/RMHE08/sys/other.dol")),
    "change hash": (CFG, CFG.replace("BF48", "0000")),
    "change selfile": (CFG, CFG.replace("mh3.sel", "mh4.sel")),
    "delete a top-level key": (CFG, CFG.replace("fill_gaps: true\n", "")),
    "rename a key": (CFG, CFG.replace("fill_gaps:", "fill_gap:")),
    "add an unrelated key": (CFG, CFG + "write_asm: true\n"),
    "mixed allowed + refused": (CFG, CFG_BLOCKED.replace("fill_gaps: true", "fill_gaps: false")),
    "a key repeated": (CFG, CFG + BLOCK + BLOCK),
    "a column-0 line that is no key": (CFG, CFG + "block_relocations\n"),
    "file created": (None, CFG),
    "file deleted": (CFG, None),
}
for name, (old, new) in ALLOWED.items():
    verdict = guard.config_change(old, new)
    check(f"config allows: {name}", verdict["ok"], True)
    check(f"prepcommit stages: {name}", pc.classify("config/RMHE08/config.yml", (old, new))[0], "stage")
for name, (old, new) in REFUSED.items():
    verdict = guard.config_change(old, new)
    check(f"config refuses: {name}", verdict["ok"], False)
    check(f"prepcommit refuses: {name}", pc.classify("config/RMHE08/config.yml", (old, new))[0], "refuse")
check("the refusal names the frozen key", "`selfile`" in guard.config_change(*REFUSED["change selfile"])["reason"],
      True)
check("the mixed refusal names only the frozen key",
      "changes `fill_gaps`;" in guard.config_change(*REFUSED["mixed allowed + refused"])["reason"], True)
check("the changed keys are reported", guard.config_change(CFG, CFG_BLOCKED)["changed"], ["block_relocations"])
check("a trailing comment on a value line is content",
      guard.config_change(CFG, CFG.replace("fill_gaps: true", "fill_gaps: true # x"))["ok"], False)

# --- classify(): real work is still staged -------------------------------------------------------------------
for path in ("src/Pl/pl_act.cpp", "tools/units/ledger.py", "docs/plan.md", "configure.py", "CLAUDE.md",
             "config/RMHE08/splits.txt", "config/RMHE08/symbols.txt", "src/types.h"):
    check(f"stage {path}", pc.classify(path)[0], "stage")

# --- classify(): include/ is retired (2026-10-05) - a new or changed path refused, a deletion staged ----------
for code in ("??", "A", "M", "R"):
    check(f"refuse a {code} path under include/", pc.classify("include/Net/x.h", code=code)[0], "refuse")
check("the refusal says where headers live", "src/" in pc.classify("include/Net/x.h", code="??")[1], True)
check("a deletion under include/ is staged", pc.classify("include/Net/x.h", code="D")[0], "stage")

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
    for rel in ("tools/git/guard.py", "tools/git/hooks/pre-commit", "tools/__init__.py", "tools/lib/__init__.py",
                "tools/lib/git.py", "tools/lib/proc.py", "tools/lib/repo.py", "tools/lib/testing.py"):
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

# 4. config.yml and the path refusals through the real hook: a repo whose first commit (hook bypassed) holds the
#    ground truth, then one commit attempt per case, each from that same base.
def ground_truth_repo() -> str:
    repo = temp_repo()
    os.makedirs(os.path.join(repo, "config", "RMHE08"))
    write(repo, "config/RMHE08/config.yml", CFG)
    write(repo, "config/RMHE08/build.sha1", "BF4850739478CAAEDFE675949EB7C28595A7FDE9  build/RMHE08/main.dol\n")
    run(repo, "add", "-A")
    run(repo, "commit", "-q", "--no-verify", "-m", "base")
    return repo


def write(repo: str, rel: str, text: str) -> None:
    path = os.path.join(repo, *rel.split("/"))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


HOOK_CASES = (  # (name, {path: new text | None to delete}, should the commit land?)
    ("add block_relocations", {"config/RMHE08/config.yml": CFG_BLOCKED}, True),
    ("edit a config comment", {"config/RMHE08/config.yml": ALLOWED["edit a comment"][1]}, True),
    ("change object", {"config/RMHE08/config.yml": REFUSED["change object"][1]}, False),
    ("change selfile", {"config/RMHE08/config.yml": REFUSED["change selfile"][1]}, False),
    ("delete a top-level key", {"config/RMHE08/config.yml": REFUSED["delete a top-level key"][1]}, False),
    ("rename a key", {"config/RMHE08/config.yml": REFUSED["rename a key"][1]}, False),
    ("mixed allowed + refused", {"config/RMHE08/config.yml": REFUSED["mixed allowed + refused"][1]}, False),
    ("delete config.yml", {"config/RMHE08/config.yml": None}, False),
    ("touch build.sha1", {"config/RMHE08/build.sha1": "0" * 40 + "  build/RMHE08/main.dol\n"}, False),
    ("delete build.sha1", {"config/RMHE08/build.sha1": None}, False),
    ("allowed config + build.sha1", {"config/RMHE08/config.yml": CFG_BLOCKED,
                                     "config/RMHE08/build.sha1": "0" * 40 + "\n"}, False),
    ("a file under orig/", {"orig/RMHE08/sys/main.dol": "dol"}, False),
    ("a file under build/", {"build/RMHE08/main.elf": "elf"}, False),
    ("an ordinary file", {"src/a.c": "int a;\n"}, True),
)
for name, edits, lands in HOOK_CASES:
    repo = ground_truth_repo()
    try:
        head = run(repo, "rev-parse", "HEAD").stdout.strip()
        for rel, text in edits.items():
            if text is None:
                run(repo, "rm", "-q", "--", rel)
            else:
                write(repo, rel, text)
                run(repo, "add", "-f", "--", rel)
        result = run(repo, "commit", "-m", "t")
        out = result.stdout + result.stderr
        moved = run(repo, "rev-parse", "HEAD").stdout.strip() != head
        check(f"hook {'accepts' if lands else 'refuses'}: {name}", (result.returncode == 0, moved), (lands, lands))
        if not lands:
            check(f"hook refusal names the path: {name}", any(rel in out for rel in edits), True)
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
