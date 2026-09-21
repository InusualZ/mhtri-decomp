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
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BEGIN = "<!-- LOCAL-ONLY-BEGIN: stripped before every commit, see Non-negotiables rule 8 -->"
END = "<!-- LOCAL-ONLY-END -->"
DEFAULT_FILE = "AGENTS.md"
DEFAULT_STATE = ".pi/local-only.state.json"
ANCHOR = 240


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
    write(path, text[:i] + text[k:])
    notice_framing(a.file, text[:i] + text[k:])
    print("pulled %d lines (%d bytes) out of %s" % (state["block"].count("\n"), len(state["block"]), a.file))
    print("stored in %s" % os.path.relpath(a.state, REPO))
    print("now: git add %s && git commit ... && %s push" % (a.file, os.path.basename(__file__)))
    return 0


def cmd_push(a):
    path = os.path.join(REPO, a.file)
    if not os.path.exists(a.state):
        raise SystemExit("refusing: no state file at %s - nothing to push back" % a.state)
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


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--file", default=DEFAULT_FILE, help="file to edit (default %s)" % DEFAULT_FILE)
    ap.add_argument("--state", default=os.path.join(REPO, DEFAULT_STATE),
                    help="state file (default %s)" % DEFAULT_STATE)
    sub = ap.add_subparsers(dest="cmd", required=True)

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
    sys.exit(a.func(a) or 0)


if __name__ == "__main__":
    main()
