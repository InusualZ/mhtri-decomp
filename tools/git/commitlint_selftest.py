"""Self-test for `tools/git/commitlint.py` - the category sets, the five checks and the three modes.

Every check runs against a **fixture tree** and a **fixture git history** built in a temp directory, never
against this repository's working tree or HEAD, so the result cannot move when the tree does. The two
mechanisms that decide the lint - the derived member sets and the message checks - are exercised both as
pure functions and through the CLI, so the exit codes (0/1/2) are pinned too.

    python tools/git/commitlint_selftest.py
    python tools/git/commitlint.py --selftest          (the same checks)
"""

from __future__ import annotations

import os
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import commitlint as cl  # noqa: E402

CHECKS = 0
FAIL: list = []


def check(name: str, got, want) -> None:
    global CHECKS
    CHECKS += 1
    if got != want:
        FAIL.append("%s: got %r, want %r" % (name, got, want))


def run(*args: str, cwd: str = None):
    """The CLI as a subprocess - the way a hook or a gate reaches it."""
    proc = subprocess.run([sys.executable, os.path.join(HERE, "commitlint.py"), *args],
                          capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=cwd)
    return proc.returncode, proc.stdout, proc.stderr


def build_tree(root: str) -> None:
    """A miniature repository: enough shape for every family to have a known member."""
    for rel in ("src/Network", "src/quest",
                "tools/units", "tools/__pycache__", "tools/agents",     # a grouping dir, a private dir
                ".agents/agents", "docs"):
        os.makedirs(os.path.join(root, rel), exist_ok=True)
    for rel in ("src/draw_shape.cpp",                                # a top-level src file is not a module
                "README.md", "LICENSE", ".github.example", ".gitignore",
                "tools/selftest.py", "tools/__init__.py", "tools/units/land.py",
                ".agents/agents/decompiler.md", ".agents/agents/surveyor.md",
                "docs/plan.md", "docs/pipeline.md"):
        with open(os.path.join(root, rel), "w", encoding="utf-8") as handle:
            handle.write("\n")


def init_repo(root: str) -> None:
    env = dict(os.environ, GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_SYSTEM=os.devnull)
    subprocess.run(["git", "init", "-q", root], check=True, env=env,
                   capture_output=True, text=True, encoding="utf-8", errors="replace")
    for args in (["config", "user.email", "selftest@example.invalid"],
                 ["config", "user.name", "commitlint selftest"],
                 ["config", "commit.gpgsign", "false"]):
        subprocess.run(["git", "-C", root] + args, check=True, env=env, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")


def commit(root: str, message: str) -> None:
    env = dict(os.environ, GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_SYSTEM=os.devnull)
    subprocess.run(["git", "-C", root, "commit", "-q", "--allow-empty", "-m", message],
                   check=True, env=env, capture_output=True, text=True, encoding="utf-8", errors="replace")


def _findings(text: str, members: dict) -> list:
    return [(f["level"], f["message"]) for f in cl.lint_message(text, members)]


def _errors(text: str, members: dict) -> list:
    return [m for level, m in _findings(text, members) if level == "error"]


# ==================================================================================================
def selftest() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = os.path.join(tmp, "repo")
        os.makedirs(root)
        build_tree(root)

        # --- 1. the member sets are derived from the tree, not hard-coded -----------------------------------
        members = cl.derive_members(root)
        check("game members are the src/ directories", members["game"], {"Network", "quest"})
        check("game does not take a top-level src file as a module", "draw_shape" in members["game"], False)
        check("tools members are the groupings plus the top-level scripts",
              members["tools"], {"units", "selftest", "agents"})
        check("tools does not take __pycache__", "__pycache__" in members["tools"], False)
        check("tools does not take the __init__ shim", "__init__" in members["tools"], False)
        check("agents members are the profiles plus `policy`",
              members["agents"], {"policy", "decompiler", "surveyor"})
        check("config has the convention's three members", members["config"], {"flags", "symbols", "splits"})
        check("docs members are the documents", members["docs"], {"plan", "pipeline"})
        check("repo members are read from the root files",
              members["repo"], {"readme", "license", "ci", "gitignore"})

        # a derived set follows the tree: a new module becomes valid the moment the directory exists
        os.makedirs(os.path.join(root, "src", "enemy"))
        check("a new src/ directory joins the game family",
              "enemy" in cl.derive_members(root)["game"], True)
        os.rmdir(os.path.join(root, "src", "enemy"))

        # --- 2. the five checks, as pure functions ----------------------------------------------------------
        # a passing message
        check("a passing subject has no finding",
              _findings("game/network: match the transport state machine", members), [])
        # each family: known members pass.  `game/network` is lower-case here on purpose: the convention's
        # own examples lower-case the prose while the tree directory is `Network`, so the member match is
        # case-insensitive and both spellings pass.
        for good in ("game/network: x", "game/Network: x", "game/quest: x", "tools/units: x",
                     "tools/selftest: x", "agents/decompiler: x", "agents/policy: x", "config/flags: x",
                     "config/symbols: x", "config/splits: x", "docs/plan: x", "repo/readme: x",
                     "repo/gitignore: x"):
            check("known member passes: %s" % good, _errors(good, members), [])

        # 2a. subject shape: a missing colon
        missing = _errors("game/network match the transport state machine", members)
        check("a missing colon is one error", len(missing), 1)
        check("the missing-colon error names the shape",
              "not `<category>: <message>`" in missing[0], True)
        check("a colon with no space is refused too", len(_errors("game/network:msg", members)), 1)

        # 2b. an invented module in a known family is an ERROR ...
        invented = _errors("game/nosuchmodule: do a thing", members)
        check("an invented member of a known family is an error", len(invented), 1)
        check("the invented-module error names the category", "game/nosuchmodule" in invented[0], True)
        # ... and a typo is named with the member it probably meant (the tree's own casing)
        typo = _errors("game/netwrok: do a thing", members)
        check("a typo'd member is an error", len(typo), 1)
        check("the typo's error suggests the close member", "game/Network" in typo[0], True)
        check("a bare known family is an error (no member)",
              len(_errors("game: do a thing", members)), 1)
        # ... while an unknown FAMILY is a WARNING (the list is open)
        unknown = _findings("widget/foo: do a thing", members)
        check("an unknown family is one warning", len(unknown), 1)
        check("the unknown family warns, it does not error", unknown[0][0], "warning")
        check("an unknown family alone does not fail the CLI's error count",
              _errors("widget/foo: do a thing", members), [])

        # 2c. length: 120 is fine, 121 is not, counted after `<category>: `
        msg120 = "a" * 120
        msg121 = "b" * 121
        check("a 120-character message passes",
              _errors("game/network: " + msg120, members), [])
        over = _errors("game/network: " + msg121, members)
        check("a 121-character message is an error", len(over), 1)
        check("the length error reports the actual count", "121 characters" in over[0], True)
        check("the length error states the limit", "limit is 120" in over[0], True)
        # the category and its two-character separator are not charged against the 120
        check("the category is not counted toward the limit",
              _errors("game/network: " + msg120, members), [])
        check("a 118-character message plus a category still passes",
              _errors("game/network: " + "c" * 118, members), [])

        # 2d. an empty / blank first line
        check("an empty message is an error", len(_errors("", members)), 1)
        check("a whitespace-only message is an error", len(_errors("   \n\n", members)), 1)
        check("the empty-subject error says so", "first line is empty" in _errors("", members)[0], True)

        # 2e. a subject, not a wall: the body must follow a blank line
        wall = _errors("game/network: match it\nthis is the body with no blank line", members)
        check("a body with no blank line is an error", len(wall), 1)
        check("the wall error names the blank line",
              "blank line" in wall[0], True)
        check("a proper blank-line body passes",
              _errors("game/network: match it\n\nthe body", members), [])
        check("a trailing newline alone is still a subject-only message",
              _errors("game/network: match it\n", members), [])

        # git's own comment lines (an editor-driven commit) are not read as the body
        check("a `#` comment line is ignored, not read as the body",
              _errors("game/network: match it\n# Please enter the commit message", members), [])

        # --- 3. the CLI: a message file (the hook shape) ----------------------------------------------------
        good_file = os.path.join(tmp, "good.txt")
        with open(good_file, "w", encoding="utf-8", newline="\n") as handle:
            handle.write("game/network: match the transport state machine\n\nthe body\n")
        rc, out, _ = run(good_file, "--root", root)
        check("a good message file exits 0", rc, 0)
        check("a good message file reports ok", ": ok" in out, True)

        bad_file = os.path.join(tmp, "bad.txt")
        with open(bad_file, "w", encoding="utf-8", newline="\n") as handle:
            handle.write("game/nosuchmodule: do a thing\n")
        rc, out, _ = run(bad_file, "--root", root)
        check("a bad message file exits 1", rc, 1)
        check("a bad message file reports FAIL", ": FAIL" in out, True)

        rc, _, err = run(os.path.join(tmp, "absent.txt"), "--root", root)
        check("an unreadable message file exits 2 (nothing checked)", rc, 2)

        # --- 4. the CLI: --message --------------------------------------------------------------------------
        rc, out, _ = run("--message", "game/network: match it", "--root", root)
        check("--message with a good subject exits 0", rc, 0)
        rc, _, _ = run("--message", "game/network match it", "--root", root)
        check("--message with no colon exits 1", rc, 1)
        rc, _, _ = run("--message", "widget/foo: do a thing", "--root", root)
        check("--message with an unknown family exits 0 (warning only)", rc, 0)
        rc, out, _ = run("--message", "widget/foo: do a thing", "--root", root)
        check("... and prints the warning", "warning:" in out, True)

        # no mode at all is "nothing checked" (argparse's exit 2)
        rc, _, _ = run("--root", root)
        check("no mode exits 2 (nothing checked)", rc, 2)

        # --- 5. the CLI: --last N over a fixture history ----------------------------------------------------
        history = os.path.join(tmp, "history")
        os.makedirs(history)
        build_tree(history)
        init_repo(history)
        commit(history, "game/network match it")              # oldest: a violation
        commit(history, "game/network: match the quest state")  # good
        commit(history, "tools/selftest: add a check")        # newest: good

        rc, out, _ = run("--last", "3", "--root", history)
        check("--last over a mixed history exits 1 (a violation is present)", rc, 1)
        check("--last counts the clean and the violating commits",
              "2 clean, 1 with violations" in out, True)
        check("--last names the violating commit", "FAIL: game/network match it" in out, True)

        rc, out, _ = run("--last", "1", "--root", history)
        check("--last 1 over a good tip exits 0", rc, 0)
        check("--last 1 counts one clean commit", "1 clean, 0 with violations" in out, True)

        rc, _, err = run("--last", "0", "--root", history)
        check("--last 0 exits 2 (nothing checked)", rc, 2)

        empty = os.path.join(tmp, "empty")
        os.makedirs(empty)
        init_repo(empty)
        rc, _, err = run("--last", "5", "--root", empty)
        check("--last over an empty history exits 2", rc, 2)

        # --- 6. --install-hook ------------------------------------------------------------------------------
        rc, out, _ = run("--install-hook", "--root", history)
        check("--install-hook exits 0", rc, 0)
        hook = os.path.join(history, ".git", "hooks", "commit-msg")
        check("the hook lands at .git/hooks/commit-msg", os.path.isfile(hook), True)
        with open(hook, encoding="utf-8") as handle:
            body = handle.read()
        check("the hook is sh-compatible", body.startswith("#!/bin/sh"), True)
        check('the hook calls the tool with "$1"', '"$1"' in body, True)
        check("the hook is this tool's (its marker is present)", cl.HOOK_MARKER in body, True)
        if os.name == "posix":
            check("the hook is mode 0755",
                  stat.S_IMODE(os.stat(hook).st_mode), 0o755)
        rc, _, _ = run("--install-hook", "--root", history)
        check("installing again is idempotent (exit 0)", rc, 0)

        with open(hook, "w", encoding="utf-8") as handle:
            handle.write("#!/bin/sh\necho someone else's hook\n")
        rc, _, err = run("--install-hook", "--root", history)
        check("a foreign hook is not clobbered (exit 1)", rc, 1)
        rc, _, _ = run("--install-hook", "--force", "--root", history)
        check("--force replaces it", rc, 0)
        with open(hook, encoding="utf-8") as handle:
            check("... with this tool's hook", cl.HOOK_MARKER in handle.read(), True)

        # --- 7. the module's own check (find_root) is exercised by --root above ------------------------------
        check("find_root honours an explicit root", cl.find_root(root), os.path.abspath(root))

    if FAIL:
        print("FAIL (%d)" % len(FAIL))
        for line in FAIL:
            print("  " + line)
        return 1
    print("ok - %d checks" % CHECKS)
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
