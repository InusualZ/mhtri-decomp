#!/usr/bin/env python3
"""Pull and push the LOCAL-ONLY working-state section of AGENTS.md.

Non-negotiables rule 8: everything between the two LOCAL-ONLY markers is live agent working state and
must never be committed, so before a commit that touches AGENTS.md the section has to come out of the
file - and afterwards it has to go back exactly where it was.

    localonly.py pull     # remove the section, store it in a state file
    git add AGENTS.md && git commit ...
    localonly.py push     # put the stored section back where it belongs

The state file (default `.pi/local-only.state.json`, gitignored) records the removed bytes plus the
text on either side of the cut, so `push` restores the file byte-for-byte; it also records the file's
hash before the pull so the round trip can be reported.

Do not do this with sed/grep by hand: the markers are also *mentioned as text* inside rule 8 further
down the file, so any pattern that is not anchored at the start of a line matches four times, and a
range-based edit silently eats the wrong region.

Other commands: `status` (is the section present / is a state file pending) and `verify` (run rule 8's
check against a committed revision: `git show <rev>:AGENTS.md | grep -c '^<!-- LOCAL-ONLY'` must be 0).
"""
import argparse
import datetime
import difflib
import hashlib
import json
import os
import shutil
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BEGIN = "<!-- LOCAL-ONLY-BEGIN: stripped before every commit, see Non-negotiables rule 8 -->"
END = "<!-- LOCAL-ONLY-END -->"
DEFAULT_FILE = "AGENTS.md"
DEFAULT_STATE = ".pi/local-only.state.json"
ANCHOR = 240
BACKUP_KEEP = 5


def sha1(data):
    return hashlib.sha1(data.encode("utf-8")).hexdigest()


def read(path):
    with open(path, "r", encoding="utf-8", newline="") as fh:
        return fh.read()


def write(path, text):
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)
    os.replace(tmp, path)


def backup_path(state, when=None):
    """The timestamped backup path for a state file.

    The stamp is the pull's own clock, formatted so a plain name sort is a time sort - which is what
    `newest_backup` and `prune_backups` rely on instead of stat calls.
    """
    when = when or datetime.datetime.now()
    return "%s.bak-%s" % (state, when.strftime("%Y%m%d-%H%M%S"))


def list_backups(state):
    """Every backup of `state`, oldest first.  The state file itself is not a backup."""
    directory = os.path.dirname(state) or "."
    if not os.path.isdir(directory):
        return []
    prefix = os.path.basename(state) + ".bak-"
    return sorted(os.path.join(directory, n) for n in os.listdir(directory) if n.startswith(prefix))


def newest_backup(state):
    backups = list_backups(state)
    return backups[-1] if backups else None


def prune_backups(state, keep=BACKUP_KEEP):
    """Drop all but the newest `keep` backups and return the paths removed.

    A pull is the moment the block's only copy moves into the state file, so the snapshot it leaves
    has to survive a mistake - but an unbounded pile of 100 KB snapshots is its own problem.
    """
    backups = list_backups(state)
    removed = []
    for path in (backups[:-keep] if keep else backups):
        try:
            os.remove(path)
            removed.append(path)
        except OSError:
            pass
    return removed


def no_state_message(state):
    """The refusal `push` gives when the state file is gone: name the newest backup, with its age.

    A backup is a *snapshot*, not the live block, so the message says when it was taken and tells the
    reader to check it - restoring a stale block silently would be worse than the missing file.
    """
    newest = newest_backup(state)
    if not newest:
        return ("refusing: no state file at %s - nothing to push back, and no backup of it either"
                % state)
    taken = datetime.datetime.fromtimestamp(os.path.getmtime(newest)).isoformat(timespec="seconds")
    return ("refusing: no state file at %s - nothing to push back.  The newest backup is %s (%s); "
            "restore it with `python tools/agents/localonly.py --state %s push` - but read it first, "
            "a backup is a snapshot and may predate your latest edits to the section."
            % (state, newest, taken, newest))


def find_block(text):
    """(start, end, end_trimmed) of the section, or None when the file has none.

    Matched on whole lines, not on substrings: rule 8 quotes both markers as text further down the
    file, so a substring search finds four of them.  The real markers are the only lines that *are*
    the marker.
    """
    lines = text.split("\n")
    begins = [i for i, line in enumerate(lines) if line.rstrip("\r") == BEGIN]
    ends = [i for i, line in enumerate(lines) if line.rstrip("\r") == END]
    if not begins and not ends:
        return None
    if len(begins) != 1 or len(ends) != 1:
        raise SystemExit("refusing: the file does not contain exactly one of each LOCAL-ONLY marker line")
    b, e = begins[0], ends[0]
    if e < b:
        raise SystemExit("refusing: the END marker comes before the BEGIN marker")
    start = sum(len(line) + 1 for line in lines[:b])
    end = sum(len(line) + 1 for line in lines[:e]) + len(lines[e])
    k = end
    while k < len(text) and text[k] in "\r\n":
        k += 1
    return start, end, k


def head_version(file):
    out = subprocess.run(["git", "show", "HEAD:%s" % file], cwd=REPO, capture_output=True,
                         text=True, encoding="utf-8", errors="replace")
    return out.stdout if out.returncode == 0 else None


def notice_framing(file, stripped):
    """A pull should leave the file byte-identical to the committed revision.

    When it does not, the section's framing is not canonical - a doubled blank line where the section
    sits is the usual cause - and that whitespace difference otherwise reaches the next commit as a
    mystery hunk next to the real edit.  Say so here, where the cause is visible, rather than leaving it
    to be discovered in a review.
    """
    head = head_version(file)
    if head is None:
        return
    if stripped.replace("\r\n", "\n") == head.replace("\r\n", "\n"):
        print("the pulled file matches HEAD - removing the section leaves no trace")
        return
    print("note: the pulled file differs from HEAD, so a commit now carries more than your staged edit:")
    diff = list(difflib.unified_diff(head.splitlines(), stripped.splitlines(), "HEAD", "pulled", n=0))
    for line in diff[:12]:
        print("   " + line)
    if len(diff) > 12:
        print("   ... (%d more diff lines)" % (len(diff) - 12))
    print("   a doubled blank line around the section is the usual cause - fix the framing, then re-pull")


def cmd_pull(a):
    path = os.path.join(REPO, a.file)
    text = read(path)
    found = find_block(text)
    if not found:
        raise SystemExit("%s has no LOCAL-ONLY section - nothing to pull" % a.file)
    if os.path.exists(a.state) and not a.force:
        raise SystemExit("refusing: %s already holds a pulled section (use --force to overwrite)" % a.state)
    i, _j, k = found
    state = {
        "file": a.file,
        "block": text[i:k],
        "before": text[max(0, i - ANCHOR):i],
        "after": text[k:k + ANCHOR],
        "sha1_before_pull": sha1(text),
        "pulled_at": datetime.datetime.now().isoformat(timespec="seconds"),
    }
    if a.dry_run:
        print("would pull %d lines (%d bytes) out of %s" % (state["block"].count("\n"), len(state["block"]), a.file))
        return 0
    os.makedirs(os.path.dirname(a.state), exist_ok=True)
    with open(a.state, "w", encoding="utf-8") as fh:
        json.dump(state, fh, indent=1)
    backup = backup_path(a.state)
    shutil.copyfile(a.state, backup)
    pruned = prune_backups(a.state)
    write(path, text[:i] + text[k:])
    notice_framing(a.file, text[:i] + text[k:])
    print("pulled %d lines (%d bytes) out of %s" % (state["block"].count("\n"), len(state["block"]), a.file))
    print("stored in %s" % os.path.relpath(a.state, REPO))
    print("backed up to %s%s" % (os.path.relpath(backup, REPO),
                                  " (%d older backup(s) pruned)" % len(pruned) if pruned else ""))
    print("now: git add %s && git commit ... && %s push" % (a.file, os.path.basename(__file__)))
    return 0


def cmd_push(a):
    path = os.path.join(REPO, a.file)
    if not os.path.exists(a.state):
        raise SystemExit(no_state_message(a.state))
    with open(a.state, "r", encoding="utf-8") as fh:
        state = json.load(fh)
    text = read(path)
    if find_block(text):
        raise SystemExit("refusing: %s already contains a LOCAL-ONLY section" % a.file)
    anchor = state["before"]
    if text.count(anchor) != 1:
        raise SystemExit("refusing: the anchor before the section matches %d times - the file changed too "
                         "much since the pull; insert the stored block by hand" % text.count(anchor))
    at = text.index(anchor) + len(anchor)
    if state["after"] and not text[at:].startswith(state["after"]):
        raise SystemExit("refusing: the text after the cut does not match the state file - the file changed "
                         "since the pull; insert the stored block by hand")
    if a.dry_run:
        print("would push %d lines (%d bytes) back into %s" % (state["block"].count("\n"), len(state["block"]), a.file))
        return 0
    write(path, text[:at] + state["block"] + text[at:])
    now = sha1(read(path))
    same = "identical to the pre-pull file" if now == state["sha1_before_pull"] else \
           "the rest of the file changed while pulled (expected if you kept editing it)"
    print("pushed %d lines back into %s - %s" % (state["block"].count("\n"), a.file, same))
    os.remove(a.state)
    print("removed %s" % os.path.relpath(a.state, REPO))
    return 0


def cmd_status(a):
    path = os.path.join(REPO, a.file)
    text = read(path)
    found = find_block(text)
    if found:
        print("%s: LOCAL-ONLY section present (%d lines between the markers)"
              % (a.file, text[found[0]:found[1]].count("\n")))
    else:
        print("%s: no LOCAL-ONLY section" % a.file)
    if os.path.exists(a.state):
        with open(a.state, "r", encoding="utf-8") as fh:
            state = json.load(fh)
        print("%s: holds a pulled section (%d lines, pulled %s)"
              % (os.path.relpath(a.state, REPO), state["block"].count("\n"), state.get("pulled_at", "?")))
    else:
        print("%s: nothing pulled" % os.path.relpath(a.state, REPO))
    backups = list_backups(a.state)
    if backups:
        taken = datetime.datetime.fromtimestamp(os.path.getmtime(backups[-1])).isoformat(timespec="seconds")
        print("%d backup(s) of the state file, newest %s (%s) - rule 8's only other copy of the section"
              % (len(backups), os.path.relpath(backups[-1], REPO), taken))
    else:
        print("no backup of the state file - a pull will write one")
    return 0


def cmd_verify(a):
    out = subprocess.run(["git", "show", "%s:%s" % (a.rev, a.file)], cwd=REPO,
                         capture_output=True, text=True, encoding="utf-8", errors="replace")
    if out.returncode != 0:
        raise SystemExit("git show %s:%s failed: %s" % (a.rev, a.file, out.stderr.strip()[:200]))
    count = sum(1 for line in out.stdout.splitlines() if line.startswith("<!-- LOCAL-ONLY"))
    if count:
        print("%s:%s still contains %d LOCAL-ONLY marker line(s) - rule 8 violated" % (a.rev, a.file, count))
        return 1
    print("%s:%s contains no LOCAL-ONLY marker line - rule 8 satisfied" % (a.rev, a.file))
    return 0


def selftest() -> int:
    """Check the backup helpers on a temp directory.

    Only the pure helpers are exercised: `pull`/`push` are bound to the real repository, and running
    either here would strip the live section out of AGENTS.md.  The runner's tree-dirty guard is the
    backstop for that, not this test.
    """
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    t0 = datetime.datetime(2026, 9, 27, 3, 4, 5)
    with tempfile.TemporaryDirectory() as tmp:
        state = os.path.join(tmp, "local-only.state.json")
        check("no backups yet", list_backups(state), [])
        check("no newest backup", newest_backup(state), None)
        check("pruning with no backups is safe", prune_backups(state), [])
        check("the backup name is the state path plus a sortable stamp",
              os.path.basename(backup_path(state, t0)), "local-only.state.json.bak-20260927-030405")
        for i in range(7):
            open(backup_path(state, t0 + datetime.timedelta(seconds=i)), "w").write(str(i))
        check("every backup is listed", len(list_backups(state)), 7)
        check("the list is oldest first", os.path.basename(list_backups(state)[0]),
              "local-only.state.json.bak-20260927-030405")
        check("the newest backup is the latest stamp", os.path.basename(newest_backup(state)),
              "local-only.state.json.bak-20260927-030411")
        removed = prune_backups(state, keep=3)
        check("pruning keeps the newest N", len(list_backups(state)), 3)
        check("... and removes the oldest first", [os.path.basename(p) for p in removed],
              ["local-only.state.json.bak-20260927-03040%d" % i for i in range(5, 9)])
        check("... so the survivor is still the newest", os.path.basename(newest_backup(state)),
              "local-only.state.json.bak-20260927-030411")
        check("the state file itself is never listed as a backup",
              open(state, "w").write("x") and len(list_backups(state)), 3)
        check("prune keeps everything when nothing is over the limit", prune_backups(state, keep=9), [])
        message = no_state_message(state)
        check("the refusal names the newest backup", os.path.basename(newest_backup(state)) in message, True)
        check("... and how to restore it", "--state" in message and "push" in message, True)
        check("... and warns it is a snapshot", "snapshot" in message, True)
    with tempfile.TemporaryDirectory() as tmp:
        check("no backup at all is said plainly",
              "no backup" in no_state_message(os.path.join(tmp, "gone.json")), True)
    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--file", default=DEFAULT_FILE, help="file to edit (default %s)" % DEFAULT_FILE)
    ap.add_argument("--state", default=os.path.join(REPO, DEFAULT_STATE),
                    help="state file (default %s)" % DEFAULT_STATE)
    ap.add_argument("--selftest", action="store_true", help="check this tool's own helpers")
    sub = ap.add_subparsers(dest="cmd")

    p = sub.add_parser("pull", help="remove the section and store it")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--force", action="store_true", help="overwrite an existing state file")
    p.set_defaults(func=cmd_pull)

    p = sub.add_parser("push", help="restore the stored section")
    p.add_argument("--dry-run", action="store_true")
    p.set_defaults(func=cmd_push)

    p = sub.add_parser("status", help="what is in the file and in the state file")
    p.set_defaults(func=cmd_status)

    p = sub.add_parser("verify", help="rule 8 check against a committed revision")
    p.add_argument("--rev", default="HEAD")
    p.set_defaults(func=cmd_verify)

    a = ap.parse_args()
    if a.selftest:
        sys.exit(selftest())
    if not a.cmd:
        ap.print_help()
        sys.exit(0)
    sys.exit(a.func(a) or 0)


if __name__ == "__main__":
    main()
