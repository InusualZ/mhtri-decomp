#!/usr/bin/env python3
"""worktreehook.py - Claude Code's WorktreeCreate / WorktreeRemove hooks, backed by the slot pool.

PROTOTYPE. An Agent-tool subagent launched with `isolation: "worktree"` gets its working directory from the
`WorktreeCreate` hook, so the hook can hand it a **slot** instead of a throwaway git worktree - which is how an
in-session subagent gets the slot isolation the headless launcher (`lanecmd.py`) gets from `cd <slot>`.

**Scoped by arming.** A project hook fires for every worktree Claude Code creates here, and most of those are
not campaign lanes (a manual `--worktree`, an ad hoc isolated subagent).  So the hook hands out a slot only
against an **arm token**: the orchestrator runs `worktreehook.py arm N` right before launching N lanes, each
`create` consumes one token (an atomic rename, so parallel hooks cannot share one) and tokens expire after
`--ttl` seconds so a forgotten arm cannot capture a later launch.  With no live token the hook does what Claude
Code would have done itself - a plain git worktree under `.claude/worktrees/<name>` - and `remove` cleans that
kind up the same way.

**A token can name a slot** (`arm --slot 3`): the orchestrator has already claimed that slot for a unit
(`queue.py next` acquires one and cuts the claim's branch), and the token binds the launch to it - `create`
hands that very slot over instead of acquiring another, so the claim, its branch, its brief and the lane share
one directory with no adoption step.  A token with no slot means "any current free slot".

  create  reads the hook JSON on stdin.  Armed: takes the first free slot whose build tree is already
          **current**, cuts a placeholder branch `worker/hook-<stamp>-<pid>` off main's tip and prints the slot path
          (the only thing stdout may carry).  Slot choice is `slots.acquire`'s own atomic `.used` sentinel, so
          parallel hooks get distinct slots.  It never re-seeds a stale slot (a hook has a timeout): with no
          current free slot it fails naming the fix, and gives the token back.
  remove  a slot path is released - unless `slots.unlanded_reason` says it holds unlanded work, in which case it
          exits 1 so Claude Code keeps the directory and the orchestrator runs `slots.py collect --release`.  A
          plain worktree under `.claude/worktrees` is removed with `git worktree remove` (which refuses a dirty
          one).  Anything else is not ours: exit 0.

Every call appends the raw input and the decision to `<main>/.pi/lanes/hook.log`.  Measured input fields:
`cwd`, `hook_event_name`, `name` (`agent-<agentId>`), `prompt_id`, `scratchpad_dir`, `session_id`,
`transcript_path` (create) and `worktree_path` (remove).

**The payload cannot identify the lane** (measured in hook.log, 2026-09-29): `name` is `agent-<agentId>`, an id
the harness generates at launch, so the orchestrator cannot know it when it arms; `prompt_id` is shared by every
Agent call issued in one assistant message (three parallel launches logged one id); there is no description,
prompt text, branch or requested-name field.  Nothing the launcher controls reaches the hook, so tokens cannot be
keyed by lane and parallel launches take tokens in claim order (a lane briefed for slot 4 got slot 3).
**Rule: a slot-bound token is armed and launched ONE AT A TIME** - `arm` refuses a second bound token, and any
mix of bound and unbound tokens; unbound tokens (any free slot) may be armed N at a time.  After each launch
returns (its create hook has consumed the token) arm the next.

  python tools/units/worktreehook.py arm N [--ttl SECONDS] | arm --slot N | disarm | status
  python tools/units/worktreehook.py create|remove        # stdin: the hook JSON
  python tools/units/worktreehook.py --selftest
"""
from __future__ import annotations

import argparse
import contextlib
import json
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from units import slots  # noqa: E402

#: How long an arm token lives.  Long enough to launch a batch, short enough that a forgotten arm expires.
TOKEN_TTL = 900


def _main_root() -> str:
    return slots._main_root()


def log(main: str, event: str, **fields) -> None:
    os.makedirs(os.path.join(main, ".pi", "lanes"), exist_ok=True)
    row = {"t": time.strftime("%Y-%m-%d %H:%M:%S"), "pid": os.getpid(), "event": event, **fields}
    with open(os.path.join(main, ".pi", "lanes", "hook.log"), "a", encoding="utf-8", newline="\n") as fh:
        fh.write(json.dumps(row, sort_keys=True) + "\n")


def read_input() -> dict:
    try:
        data = json.load(sys.stdin)
    except ValueError:
        return {}
    return data if isinstance(data, dict) else {}


# --- arming ----------------------------------------------------------------------------------------

def armed_dir(main: str) -> str:
    return os.path.join(main, ".pi", "lanes", "armed")


def _tokens(main: str) -> list[str]:
    d = armed_dir(main)
    try:
        return sorted(n for n in os.listdir(d) if n.endswith(".token"))
    except OSError:
        return []


def _live_slot_bound(main: str) -> int:
    n = 0
    for name in _tokens(main):
        path = os.path.join(armed_dir(main), name)
        if not _expired(path) and _token_slot(path) is not None:
            n += 1
    return n


def arm(main: str, count: int = 0, ttl: int = TOKEN_TTL, slot_numbers: list[int] | None = None) -> list[str]:
    """Write tokens that each authorise one slot hand-out for the next `ttl` seconds.

    `count` tokens name no slot (any current free one); each number in `slot_numbers` is a token bound to
    that slot.  A slot-bound token must be the ONLY live token (SystemExit otherwise): the hook payload
    carries nothing the launcher controls (module docstring), so a token cannot be matched to its lane and
    parallel launches take tokens in claim order.  Bound launches are therefore sequential - arm one slot,
    launch that one lane, arm the next.
    """
    bound = list(slot_numbers or [])
    if bound:
        live = armed_count(main)
        if len(bound) > 1:
            raise SystemExit("worktreehook: refusing to arm %d slot-bound tokens at once (slots %s): the hook "
                             "payload cannot tell the launches apart, so the lanes would receive slots in "
                             "token-claim order, not launch order. Arm ONE slot, launch that lane, then arm "
                             "the next." % (len(bound), ", ".join(map(str, bound))))
        if count or live:
            raise SystemExit("worktreehook: a slot-bound token must be the only live token (%d unbound "
                             "requested, %d already live): another launch could consume it. Launch or "
                             "`disarm` first." % (count, live))
    elif count and _live_slot_bound(main):
        raise SystemExit("worktreehook: a slot-bound token is live; an unbound launch could consume it and "
                         "take the wrong slot. Launch that lane or `disarm` first.")
    return _write_tokens(main, count, ttl, bound)


def _write_tokens(main: str, count: int, ttl: int, slot_numbers: list[int]) -> list[str]:
    os.makedirs(armed_dir(main), exist_ok=True)
    stamp = int(time.time() * 1000)
    names = []
    for i, slot in enumerate([None] * count + list(slot_numbers or [])):
        name = "%d-%02d-%d.token" % (stamp, i, os.getpid())
        with open(os.path.join(armed_dir(main), name), "w", encoding="utf-8") as fh:
            json.dump({"expires": time.time() + ttl, "slot": slot}, fh)
        names.append(name)
    return names


def _expired(path: str) -> bool:
    try:
        with open(path, encoding="utf-8") as fh:
            return float(json.load(fh).get("expires", 0)) < time.time()
    except (OSError, ValueError, AttributeError, TypeError):
        return True                      # an unreadable token authorises nothing


def armed_count(main: str) -> int:
    """Live tokens (expired ones are not counted, and not removed - `take_token` sweeps them)."""
    return sum(1 for n in _tokens(main) if not _expired(os.path.join(armed_dir(main), n)))


def disarm(main: str) -> int:
    """Remove every token; the number removed."""
    n = 0
    for name in _tokens(main):
        try:
            os.remove(os.path.join(armed_dir(main), name))
            n += 1
        except OSError:
            pass
    return n


def _token_slot(path: str) -> int | None:
    try:
        with open(path, encoding="utf-8") as fh:
            slot = json.load(fh).get("slot")
    except (OSError, ValueError, AttributeError):
        return None
    return slot if isinstance(slot, int) and not isinstance(slot, bool) else None


def take_token(main: str) -> dict | None:
    """Consume one live token, atomically: `{"name", "slot"}` (slot is `None` for "any"), or `None`.

    The claim is an exclusive create (`O_EXCL`) of `<token>.claim`: of two hooks racing for the same token
    exactly one create succeeds.  (A rename was tried first and is NOT safe here - on Windows two threads
    both "renamed" one source, 8 winners for 5 tokens.)  The winner re-checks that the token still exists,
    because a hook working from a stale directory listing can create the claim after the token was consumed;
    then it deletes the token and, last, its claim.  Expired tokens are deleted on the way past.
    """
    for name in _tokens(main):
        src = os.path.join(armed_dir(main), name)
        claim = src + ".claim"
        if _expired(src):
            for path in (src, claim):
                try:
                    os.remove(path)
                except OSError:
                    pass
            continue
        try:
            os.close(os.open(claim, os.O_CREAT | os.O_EXCL | os.O_WRONLY))
        except OSError:
            continue
        slot = _token_slot(src)
        won = os.path.exists(src)
        if won:
            try:
                os.remove(src)
            except OSError:
                won = False
        try:
            os.remove(claim)
        except OSError:
            pass
        if won:
            return {"name": name, "slot": slot}
    return None


def give_back(main: str, token: dict, ttl: int = TOKEN_TTL) -> None:
    """Return a token a failed hand-out did not use, so a slot shortage does not silently disarm the batch."""
    bound = token.get("slot") is not None
    _write_tokens(main, 0 if bound else 1, ttl, [token["slot"]] if bound else [])


# --- create ----------------------------------------------------------------------------------------

def create_slot(main: str, data: dict) -> str:
    """The path of a freshly acquired, current slot, or SystemExit naming why there is none."""
    unit = "lane/hook-%s-%d" % (time.strftime("%Y%m%d-%H%M%S"), os.getpid())
    tried = []
    for row in slots.free_slots(main):
        n = row["slot"]
        v = slots.verify(main, n)
        if not v["ok"]:
            tried.append("slot %d is not current (%s)" % (n, "; ".join(v["reasons"])[:120]))
            continue
        try:
            with contextlib.redirect_stdout(sys.stderr):
                info = slots.acquire(main, unit, worker="worktree-hook", slot=n)
        except SystemExit as exc:          # a racing hook took it between `free_slots` and `acquire`
            tried.append("slot %d lost the race (%s)" % (n, str(exc).splitlines()[0][:100]))
            continue
        log(main, "create", slot=n, unit=unit, branch=info["branch"], path=info["dir"], input=data)
        return info["dir"]
    log(main, "create-refused", tried=tried, input=data)
    raise SystemExit("worktreehook: no free slot with a current build tree (%s). Warm one first: "
                     "python tools/units/slots.py acquire lane/warm --slot N, then release it."
                     % ("; ".join(tried) or "the pool is full"))


def plain_root(main: str) -> str:
    return os.path.join(main, ".claude", "worktrees")


def _safe_name(name: str) -> str:
    name = re.sub(r"[^A-Za-z0-9._-]+", "-", name or "").strip("-.")
    return name or "wt-%s-%d" % (time.strftime("%Y%m%d-%H%M%S"), os.getpid())


def create_plain(main: str, data: dict) -> str:
    """What Claude Code would have done itself: a git worktree under `.claude/worktrees/<name>` off HEAD."""
    name = _safe_name(data.get("name") or "")
    path = os.path.join(plain_root(main), name)
    branch = "worktree-%s" % name
    p = subprocess.run(["git", "-C", main, "worktree", "add", "-b", branch, path, "HEAD"],
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        log(main, "create-plain-failed", name=name, error=(p.stderr or "").strip()[:300], input=data)
        raise SystemExit("worktreehook: git worktree add failed: %s" % (p.stderr or "").strip())
    log(main, "create-plain", path=path, branch=branch, input=data)
    return path


def create_bound(main: str, slot: int, data: dict) -> str:
    """Hand over slot `slot`, which the orchestrator has already claimed - never acquire another.

    Refuses (SystemExit) unless the slot is occupied by a claim (its `.used` sentinel) and is on a branch:
    a token naming a free or detached slot means the claim was released or never made.
    """
    d = slots.slot_dir(main, slot)
    branch = slots.slot_attached_branch(d) if os.path.isdir(d) else None
    if not branch or not slots.marker_present(d):
        log(main, "create-bound-refused", slot=slot, branch=branch, input=data)
        raise SystemExit("worktreehook: slot %d is not a claimed slot on a branch (branch=%r) - claim it first "
                         "(queue.py next / slots.py acquire) and arm it again" % (slot, branch))
    log(main, "create-bound", slot=slot, branch=branch, path=d, input=data)
    return d


def create(main: str, data: dict) -> str:
    """The claimed slot a token names, else any current free slot when armed, otherwise a plain worktree."""
    token = take_token(main)
    if token is None:
        return create_plain(main, data)
    try:
        if token["slot"] is not None:
            return create_bound(main, token["slot"], data)
        return create_slot(main, data)
    except SystemExit:
        give_back(main, token)
        raise


# --- remove ----------------------------------------------------------------------------------------

def _under(path: str, root: str) -> bool:
    a, b = os.path.normcase(os.path.abspath(path)), os.path.normcase(os.path.abspath(root))
    return a.startswith(b.rstrip(os.sep) + os.sep)


def remove_plain(main: str, path: str, data: dict) -> int:
    p = subprocess.run(["git", "-C", main, "worktree", "remove", path],
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        log(main, "remove-plain-vetoed", path=path, error=(p.stderr or "").strip()[:300], input=data)
        sys.stderr.write("worktreehook: keeping %s: %s\n" % (path, (p.stderr or "").strip()))
        return 1
    branch = "worktree-%s" % os.path.basename(path.rstrip("/\\"))
    subprocess.run(["git", "-C", main, "branch", "-d", branch], capture_output=True)   # refuses unmerged work
    log(main, "remove-plain", path=path, input=data)
    return 0


def remove(main: str, data: dict) -> int:
    path = data.get("worktree_path") or ""
    n = slots.slot_of_path(main, path)
    if n is None:
        if path and _under(path, plain_root(main)):
            return remove_plain(main, path, data)
        log(main, "remove-not-ours", path=path, input=data)
        return 0
    d = slots.slot_dir(main, n)
    branch = slots.slot_attached_branch(d)
    reason = slots.unlanded_reason(main, n)
    if reason:
        log(main, "remove-vetoed", slot=n, branch=branch, reason=reason, input=data)
        sys.stderr.write("worktreehook: keeping slot %d (%s): %s - `slots.py collect --slot %d --release` "
                         "frees it once it is landed\n" % (n, d, reason, n))
        return 1
    with contextlib.redirect_stdout(sys.stderr):
        slots.release(main, slot=n, unit=None, branch=branch)
    log(main, "remove", slot=n, branch=branch, input=data)
    return 0


# --- selftest --------------------------------------------------------------------------------------

def selftest() -> int:
    import io
    import tempfile
    import threading

    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def quiet(fn, *a, **k):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            try:
                return fn(*a, **k)
            except SystemExit as exc:
                return exc

    def git(cwd, *args):
        subprocess.run(["git", "-C", cwd, *args], capture_output=True, check=True)

    with tempfile.TemporaryDirectory() as tmp:
        main = os.path.join(tmp, "repo")
        os.makedirs(main)
        git(main, "init", "-q", "-b", "main")
        git(main, "config", "user.email", "t@t")
        git(main, "config", "user.name", "t")
        git(main, "config", "core.autocrlf", "false")
        with open(os.path.join(main, ".gitignore"), "w") as fh:
            fh.write(".pi/\n.claude/worktrees/\n")
        with open(os.path.join(main, "f.md"), "w") as fh:
            fh.write("x\n")
        git(main, "add", "-A")
        git(main, "commit", "-qm", "init")

        # --- arming ------------------------------------------------------------------------------
        check("nothing armed to begin with", armed_count(main), 0)
        check("no token to take", take_token(main), None)
        arm(main, 3)
        check("arm(3) makes three live tokens", armed_count(main), 3)
        got = [take_token(main) for _ in range(4)]
        check("three takes succeed and the fourth finds none", [g is not None for g in got], [True, True, True, False])
        check("... each token name was distinct", len({g["name"] for g in got if g}), 3)
        check("... an unbound token names no slot", got[0]["slot"], None)
        check("... and the armed count is back to zero", armed_count(main), 0)
        arm(main, 1, ttl=-1)
        check("an expired token is not live", armed_count(main), 0)
        check("... authorises nothing", take_token(main), None)
        check("... and taking it swept the file", _tokens(main), [])
        arm(main, 2)
        check("disarm removes what is armed", disarm(main), 2)
        check("... leaving nothing", armed_count(main), 0)
        with open(os.path.join(armed_dir(main), "junk.token"), "w") as fh:
            fh.write("not json")
        check("an unreadable token authorises nothing", take_token(main), None)
        # racing hooks: 5 tokens, 12 threads, exactly 5 winners and no name twice
        arm(main, 5)
        wins, lock = [], threading.Lock()

        def racer():
            t = take_token(main)
            if t:
                with lock:
                    wins.append(t["name"])
        threads = [threading.Thread(target=racer) for _ in range(12)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        check("12 racing hooks share 5 tokens: exactly 5 win", len(wins), 5)
        check("... and no token is taken twice", len(set(wins)), 5)
        arm(main, 0, slot_numbers=[4])
        check("a slot-bound token carries its slot", take_token(main)["slot"], 4)
        # parallel bound launches are refused (the payload cannot tell them apart)
        res = quiet(arm, main, 0, TOKEN_TTL, [2, 3, 4])
        check("arming several slot-bound tokens at once is refused", isinstance(res, SystemExit), True)
        check("... naming the reason", "token-claim order" in str(res), True)
        check("... and arms nothing", armed_count(main), 0)
        arm(main, 0, slot_numbers=[2])
        res = quiet(arm, main, 0, TOKEN_TTL, [3])
        check("a second bound token while one is live is refused", isinstance(res, SystemExit), True)
        res = quiet(arm, main, 1)
        check("an unbound token beside a live bound one is refused", isinstance(res, SystemExit), True)
        check("... the bound token is untouched", (armed_count(main), _live_slot_bound(main)), (1, 1))
        disarm(main)
        arm(main, 1)
        res = quiet(arm, main, 0, TOKEN_TTL, [3])
        check("a bound token beside a live unbound one is refused", isinstance(res, SystemExit), True)
        disarm(main)
        arm(main, 2)
        arm(main, 2)
        check("unbound tokens may be armed in batches", armed_count(main), 4)
        disarm(main)
        arm(main, 0, ttl=-1, slot_numbers=[2])
        arm(main, 0, slot_numbers=[3])
        check("an expired bound token does not block the next arm", armed_count(main), 1)
        disarm(main)
        # bound tokens armed sequentially, launches racing on the take: each arm is consumed by exactly one
        # hook, and the taker gets the slot its own arm named
        got_slots, lock2 = [], threading.Lock()
        for slot in (2, 3, 4):
            arm(main, 0, slot_numbers=[slot])
            threads = [threading.Thread(target=lambda: (lambda t: t and (lock2.acquire(), got_slots.append(t["slot"]),
                                                                          lock2.release()))(take_token(main)))
                       for _ in range(4)]
            for t in threads:
                t.start()
            for t in threads:
                t.join()
        check("sequential bound arms under racing hooks hand out each slot exactly once", got_slots, [2, 3, 4])

        # --- plain worktrees (unarmed) -----------------------------------------------------------
        path = quiet(create, main, {"name": "agent-abc123"})
        check("unarmed create makes a plain worktree under .claude/worktrees",
              isinstance(path, str) and _under(path, plain_root(main)) and os.path.isdir(path), True)
        check("... on its own branch", subprocess.run(["git", "-C", path, "branch", "--show-current"],
                                                      capture_output=True, text=True, encoding="utf-8").stdout.strip(),
              "worktree-agent-abc123")
        check("a name with odd characters is made safe", _safe_name("../a b/c"), "a-b-c")
        with open(os.path.join(path, "dirty.md"), "w") as fh:
            fh.write("uncommitted")
        check("remove vetoes a dirty plain worktree (exit 1)", quiet(remove, main, {"worktree_path": path}), 1)
        check("... and keeps the directory", os.path.isdir(path), True)
        os.remove(os.path.join(path, "dirty.md"))
        check("remove deletes a clean plain worktree", quiet(remove, main, {"worktree_path": path}), 0)
        check("... the directory is gone", os.path.exists(path), False)
        check("... and its unmerged-nothing branch with it",
              subprocess.run(["git", "-C", main, "branch", "--list", "worktree-agent-abc123"],
                             capture_output=True, text=True, encoding="utf-8").stdout.strip(), "")
        check("a path that is neither slot nor plain is not ours (exit 0)",
              quiet(remove, main, {"worktree_path": os.path.join(tmp, "elsewhere")}), 0)

        # --- armed: slot routing, against a fake pool ---------------------------------------------
        class FakeSlots:
            def __init__(self):
                self.acquired, self.released, self.reason = [], [], None
                self.dirs = {n: os.path.join(tmp, "slot%d" % n) for n in (1, 2, 3)}

            def free_slots(self, _m):
                return [{"slot": n} for n in (1, 2, 3)]

            def verify(self, _m, n):
                return {"ok": n == 2, "reasons": ["build tree is stale"]}

            def acquire(self, _m, unit, worker=None, slot=None):
                self.acquired.append((unit, slot))
                return {"dir": self.dirs[slot], "branch": "worker/" + unit.split("/")[-1]}

            def slot_of_path(self, _m, path):
                for n, d in self.dirs.items():
                    if os.path.normcase(os.path.abspath(path)) == os.path.normcase(os.path.abspath(d)):
                        return n
                return None

            def slot_dir(self, _m, n):
                return self.dirs[n]

            def slot_attached_branch(self, d):
                return None if d == self.dirs[1] else "worker/x"

            def marker_present(self, d):
                return d != self.dirs[2]

            def unlanded_reason(self, _m, _n):
                return self.reason

            def release(self, _m, slot=None, unit=None, branch=None):
                self.released.append(slot)
                return {}

        fake = FakeSlots()
        real = globals()["slots"]
        globals()["slots"] = fake
        try:
            arm(main, 1)
            path = quiet(create, main, {"name": "agent-1"})
            check("armed create hands out the first CURRENT slot (slot 1 is stale)", path, fake.dirs[2])
            check("... having acquired exactly that slot", [s for _u, s in fake.acquired], [2])
            check("... and consumed the token", armed_count(main), 0)
            check("the next create, unarmed, is a plain worktree",
                  _under(quiet(create, main, {"name": "agent-2"}), plain_root(main)), True)
            quiet(remove, main, {"worktree_path": os.path.join(plain_root(main), "agent-2")})
            for d in fake.dirs.values():
                os.makedirs(d, exist_ok=True)
            arm(main, 0, slot_numbers=[3])
            path = quiet(create, main, {"name": "agent-b"})
            check("a slot-bound token hands over THAT claimed slot", path, fake.dirs[3])
            check("... acquiring nothing", len(fake.acquired), 1)
            arm(main, 0, slot_numbers=[1])
            res = quiet(create, main, {"name": "agent-c"})
            check("a token naming a detached slot is refused", isinstance(res, SystemExit), True)
            check("... and returned, still bound to its slot", (armed_count(main), take_token(main)["slot"]), (1, 1))
            arm(main, 0, slot_numbers=[2])
            res = quiet(create, main, {"name": "agent-d"})
            check("a token naming a slot with no claim marker is refused", isinstance(res, SystemExit), True)
            disarm(main)
            fake.verify = lambda _m, n: {"ok": False, "reasons": ["stale"]}
            arm(main, 1)
            res = quiet(create, main, {"name": "agent-3"})
            check("armed create with no current slot fails", isinstance(res, SystemExit), True)
            check("... naming the fix", "Warm one first" in str(res), True)
            check("... and gives the token back, so the batch is not silently disarmed", armed_count(main), 1)
            disarm(main)
            check("remove releases a slot with nothing unlanded", quiet(remove, main, {"worktree_path": fake.dirs[2]}), 0)
            check("... via slots.release", fake.released, [2])
            fake.reason = "branch worker/x holds commits main does not have"
            check("remove VETOES a slot holding unlanded work (exit 1)",
                  quiet(remove, main, {"worktree_path": fake.dirs[3]}), 1)
            check("... and does not release it", fake.released, [2])
        finally:
            globals()["slots"] = real

        rows = [json.loads(ln) for ln in open(os.path.join(main, ".pi", "lanes", "hook.log"), encoding="utf-8")]
        events = {r["event"] for r in rows}
        check("every decision is logged", {"create", "create-plain", "create-refused", "remove",
                                           "remove-vetoed", "remove-plain", "remove-plain-vetoed",
                                           "remove-not-ours"} <= events, True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    a = sub.add_parser("arm", help="authorise N slot hand-outs (or hand-outs of named claimed slots) for --ttl seconds")
    a.add_argument("count", type=int, nargs="?", default=0, help="any-current-free-slot tokens")
    a.add_argument("--slot", type=int, action="append", default=[],
                   help="a token bound to this already-claimed slot (one at a time: see the module docstring)")
    a.add_argument("--ttl", type=int, default=TOKEN_TTL)
    sub.add_parser("disarm", help="remove every arm token")
    sub.add_parser("status", help="how many hand-outs are armed")
    sub.add_parser("create", help="WorktreeCreate hook (stdin: the hook JSON)")
    sub.add_parser("remove", help="WorktreeRemove hook (stdin: the hook JSON)")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 2
    root = _main_root()
    if args.cmd == "arm":
        if args.count + len(args.slot) < 1:
            sys.stderr.write("arm: give a count and/or --slot N\n")
            return 2
        arm(root, args.count, args.ttl, args.slot)
        print("armed %d hand-out(s)%s, expiring in %ds" % (
            args.count + len(args.slot), " (slots %s)" % ", ".join(map(str, args.slot)) if args.slot else "",
            args.ttl))
        return 0
    if args.cmd == "disarm":
        print("removed %d token(s)" % disarm(root))
        return 0
    if args.cmd == "status":
        print("%d hand-out(s) armed" % armed_count(root))
        return 0
    data = read_input()
    if args.cmd == "create":
        try:
            path = create(root, data)
        except SystemExit as exc:
            sys.stderr.write("%s\n" % exc)
            return 1
        sys.stdout.write(path.replace("\\", "/") + "\n")
        return 0
    return remove(root, data)


if __name__ == "__main__":
    sys.exit(main())
