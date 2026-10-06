"""The lane-manifest row: with `--manifest`, every path the batch changes is one the lane owns and none is read-only.
Spec: docs/tools/spec/landing.md (the format: lane-manifest.md). CLI: none (`land.py land --manifest <path|slug>`)."""
from __future__ import annotations

import fnmatch
import json
import os

from tools.units.landing import state
from tools.units.landing.common import Batch, KIND_BOOKKEEPING

ROW = "the batch touches only the lane's manifest"
#: The key a lane's outbox (`.pi/outbox/<slug>.json`) carries its manifest under.
OUTBOX_KEY = "manifest"
LIST_KEYS = ("owns", "read_only", "units")


def manifest_path(main: str, spec: str) -> str:
    """The file `spec` names: an existing path (absolute, or relative to MAIN), else the slug's outbox
    `MAIN/.pi/outbox/<slug>.json` (a `worker/` prefix is dropped, so a branch name works too)."""
    for cand in (spec, os.path.join(main, spec)):
        if os.path.isfile(cand):
            return cand
    slug = spec[len("worker/"):] if spec.startswith("worker/") else spec
    return os.path.join(main, ".pi", "outbox", slug + ".json")


def shape_problems(data) -> list[str]:
    """What is wrong with a manifest's shape: `lane` a non-empty string, `owns` a non-empty list of non-empty strings,
    `read_only`/`units` lists of strings when present, `base` a string when present. `[]` when it is well formed."""
    if not isinstance(data, dict):
        return ["the manifest is not a JSON object"]
    out = []
    if not isinstance(data.get("lane"), str) or not data.get("lane").strip():
        out.append("`lane` must be a non-empty string")
    if "base" in data and not isinstance(data["base"], str):
        out.append("`base` must be a string (the commit the lane was cut from)")
    for key in LIST_KEYS:
        value = data.get(key, [])
        if not isinstance(value, list) or not all(isinstance(v, str) and v.strip() for v in value):
            out.append("`%s` must be a list of non-empty strings" % key)
    if not data.get("owns"):
        out.append("`owns` is empty: a manifest that owns nothing refuses every path")
    return out


def load(main: str, spec: str) -> tuple[dict | None, str | None]:
    """-> (manifest, None) or (None, why it cannot be read). A file with a top-level `manifest` key (a lane's outbox)
    yields that object; any other JSON object is the manifest itself."""
    path = manifest_path(main, spec)
    try:
        with open(path, encoding="utf-8") as fh:
            data = json.load(fh)
    except OSError as exc:
        return None, "no manifest at %s (%s)" % (path, exc.__class__.__name__)
    except ValueError as exc:
        return None, "%s is not JSON: %s" % (path, exc)
    if isinstance(data, dict) and OUTBOX_KEY in data:
        data = data[OUTBOX_KEY]
    bad = shape_problems(data)
    if bad:
        return None, "%s: %s" % (path, "; ".join(bad))
    return data, None


def matches(path: str, pattern: str) -> bool:
    """True when the repo-relative `path` falls under `pattern`: a glob (`fnmatch`, where `*` also crosses `/`), a
    directory (`src/Network/` or `src/Network` - everything below it), or the exact path."""
    path, pattern = path.replace("\\", "/"), pattern.replace("\\", "/").strip()
    if any(ch in pattern for ch in "*?["):
        return fnmatch.fnmatchcase(path, pattern)
    stem = pattern.rstrip("/")
    return path == stem or path.startswith(stem + "/")


def outside_manifest(paths: list[str], manifest: dict) -> list[tuple[str, str]]:
    """`[(path, why)]` for every path the manifest does not let the batch change: inside a `read_only` glob (it wins
    over `owns`), or matched by no `owns` glob. The decision of the row, pure."""
    owns, read_only = manifest.get("owns") or [], manifest.get("read_only") or []
    out = []
    for path in paths:
        hit = next((g for g in read_only if matches(path, g)), None)
        if hit:
            out.append((path, "read-only `%s`" % hit))
        elif not any(matches(path, g) for g in owns):
            out.append((path, "not in `owns`"))
    return out


# --- the row --------------------------------------------------------------------------------------------------

def manifest_row(b: Batch) -> None:
    """3c. with `--manifest` (`state.MANIFEST`): every changed path is inside the lane's `owns` and outside its
    `read_only` (GATE); an unreadable or malformed manifest is BOOKKEEPING. No row without a manifest."""
    spec = state.MANIFEST
    if not spec:
        return
    manifest, why = load(b.main, spec)
    if manifest is None:
        b.check(ROW, False, why, kind=KIND_BOOKKEEPING,
                remedy="write the lane's manifest (`{lane, base, owns, read_only, units}`) under the `manifest` key "
                       "of its outbox, collect it (`slots.py collect`), or pass --manifest the file's path")
        return
    paths = [p for p in (b.paths or []) if p not in b.scratch]
    bad = outside_manifest(paths, manifest)
    b.check(ROW, not bad, "%d path(s) outside lane %s's manifest: %s"
            % (len(bad), manifest["lane"], "; ".join("%s (%s)" % pw for pw in bad[:8])
               + (" ... (%d more)" % (len(bad) - 8) if len(bad) > 8 else "")),
            info="lane %s: %d path(s), all owned" % (manifest["lane"], len(paths)),
            remedy="move the edit out of the batch into an integrator request (`docs/tools/spec/integrate.md`) for "
                   "the file's owner, or extend the lane's manifest with the orchestrator's approval and re-run")
