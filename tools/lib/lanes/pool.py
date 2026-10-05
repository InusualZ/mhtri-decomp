"""The slot pool: N reusable lane directories at stable paths (`<repo>.slot<n>`) - the manifest, the per-slot
lock record, the `.used` sentinel, each slot's state, and the fail-closed acquire / release / reclaim / verify.
A slot holds a directory, never a branch. Spec: docs/tools/spec/lib-lanes.md. CLI: none (`tools/units/slots.py`)."""
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import time

from tools.lib.git import Git
from tools.lib.lanes import naming, registry, seed, sessions

DEFAULT_COUNT = 6
SLOT_SUFFIX = ".slot"
LOCK_SUBDIR = ("slots",)
#: What a reset keeps; everything else untracked/ignored is scratch (`clean_slot`).
SLOT_KEEP = (".ninja_deps", ".ninja_log", "build.ninja", "objdiff.json", "compile_commands.json",
             "build", "orig", os.path.join("tools", "m2c"), ".used")
SLOT_MARKER = ".used"
#: The ninja-invocation seam: None means `subprocess.run`; a test swaps it to make the currency check hermetic.
NINJA_RUNNER = None


def git(args: list[str], cwd: str, check: bool = True) -> str:
    p = Git(cwd).run(*args)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, p.stderr.strip()))
    return (p.stdout or "").strip()


# --- the sentinel -------------------------------------------------------------------------------------------------

def marker_path(slot: str) -> str:
    return os.path.join(slot, SLOT_MARKER)


def marker_present(slot: str) -> bool:
    return os.path.exists(marker_path(slot))


def marker_text(owner: str, unit: str = "", branch: str = "") -> str:
    """The sentinel's body: `key=value` lines, the owner label first."""
    return ("owner=%s\nunit=%s\nbranch=%s\npid=%d\nat=%s\n"
            % (owner, unit, branch, os.getpid(), time.strftime("%Y-%m-%dT%H:%M:%S")))


def mark_used(slot: str, unit: str = "", branch: str = "", owner: str = "") -> bool:
    """Create the `.used` sentinel atomically (`O_EXCL`) -> False when one exists (a racing acquire lost)."""
    path = marker_path(slot)
    try:
        fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o644)
    except OSError:
        return False
    try:
        os.write(fd, marker_text(owner, unit, branch).encode("utf-8", "replace"))
    except OSError:
        pass
    finally:
        os.close(fd)
    return True


def marker_info(slot: str) -> dict:
    """The sentinel's fields, `{}` when absent; `legacy=True` for a body without `owner=` (the old positional
    `<pid> <unit> <branch>` line is parsed best-effort)."""
    try:
        with open(marker_path(slot), encoding="utf-8", errors="replace") as fh:
            text = fh.read()
    except OSError:
        return {}
    out: dict = {}
    for line in text.splitlines():
        key, sep, value = line.partition("=")
        if sep and key.strip():
            out[key.strip()] = value.strip()
    if "owner" in out:
        out["legacy"] = False
        return out
    parts = text.split()
    if len(parts) >= 3 and os.path.isdir(slot):
        out.update({"pid": parts[0], "unit": parts[1], "branch": parts[2]})
    out["legacy"] = True
    return out


def marker_owner(slot: str) -> str:
    return marker_info(slot).get("owner") or ""


def owner_label(unit: str, branch: str | None, worker: str | None = None) -> str:
    """The label a claim records in its sentinel: the worker, else the branch slug, else the unit."""
    return worker or (naming.slug_of_branch(branch) if branch else "") or unit


def clear_marker(slot: str) -> bool:
    try:
        os.remove(marker_path(slot))
        return True
    except OSError:
        return False


# --- the manifest and the lock records ------------------------------------------------------------------------------

def slot_dir(main: str, n: int) -> str:
    """A slot's stable path: the sibling `<repo>.slot<n>` of MAIN."""
    main = os.path.abspath(main)
    return os.path.join(os.path.dirname(main), "%s%s%d" % (os.path.basename(main), SLOT_SUFFIX, n))


def locks_dir(main: str) -> str:
    return os.path.join(main, ".pi", *LOCK_SUBDIR)


def lock_path(main: str, n: int) -> str:
    return os.path.join(locks_dir(main), "%d.json" % n)


def pool_manifest(main: str) -> dict:
    """`.pi/slots/pool.json` (`count`, `created_at`), or `{}`."""
    try:
        with open(os.path.join(locks_dir(main), "pool.json"), encoding="utf-8") as fh:
            return json.loads(fh.read())
    except (OSError, json.JSONDecodeError):
        return {}


def slot_count(main: str) -> int:
    """How many slot directories exist (the presence reading; 0 = not initialised)."""
    count = pool_manifest(main).get("count")
    if not isinstance(count, int) or count < 1:
        count = DEFAULT_COUNT
    n = 0
    for i in range(1, count + 1):
        d = slot_dir(main, i)
        if os.path.isdir(d) and os.path.exists(os.path.join(d, ".git")):
            n += 1
    return n


def pool_size(main: str) -> int:
    """The cap: `pool.json`'s `count` (a broken slot is still a slot); the directory count without a manifest."""
    count = pool_manifest(main).get("count")
    if isinstance(count, int) and count >= 1:
        return count
    return slot_count(main)


def enabled(main: str) -> bool:
    return slot_count(main) > 0


def read_lock(main: str, n: int) -> dict:
    try:
        with open(lock_path(main, n), encoding="utf-8") as fh:
            return json.loads(fh.read())
    except (OSError, json.JSONDecodeError):
        return {}


def write_lock(main: str, n: int, data: dict) -> None:
    path = lock_path(main, n)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp%d" % os.getpid()
    with open(tmp, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)
    os.replace(tmp, path)


def clear_lock(main: str, n: int) -> None:
    try:
        os.remove(lock_path(main, n))
    except OSError:
        pass


# --- a slot's git state ---------------------------------------------------------------------------------------------

def slot_attached_branch(slot: str) -> str | None:
    p = subprocess.run(["git", "-C", slot, "symbolic-ref", "-q", "HEAD"], capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    ref = (p.stdout or "").strip()
    if p.returncode == 0 and ref.startswith("refs/heads/"):
        return ref[len("refs/heads/"):]
    return None


def slot_head(slot: str) -> str | None:
    p = subprocess.run(["git", "-C", slot, "rev-parse", "HEAD"], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return (p.stdout or "").strip() if p.returncode == 0 else None


def slot_dirty(slot: str) -> list[str]:
    """The rows a release would destroy (`git status --porcelain -uall`, read raw: stripping eats a row's
    leading status space)."""
    p = subprocess.run(["git", "-C", slot, "status", "--porcelain", "-uall"], capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("git status failed in %s: %s" % (slot, (p.stderr or "").strip()))
    return [row.rstrip() for row in (p.stdout or "").splitlines() if row.strip()]


def orphaned_commits(slot: str) -> list[str]:
    """Commits on the slot's HEAD that no branch or remote-tracking ref reaches."""
    head = slot_head(slot)
    if not head:
        return []
    return [ln.strip() for ln in git(["rev-list", head, "--not", "--branches", "--remotes"], slot).splitlines()
            if ln.strip()]


def refs_containing(slot: str) -> list[str]:
    head = slot_head(slot)
    if not head:
        return []
    return [ln.strip() for ln in git(["for-each-ref", "--contains", head, "--format=%(refname)"], slot).splitlines()
            if ln.strip()]


def release_blockers(main: str, n: int, d: str, allow_dirty: bool = False, registry: str | None = None,
                     config: str | None = None) -> list[tuple[str, str]]:
    """`[(summary, detail)]` that make releasing slot `n` unsafe: a live session in it, a dirty tree (unless
    `allow_dirty`), commits no branch reaches. `[]` when safe. `registry` is the session registry dir."""
    out: list[tuple[str, str]] = []
    runs = sessions.runs_in(d, registry, config)
    if runs:
        out.append(("a RUNNING Claude session is still working in it: "
                    + ", ".join(sessions.run_label(r) for r in runs),
                    "cwd %s - the session registry under the Claude config dir is the only live-lane signal there is, "
                    "because a lock cannot see a lane" % d))
    if not allow_dirty:
        dirty = slot_dirty(d)
        if dirty:
            out.append(("the slot's tree is dirty (%d path(s), e.g. %s)" % (len(dirty), dirty[0].strip()),
                        "a release would discard:\n      " + "\n      ".join(dirty[:8])
                        + ("\n      (+%d more)" % (len(dirty) - 8) if len(dirty) > 8 else "")))
    orphans = orphaned_commits(d)
    if orphans:
        refs = refs_containing(d)
        out.append(("HEAD holds %d commit(s) no branch reaches (%s)" % (len(orphans), ", ".join(c[:8] for c in orphans[:4])),
                    "a release would detach and orphan them"
                    + ("; they do live in %s" % ", ".join(refs[:4]) if refs else "")))
    return out


def release_refusal(main: str, n: int, d: str, blockers: list[tuple[str, str]]) -> str:
    """Every reason on the first line, the detail under it, then the `--force` line."""
    lines = ["REFUSED release slot %d (%s): %s" % (n, d, "; ".join(s for s, _ in blockers)),
             "  releasing would detach the slot and delete its branch while that is still live:"]
    for summary, detail in blockers:
        lines.append("  - %s: %s" % (summary, detail))
    lines.append("  if that is what you mean, say so deliberately: "
                 "python tools/units/slots.py release --slot %d --force" % n)
    return "\n".join(lines)


def marker_claim_conflict(slot: str, owner: str, n: int | None = None) -> str | None:
    """The refusal when the sentinel names an owner other than `owner`; None for no marker, an ownerless
    (legacy/crashed) marker, or this owner's own."""
    found = marker_info(slot)
    if not found or not found.get("owner") or found.get("owner") == owner:
        return None
    how = ("--slot %d --force" % n) if n is not None else "--force"
    return ("REFUSED slot %s: its `.used` sentinel belongs to another claim - owner %r (unit %r, branch "
            "%r).\n  one lane per slot: reusing a slot under a live owner is how a lane's HEAD got "
            "detached mid-run.\n  if that owner is gone, say so deliberately: `python tools/units/slots.py "
            "acquire <unit> %s` (or release that slot first)"
            % (os.path.basename(slot), found.get("owner"), found.get("unit") or "?",
               found.get("branch") or "?", how))


def lock_stale(main: str, n: int, lock: dict | None = None) -> bool:
    """A lock is stale when it names no branch, its branch is gone, or the slot has left that branch."""
    lock = read_lock(main, n) if lock is None else lock
    if not lock:
        return False
    branch = lock.get("branch")
    if not branch or not registry.branch_exists(main, branch):
        return True
    return slot_attached_branch(slot_dir(main, n)) != branch


def slot_of_path(main: str, path: str | None) -> int | None:
    """The slot number `path` resolves to, or None."""
    if not path:
        return None
    want = os.path.normcase(os.path.realpath(path))
    for n in range(1, pool_size(main) + 1):
        if os.path.normcase(os.path.realpath(slot_dir(main, n))) == want:
            return n
    return None


def lock_for_path(main: str, path: str | None) -> dict:
    """The lock record of the slot `path` is, `{}` when it is no slot or holds no lock (a spawned lane's claim:
    its unit, branch, kind and the unit set it holds)."""
    n = slot_of_path(main, path)
    return read_lock(main, n) if n is not None else {}


def registry_claims_by_slot(main: str) -> tuple[dict, bool]:
    """`(claim records keyed by slot number, readable)` from `.pi/claims.json`; a legacy row with only a
    `worktree` is matched by resolving that path to a slot."""
    data, ok = registry.read(main)
    if not ok:
        return {}, False
    by_slot: dict = {}
    for unit, rec in data.items():
        if not isinstance(rec, dict):
            continue
        n = rec.get("slot")
        if not isinstance(n, int):
            n = slot_of_path(main, rec.get("worktree"))
        if n is None or n in by_slot:
            continue
        by_slot[n] = dict(rec, unit=rec.get("unit") or unit)
    return by_slot, True


def slot_state(main: str, n: int, runs: list | None = None, claims_by_slot: dict | None = None) -> dict:
    """One slot's row: exists / attached branch / lock / sentinel / claim / live run, the one-word `state`
    (`free`, `no worktree`, `LIVE`, `claimed`, `branch`, `debris`) and the actionable `why`."""
    d = slot_dir(main, n)
    exists = os.path.isdir(d) and os.path.exists(os.path.join(d, ".git"))
    if claims_by_slot is None:
        claims_by_slot, registry_ok = registry_claims_by_slot(main)
    else:
        registry_ok = True
    claim = claims_by_slot.get(n)
    lock = read_lock(main, n)
    stale = lock_stale(main, n, lock) if lock else False
    live_lock = bool(lock and not stale)
    attached = slot_attached_branch(d) if exists else None
    marked = marker_present(d) if exists else False
    marker = marker_info(d) if marked else {}
    live_claim = bool(claim) or live_lock
    marker_stale = marked and not attached and not live_claim
    used = bool(attached) or bool(claim) or live_lock or (marked and not marker_stale)
    free = exists and not used
    run = None
    if exists:
        found = sessions.runs_in(d) if runs is None else sessions.runs_in(d, runs=runs)
        run = found[0] if found else None
    owner = marker.get("owner") or ((claim or {}).get("worker") if claim else "") \
        or (lock.get("worker") if lock else "") or ""
    if not exists:
        state = "no worktree"
        why = ("no worktree at %s (its directory or `.git` is missing) - re-create it (`slots.py init`) or "
               "use another slot" % d)
    elif run:
        state, why = "LIVE", "in use by a RUNNING Claude session %s" % sessions.run_label(run)
    elif claim:
        state = "claimed"
        why = "in use by a live claim: %s (%s, branch %s)" % (
            claim.get("unit") or "?", claim.get("worker") or "?", claim.get("branch") or "?")
    elif live_lock:
        state, why = "claimed", "in use by %s (%s)" % (lock.get("unit") or "?", lock.get("worker") or "?")
    elif attached:
        state = "branch"
        why = "in use - holds branch %s (its `.used` marker is %s%s); release it first, or reclaim it if it " \
              "is already landed: slots.py reclaim --slot %d" % (
                  attached, "present" if marked else "MISSING", ", owner %s" % owner if owner else "", n)
    elif marked and not marker_stale:
        state = "claimed"
        why = "used (`.used` marker present, worktree detached%s)" % (", owner %s" % owner if owner else "")
    elif marker_stale:
        state = "debris"
        why = ("debris: a `.used` marker on a detached worktree with no live claim - reclaimable%s"
               % ("; it names owner %s" % owner if owner else ""))
    elif lock and stale:
        state, why = "free", "stale lock (%s) - reclaimable" % (lock.get("unit") or "?")
    else:
        state, why = "free", "free"
    record_note = ""
    if exists and (attached or marked) and not lock:
        record_note = "missing record (no `.pi/slots/%d.json`) - reclaim --slot %d if it is landed, else release" \
                      % (n, n)
    elif exists and lock and stale:
        record_note = ("stale record: it names branch %s (base %s) but the slot holds %s - `acquire --slot %d` "
                       "resets it" % (lock.get("branch") or "?", (lock.get("base") or "?")[:9],
                                       attached or "nothing", n))
    if record_note and "free" not in why:
        why = "%s; %s" % (why, record_note)
    return {"slot": n, "dir": d, "exists": exists, "attached": attached, "lock": lock, "stale": stale,
            "marked": marked, "marker": marker, "owner": owner, "marker_stale": marker_stale,
            "claim": claim, "live_claim": live_claim, "registry_ok": registry_ok, "state": state,
            "record_note": record_note, "used": used, "free": free, "run": run, "why": why}


def all_slots(main: str, runs: list | None = None, claims_by_slot: dict | None = None) -> list[dict]:
    """Every slot in the manifest (each registry read once)."""
    live = sessions.live_runs() if runs is None else runs
    if claims_by_slot is None:
        claims_by_slot, _ok = registry_claims_by_slot(main)
    return [slot_state(main, n, runs=live, claims_by_slot=claims_by_slot) for n in range(1, pool_size(main) + 1)]


def free_slots(main: str) -> list[dict]:
    return [s for s in all_slots(main) if s["free"]]


def free_count(main: str) -> int:
    return len(free_slots(main))


def capacity_error(main: str) -> str | None:
    """The refusal when no slot is free (a free slot is the concurrency cap); None with no pool or a free slot."""
    if not enabled(main):
        return None
    if free_slots(main):
        return None
    rows = all_slots(main)
    lines = ["all %d slot(s) are in use - this is the concurrency cap, not a queue shortage." % len(rows),
             "  a slot is only freed by landing/releasing its claim:",
             "      python tools/units/claims.py release <unit>   # or `slots.py release --slot N`"]
    for s in rows:
        lines.append("  slot %d: %s" % (s["slot"], s["why"]))
    lines.append("  see: python tools/units/slots.py status")
    return "\n".join(lines)


# --- the build-tree guard -----------------------------------------------------------------------------------------

def report_matches(main: str, slot: str) -> bool:
    """The slot's `build/RMHE08/report.json` is byte-identical to MAIN's."""
    a = os.path.join(main, seed.RMHE08_REL, "report.json")
    b = os.path.join(slot, seed.RMHE08_REL, "report.json")
    try:
        if not (os.path.isfile(a) and os.path.isfile(b)):
            return False
        with open(a, "rb") as fa, open(b, "rb") as fb:
            return fa.read() == fb.read()
    except OSError:
        return False


def manifest_outputs(root: str) -> list[str] | None:
    """The `target_path`/`base_path` entries of `root`'s `objdiff.json`, host-separated; None if unreadable."""
    try:
        with open(os.path.join(root, "objdiff.json"), encoding="utf-8") as fh:
            units = json.loads(fh.read())["units"]
        if not isinstance(units, list):
            return None
    except (OSError, ValueError, KeyError, TypeError):
        return None
    out: list[str] = []
    for u in units:
        if not isinstance(u, dict):
            continue
        for key in ("target_path", "base_path"):
            p = u.get(key)
            if isinstance(p, str) and p:
                out.append(p.replace("/", os.sep))
    return out


def compile_outputs(main: str, slot: str) -> tuple[list[str], int, bool]:
    """`(missing, checked, manifest_ok)`: compile outputs MAIN holds and the slot lacks (existence, non-empty),
    over the union of both manifests; `manifest_ok` is False when MAIN has a manifest and the slot has none."""
    slot_units = manifest_outputs(slot)
    main_units = manifest_outputs(main)
    manifest_ok = slot_units is not None or main_units is None
    missing: list[str] = []
    seen: set[str] = set()
    for rel in (main_units or []) + (slot_units or []):
        if rel in seen:
            continue
        seen.add(rel)
        if os.path.isfile(os.path.join(slot, rel)) and os.path.getsize(os.path.join(slot, rel)) > 0:
            continue
        if os.path.isfile(os.path.join(main, rel)):
            missing.append(rel)
    return missing, len(seen), manifest_ok


def verify(main: str, n: int) -> dict:
    """Whether slot `n`'s build tree is proven current against MAIN (split mtime guard, report bytes, the
    compile-output set) - fail closed, with every reason."""
    d = slot_dir(main, n)
    reasons: list[str] = []
    if not os.path.isdir(d):
        return {"slot": n, "dir": d, "ok": False, "main_build_current": False, "build_current": False,
                "report_matches": False, "compile_outputs": False, "compile_outputs_checked": 0,
                "compile_outputs_missing": 0, "reasons": ["slot directory missing: %s" % d]}
    main_current = seed.main_build_is_current(main)
    build_current = seed.build_is_current(d, main)
    report_ok = report_matches(main, d)
    missing, checked, manifest_ok = compile_outputs(main, d)
    compile_ok = bool(manifest_ok and not missing)
    if not main_current:
        reasons.append("MAIN's own build tree is stale (its config.json predates the map/DOL) - `ninja` in MAIN")
    if not build_current:
        reasons.append("slot build/RMHE08/config.json predates MAIN's current map/DOL - the split is stale")
    if not report_ok:
        reasons.append("slot build/RMHE08/report.json differs from MAIN's current report")
    if not manifest_ok:
        reasons.append("slot objdiff.json is missing or unreadable - the official scorer reads it")
    if missing:
        shown = ", ".join(missing[:3])
        more = "" if len(missing) <= 3 else " (+%d more)" % (len(missing) - 3)
        reasons.append("slot is missing %d of the %d compile outputs the official scorer reads (e.g. %s%s) - "
                       "`ninja` here or re-seed" % (len(missing), checked, shown, more))
    return {"slot": n, "dir": d, "ok": bool(main_current and build_current and report_ok and compile_ok),
            "main_build_current": main_current, "build_current": build_current, "report_matches": report_ok,
            "compile_outputs": compile_ok, "compile_outputs_checked": checked,
            "compile_outputs_missing": len(missing), "reasons": reasons}


def refresh_slot(main: str, n: int, force: bool = False, runs: list | None = None) -> dict:
    """Bring slot `n`'s build tree current from MAIN by copying only what differs (`seed_worktree_build(...,
    overwrite=True)`: a file is recopied only when its size or mtime differs), then re-verify. A slot that already
    verifies is left alone unless `force`. Refuses a slot a RUNNING session works in (`force` overrides) and a MAIN
    whose own build is stale (the seed would copy a stale tree). -> `{slot, dir, before, refreshed, seconds, after}`."""
    d = slot_dir(main, n)
    if not os.path.isdir(d):
        raise SystemExit("REFUSED: no slot %d at %s" % (n, d))
    row = slot_state(main, n, runs=runs)
    if row.get("state") == "LIVE" and not force:
        raise SystemExit("REFUSED slot %d: %s - refreshing a build tree under a running lane changes what it "
                         "measures mid-run; --force overrides" % (n, row["why"]))
    before = verify(main, n)
    out = {"slot": n, "dir": d, "before": before, "refreshed": None, "seconds": 0.0, "after": before}
    if before["ok"] and not force:
        return out
    if not seed.main_build_is_current(main):
        raise SystemExit("REFUSED slot %d: MAIN's own build tree is not current (its config.json or build.ninja "
                         "predates its inputs) - run `ninja` in MAIN first, then refresh" % n)
    t0 = time.time()
    out["refreshed"] = seed.seed_worktree_build(main, d, copy_orig=True, overwrite=True)
    out["seconds"] = round(time.time() - t0, 1)
    out["after"] = verify(main, n)
    return out


_NINJA_STEP = re.compile(r"^\[\d+/\d+\]", re.M)


def ninja_pending(slot: str, runner=None) -> tuple[int | None, str]:
    """`(steps, note)` - what `ninja -n` would still run; `steps` is None when ninja cannot say."""
    runner = runner or NINJA_RUNNER or subprocess.run
    try:
        p = runner(["ninja", "-n"], cwd=slot, capture_output=True, text=True, encoding="utf-8", errors="replace")
    except OSError as exc:
        return None, "ninja unavailable (%s)" % exc
    if p.returncode != 0:
        tail = [ln for ln in ((p.stderr or "") + "\n" + (p.stdout or "")).strip().splitlines() if ln.strip()]
        return None, "ninja -n failed: %s" % (tail[-1] if tail else "exit %d" % p.returncode)
    return len(_NINJA_STEP.findall(p.stdout or "")), ""


def claim_currency(main: str, n: int, v: dict | None = None, complete: bool = True, runner=None) -> dict:
    """The handover proof: report bytes, compile outputs, and the pending `ninja` steps - finished here
    (`complete`) and re-counted, so the number is what is left. `ok` is False for an unknown count."""
    runner = runner or NINJA_RUNNER or subprocess.run
    d = slot_dir(main, n)
    if v is None:
        v = verify(main, n)
    steps, note = ninja_pending(d, runner=runner)
    completed = None
    if complete and steps:
        try:
            p = runner(["ninja"], cwd=d, capture_output=True, text=True, encoding="utf-8", errors="replace")
        except OSError as exc:
            p, note = None, "ninja unavailable (%s)" % exc
        if p is not None:
            completed = p.returncode == 0
            if not completed:
                tail = [ln for ln in ((p.stderr or "") + "\n" + (p.stdout or "")).strip().splitlines() if ln.strip()]
                note = "ninja failed: %s" % (tail[-1] if tail else "exit %d" % p.returncode)
            steps, note2 = ninja_pending(d, runner=runner)
            if completed:
                note = note2
                v = verify(main, n)
    return {"report_matches": v["report_matches"], "compile_outputs": v["compile_outputs"],
            "objects_present": v["compile_outputs_checked"] - v["compile_outputs_missing"],
            "objects_checked": v["compile_outputs_checked"], "pending": steps, "pending_note": note,
            "completed": completed, "verify": v, "ok": bool(v["ok"] and steps == 0)}


# --- reset ----------------------------------------------------------------------------------------------------------

def clean_slot(slot: str) -> list[str]:
    """`git clean -ffdx` keeping the warm trees (`SLOT_KEEP`) -> the paths removed."""
    args = ["clean", "-ffdx", "-q"]
    for keep in SLOT_KEEP:
        args += ["-e", keep]
    p = subprocess.run(["git", "-C", slot, *args], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("git clean failed in %s: %s" % (slot, (p.stderr or "").strip()))
    return [line.split(" ", 1)[1] for line in p.stdout.splitlines() if line.startswith("Removing ")]


def reset_slot(main: str, n: int) -> str:
    """Detach the slot at main's current tip and discard every leftover -> main's tip."""
    d = slot_dir(main, n)
    tip = git(["rev-parse", "HEAD"], main)
    git(["checkout", "-f", "-q", "--detach", tip], d)
    clean_slot(d)
    return tip


# --- reclaim: a slot whose branch is already landed ---------------------------------------------------------------

_TREE_OID = re.compile(r"[0-9a-f]{40}|[0-9a-f]{64}")


def merge_tree_oid(returncode: int, stdout: str) -> str | None:
    """`merge-tree --write-tree`'s result tree, or None on a conflict or an unparseable first line."""
    if returncode != 0:
        return None
    lines = (stdout or "").splitlines()
    if not lines or not _TREE_OID.fullmatch(lines[0].strip().lower()):
        return None
    return lines[0].strip().lower()


def merge_tree_of(main: str, ref: str) -> str | None:
    p = Git(main).merge_tree("main", ref)
    return merge_tree_oid(p.returncode, p.stdout)


def branch_fully_applied(main: str, branch: str) -> bool | None:
    """Whether `branch`'s content is already in main: `merge-tree --write-tree main <branch>` equals
    `main^{tree}` (trees, not commits). None when the test cannot answer - never reclaim on None."""
    merged = merge_tree_of(main, branch)
    if merged is None:
        return None
    p = subprocess.run(["git", "rev-parse", "main^{tree}"], cwd=main, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    main_tree = (p.stdout or "").strip().lower()
    if p.returncode != 0 or not _TREE_OID.fullmatch(main_tree):
        return None
    return merged == main_tree


def reclaim_line(out: dict) -> str:
    if not out.get("branch"):
        return ("slot %d: RECLAIMED - debris: a `.used` sentinel with no live claim and no branch checked "
                "out; cleared the sentinel and released the slot (a slot holds a directory, never a branch)"
                % out["slot"])
    return ("slot %d: RECLAIMED - branch %r is already fully applied to main "
            "(git merge-tree --write-tree main %s == main^{tree}); parked %s at %s, detached the worktree, "
            "deleted the branch and released the slot (a slot holds a directory, never a branch)"
            % (out["slot"], out["branch"], out["branch"], out["rescue_ref"], (out["rescue_tip"] or "?")[:8]))


def reclaim_verdict(main: str, n: int, branch: str | None = None, run_registry: str | None = None) -> dict:
    """Read-only: is slot `n` reclaimable? A live claim, an unreadable registry, an unlanded or unprovable
    branch, or a release blocker refuse; a fully applied branch or debris is reclaimable."""
    d = slot_dir(main, n)
    v = {"reclaimable": False, "reason": "", "branch": branch, "applied": None, "kind": None, "live_claim": None}
    if not os.path.exists(os.path.join(d, ".git")):
        v["reason"] = "slot %d is not a worktree (%s)" % (n, d)
        return v
    by_slot, registry_ok = registry_claims_by_slot(main)
    if not registry_ok:
        v["reason"] = ("the claim registry (%s) cannot be read - liveness cannot be established, so the "
                       "slot is NOT reclaimed (fail closed)" % registry.registry_path(main))
        return v
    claim = by_slot.get(n)
    lock = read_lock(main, n)
    live_lock = bool(lock and not lock_stale(main, n, lock))
    if claim or live_lock:
        who = claim or lock
        v["live_claim"] = who
        v["reason"] = ("a live claim holds the slot: %s (%s, branch %s) - never reclaim a slot with live "
                       "work (the claim registry is the authority for liveness)"
                       % (who.get("unit") or "?", who.get("worker") or "?", who.get("branch") or "?"))
        return v
    attached = branch or slot_attached_branch(d)
    v["branch"] = attached
    if attached:
        v["applied"] = applied = branch_fully_applied(main, attached)
        if applied is None:
            v["reason"] = ("the merge-tree test for branch %r could not run or its output was ambiguous - "
                           "failing closed, NOT reclaiming" % attached)
            return v
        if not applied:
            v["reason"] = ("branch %r is NOT fully applied to main (git merge-tree --write-tree main %s != "
                           "main^{tree}) - it is unlanded work" % (attached, attached))
            return v
        v["kind"] = "branch"
    elif marker_present(d):
        v["kind"] = "debris"
    else:
        v["reason"] = "the slot holds no branch and no `.used` sentinel - nothing to reclaim"
        return v
    blockers = release_blockers(main, n, d, registry=run_registry)
    if blockers:
        v["reason"] = ("the slot is %s but not safe to reclaim: %s"
                       % (("on a fully applied branch" if attached else
                           "carrying a `.used` sentinel with no live claim"),
                          "; ".join(summary for summary, _ in blockers)))
        v["blockers"] = blockers
        return v
    v["reclaimable"] = True
    v["reason"] = ("branch %r is fully applied to main" % attached if attached else
                   "a `.used` sentinel with no live claim and no branch checked out (debris)")
    return v


def reclaim_slot(main: str, n: int, branch: str | None = None, unit: str | None = None,
                 registry: str | None = None) -> dict:
    """Reclaim slot `n` when `reclaim_verdict` allows: rescue ref FIRST, detach, delete the branch, clean,
    refresh, clear the sentinel and the lock (debris: clean, clear). `registry` is the session registry."""
    d = slot_dir(main, n)
    out = {"slot": n, "dir": d, "branch": branch, "unit": unit, "reclaimed": False, "reason": "",
           "applied": None, "kind": None, "live_claim": None, "rescue_ref": None, "rescue_tip": None,
           "detached": False, "branch_deleted": False, "cleaned": [], "refreshed": None,
           "marker_cleared": False, "released": False, "steps": [], "line": None}
    verdict = reclaim_verdict(main, n, branch, run_registry=registry)
    out.update({"branch": verdict["branch"], "applied": verdict["applied"], "kind": verdict["kind"],
                "live_claim": verdict["live_claim"], "reason": verdict["reason"]})
    if verdict.get("blockers"):
        out["blockers"] = verdict["blockers"]
    if not verdict["reclaimable"]:
        return out
    if verdict["kind"] == "debris":
        out["cleaned"] = clean_slot(d)
        out["marker_cleared"] = clear_marker(d)
        clear_lock(main, n)
        out["released"] = True
        out["steps"].append({"step": "release"})
        out["reclaimed"] = True
        out["line"] = reclaim_line(out)
        return out
    attached = verdict["branch"]
    named_unit = unit or read_lock(main, n).get("unit")
    branch_slug = naming.slug_of_branch(attached)
    out["unit"] = named_unit or branch_slug or attached
    ref = naming.rescue_ref(named_unit) if named_unit else (naming.RESCUE_PREFIX + (branch_slug or attached))
    tip = slot_head(d)
    p = subprocess.run(["git", "update-ref", ref, attached], cwd=main, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if p.returncode != 0:
        out["reason"] = ("could not park the rescue ref %s (%s) - refusing to delete the branch"
                         % (ref, (p.stderr or "").strip()))
        return out
    out["rescue_ref"], out["rescue_tip"] = ref, tip
    out["steps"].append({"step": "rescue", "ref": ref, "tip": tip})
    main_tip = git(["rev-parse", "HEAD"], main)
    git(["checkout", "-f", "-q", "--detach", main_tip], d)
    out["detached"] = True
    out["steps"].append({"step": "detach", "at": main_tip})
    git(["branch", "-D", attached], main)
    out["branch_deleted"] = True
    out["steps"].append({"step": "delete-branch", "branch": attached})
    out["cleaned"] = clean_slot(d)
    if seed.main_build_is_current(main):
        out["refreshed"] = seed.seed_worktree_build(main, d, copy_orig=True, overwrite=True)
    out["marker_cleared"] = clear_marker(d)
    clear_lock(main, n)
    out["released"] = True
    out["steps"].append({"step": "release"})
    out["reclaimed"] = True
    out["reason"] = "branch %r was fully applied to main" % attached
    out["line"] = reclaim_line(out)
    return out


def announce_reclaim(res: dict) -> None:
    """A reclaim's one line to stderr (diagnostics, never part of a spawn's stdout)."""
    if res.get("reclaimed") and res.get("line"):
        print(res["line"], file=sys.stderr)


# --- acquire --------------------------------------------------------------------------------------------------------

def pick_free(main: str, slot: int | None = None, claim=None, reclaim: bool = True) -> dict:
    """A free slot, falling through occupied ones; `claim(row)` marks it atomically (and may refuse). A slot
    named by number refuses when occupied. With nothing free, a slot on a landed branch is reclaimed and
    taken (`reclaim`). An unreadable claim registry refuses the whole search."""
    by_slot, registry_ok = registry_claims_by_slot(main)
    if not registry_ok:
        raise SystemExit("REFUSED: the claim registry (%s) cannot be read - no slot can be handed out while "
                         "liveness is unknown (fail closed)" % registry.registry_path(main))
    rows = all_slots(main, claims_by_slot=by_slot)
    if not rows:
        raise SystemExit("REFUSED: the slot pool is not initialised - run `python tools/units/slots.py init`")
    if slot is not None:
        row = next((s for s in rows if s["slot"] == slot), None)
        if row is None:
            raise SystemExit("REFUSED: no slot %d (the pool is slots 1..%d)" % (slot, len(rows)))
        if not row["free"] and reclaim:
            res = reclaim_slot(main, slot, branch=row.get("attached"))
            if res["reclaimed"]:
                announce_reclaim(res)
                row = dict(slot_state(main, slot, claims_by_slot=by_slot), landed_reclaim=res)
        if not row["free"]:
            if row["attached"]:
                raise SystemExit("REFUSED slot %d: it still has branch %r checked out - a slot holds a "
                                 "directory, never a branch.\n  release it first: python tools/units/slots.py "
                                 "release --slot %d" % (slot, row["attached"], slot))
            raise SystemExit("REFUSED slot %d: %s" % (slot, row["why"]))
        if claim is not None and not claim(row):
            raise SystemExit("REFUSED slot %d: another acquire claimed it first (its `.used` marker exists); "
                             "retry, or drop `--slot` to let the search fall through to the next free slot"
                             % slot)
        return row
    for row in rows:
        if not row["free"]:
            continue
        if claim is None or claim(row):
            return row
    if reclaim:
        for row in rows:
            if row["free"] or not row.get("attached"):
                continue
            res = reclaim_slot(main, row["slot"], branch=row["attached"])
            if not res["reclaimed"]:
                continue
            announce_reclaim(res)
            row = dict(slot_state(main, row["slot"], claims_by_slot=by_slot), landed_reclaim=res)
            if claim is None or claim(row):
                return row
    raise SystemExit("REFUSED: %s" % capacity_error(main))


def preview(main: str, unit: str, branch: str | None = None, slot: int | None = None,
            worker: str | None = None, force: bool = False) -> dict:
    """What `acquire` would do, touching nothing; surfaces the same sentinel refusal and a predicted reclaim."""
    unit = naming.norm_unit(unit.strip("/"))
    branch = branch or naming.branch_for(unit)
    reclaimed = None
    try:
        row = pick_free(main, slot, reclaim=False)
    except SystemExit:
        if slot is None:
            raise
        verdict = reclaim_verdict(main, slot)
        if not verdict["reclaimable"]:
            raise
        reclaimed = {"slot": slot, "branch": verdict["branch"]}
        row = slot_state(main, slot)
    conflict = None if reclaimed else marker_claim_conflict(row["dir"], owner_label(unit, branch, worker), row["slot"])
    if conflict and not force:
        raise SystemExit(conflict.split("\n")[0] + "\n  (a dry run reports the same refusal `acquire` gives)")
    tip = git(["rev-parse", "HEAD"], main)
    return {"slot": row["slot"], "dir": row["dir"], "branch": branch, "base": tip,
            "owner": owner_label(unit, branch, worker), "reclaim": reclaimed,
            "command": "git -C %s checkout -B %s %s" % (row["dir"], branch, tip)}


def acquire(main: str, unit: str, branch: str | None = None, worker: str | None = None,
            slot: int | None = None, force: bool = False, ninja_runner=None) -> dict:
    """Take a slot for `unit`: mark it `.used` atomically, reset it to main's tip, cut a fresh branch, prove the
    build tree current (re-seed once; refuse a doubt), finish the pending ninja steps, write the lock."""
    unit = naming.norm_unit(unit.strip("/"))
    branch = branch or naming.branch_for(unit)
    if registry.branch_exists(main, branch):
        raise SystemExit("REFUSED: branch %s already exists - the unit is claimed (or was never released).\n"
                         "  see: python tools/units/claims.py list" % branch)
    owner = owner_label(unit, branch, worker)

    def claim(row: dict) -> bool:
        conflict = marker_claim_conflict(row["dir"], owner, row["slot"])
        if conflict and not force:
            raise SystemExit(conflict)
        if row.get("marker_stale"):
            clear_marker(row["dir"])
        return mark_used(row["dir"], unit, branch, owner)

    row = pick_free(main, slot, claim=claim)
    n, d = row["slot"], row["dir"]
    try:
        attached = slot_attached_branch(d)
        if attached:
            raise SystemExit("REFUSED slot %d: it still has branch %r checked out - a slot holds a directory, "
                             "never a branch.\n  release it first: python tools/units/slots.py release "
                             "--slot %d" % (n, attached, n))
        reclaimed = None
        lock = read_lock(main, n)
        if lock and lock_stale(main, n, lock):
            if lock.get("branch") and registry.branch_exists(main, lock["branch"]):
                raise SystemExit("REFUSED slot %d: it holds the unlanded branch %s from a previous round "
                                 "(lock by %s at %s).\n  land it or release it before reusing the slot."
                                 % (n, lock["branch"], lock.get("worker") or "?", lock.get("acquired_at") or "?"))
            reclaimed = lock
            clear_lock(main, n)
        tip = reset_slot(main, n)
        git(["checkout", "-q", "-B", branch, tip], d)
        seed_note = seed.seed_worktree_build(main, d, copy_orig=True, overwrite=False)
        v = verify(main, n)
        refreshed = None
        if not v["ok"]:
            if not seed.main_build_is_current(main):
                raise SystemExit("REFUSED slot %d: MAIN's own build tree is not current (%s);\n"
                                 "  run `ninja` in MAIN so the slot can be re-seeded - never hand a lane a doubt"
                                 % (n, "; ".join(v["reasons"])))
            refreshed = seed.seed_worktree_build(main, d, copy_orig=True, overwrite=True)
            v = verify(main, n)
            if not v["ok"]:
                if not v["main_build_current"]:
                    raise SystemExit("REFUSED slot %d: MAIN's own build tree is not current (%s);\n"
                                     "  run `ninja` in MAIN so the slot can be re-seeded - never hand a lane a doubt"
                                     % (n, "; ".join(v["reasons"])))
                if not v["report_matches"]:
                    raise SystemExit("REFUSED slot %d: slot build/RMHE08/report.json differs from MAIN's - "
                                     "a stale report is how a lane measures the wrong build (%s)"
                                     % (n, "; ".join(v["reasons"])))
                if not v["compile_outputs"]:
                    raise SystemExit("REFUSED slot %d: the slot is missing compile outputs the official "
                                     "scorer reads (%s)\n  re-seed it (release then acquire) before a lane "
                                     "lands" % (n, "; ".join(v["reasons"])))
                print("slot %d: MAIN's tree is current and report.json is byte-identical to MAIN's, so the only "
                      "remaining doubt is config.json's mtime (%s) - accepting; a stale build would have failed "
                      "report_matches" % (n, "; ".join(v["reasons"])))
                v = dict(v, ok=True, accepted_mtime_note=True)
        cur = claim_currency(main, n, v=v, runner=ninja_runner)
        v = cur["verify"]
        write_lock(main, n, {"slot": n, "dir": d, "unit": unit, "branch": branch,
                             "worker": worker or os.environ.get("USERNAME") or os.environ.get("USER") or "unknown",
                             "base": tip, "acquired_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
                             "verified": v["ok"], "report_matches": v["report_matches"],
                             "compile_outputs": v["compile_outputs"], "pending": cur["pending"]})
        return {"slot": n, "dir": d, "worktree": d, "branch": branch, "base": tip, "unit": unit,
                "seeded": seed_note, "refreshed": refreshed, "verify": v, "currency": cur,
                "reclaimed": reclaimed, "landed_reclaim": row.get("landed_reclaim")}
    except BaseException:
        clear_marker(d)
        raise


# --- release --------------------------------------------------------------------------------------------------------

def resolve_slot(main: str, slot: int | None, unit: str | None, branch: str | None) -> tuple[int, dict]:
    rows = all_slots(main)
    if not rows:
        raise SystemExit("REFUSED: the slot pool is not initialised")
    if slot is not None:
        row = next((s for s in rows if s["slot"] == slot), None)
        if row is None:
            raise SystemExit("REFUSED: no slot %d (the pool is slots 1..%d)" % (slot, len(rows)))
        return slot, row.get("lock") or {}
    for row in rows:
        lock = row.get("lock") or {}
        if branch and lock.get("branch") == branch:
            return row["slot"], lock
        if unit and lock.get("unit") == unit:
            return row["slot"], lock
    if branch:
        for row in rows:
            if row["attached"] == branch:
                return row["slot"], row.get("lock") or {}
    raise SystemExit("REFUSED release: no slot holds %s" % (branch or unit or "anything"))


def release(main: str, slot: int | None = None, unit: str | None = None, branch: str | None = None,
            delete_branch: bool = True, rescue: bool = True, refresh: bool = True,
            dry_run: bool = False, force: bool = False, allow_dirty: bool = False,
            registry: str | None = None) -> dict:
    """Return a slot to main's tip: detach, clean, rescue-ref then delete its branch, refresh, unlock. Fails
    closed on `release_blockers` unless `force` (which records what it overrode). `registry` = session dir."""
    n, lock = resolve_slot(main, slot, unit, branch)
    d = slot_dir(main, n)
    branch = branch or lock.get("branch")
    unit = unit or lock.get("unit") or (naming.slug_of_branch(branch) if branch else None)
    result = {"slot": n, "dir": d, "branch": branch, "unit": unit, "detached": False, "cleaned": [],
              "branch_deleted": False, "rescue_ref": None, "refreshed": None, "lock_cleared": False,
              "marker_cleared": False, "dry_run": dry_run, "forced": bool(force), "overridden": []}
    if not os.path.exists(os.path.join(d, ".git")):
        raise SystemExit("REFUSED release slot %d: %s is not a worktree" % (n, d))
    blockers = release_blockers(main, n, d, allow_dirty=allow_dirty, registry=registry)
    if blockers and not force:
        raise SystemExit(release_refusal(main, n, d, blockers))
    result["overridden"] = [summary for summary, _detail in blockers]
    tip = git(["rev-parse", "HEAD"], main)
    if dry_run:
        result["detached"] = True
        result["branch_deleted"] = bool(branch and _branch_exists(main, branch))
        result["lock_cleared"] = bool(lock)
        result["marker_cleared"] = marker_present(d)
        return result
    git(["checkout", "-f", "-q", "--detach", tip], d)
    result["detached"] = True
    result["cleaned"] = clean_slot(d)
    branch_present = bool(branch and _branch_exists(main, branch))
    if delete_branch and branch_present:
        if rescue and not _merged(main, branch) and _ahead(main, branch):
            ref = naming.rescue_ref(unit or naming.slug_of_branch(branch) or branch)
            git(["update-ref", ref, branch], main)
            result["rescue_ref"] = ref
        git(["branch", "-D", branch], main)
        result["branch_deleted"] = True
    if refresh and seed.main_build_is_current(main):
        result["refreshed"] = seed.seed_worktree_build(main, d, copy_orig=True, overwrite=True)
    result["marker_cleared"] = clear_marker(d)
    clear_lock(main, n)
    result["lock_cleared"] = True
    return result


# `release` takes a parameter named `registry` (the session dir), so the claim-registry module is reached through
# these three names inside it.
def _branch_exists(main: str, branch: str) -> bool:
    return registry_mod.branch_exists(main, branch)


def _merged(main: str, branch: str) -> bool:
    return registry_mod.merged_into_main(main, branch)


def _ahead(main: str, branch: str) -> int:
    return registry_mod.commits_ahead(main, branch)


registry_mod = registry


def unlanded_reason(main: str, n: int) -> str | None:
    """Why slot `n` must NOT be released yet (a dirty tree, or a branch main does not contain); None if clear."""
    d = slot_dir(main, n)
    dirty = slot_dirty(d)
    if dirty:
        return "uncommitted changes (%d row(s), e.g. %s)" % (len(dirty), dirty[0])
    branch = slot_attached_branch(d)
    if branch and branch_fully_applied(main, branch) is not True:
        return "branch %s holds commits main does not have" % branch
    return None
