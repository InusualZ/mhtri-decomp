"""The lane-manifest row: with `--manifest`, every path the batch changes is one the lane owns and none is read-only;
a map under `own_rows` changes only rows inside the batch units' claims.
Spec: docs/tools/spec/landing.md (the format: lane-manifest.md). CLI: none (`land.py land --manifest <path|slug>`)."""
from __future__ import annotations

import fnmatch
import json
import os
from collections import Counter

from tools.lib.git import Git
from tools.lib.lanes import naming
from tools.lib.project import splits as _splits
from tools.lib.project import symbols as _symbols
from tools.units.landing import state
from tools.units.landing.common import Batch, KIND_BOOKKEEPING

ROW = "the batch touches only the lane's manifest"
#: The key a lane's outbox (`.pi/outbox/<slug>.json`) carries its manifest under.
OUTBOX_KEY = "manifest"
LIST_KEYS = ("owns", "read_only", "units", "own_rows")


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
    `read_only`/`units`/`own_rows` lists of strings when present, `base` a string when present. `[]` when it is well
    formed."""
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


def row_paths(paths: list[str], manifest: dict) -> list[str]:
    """The changed paths an `own_rows` entry matches and no `read_only` entry does: maps the batch may change only row
    by row (`out_of_claim_rows`). An `own_rows` match wins over `owns`, so listing the map in both still narrows it."""
    own_rows, read_only = manifest.get("own_rows") or [], manifest.get("read_only") or []
    return [p for p in paths if any(matches(p, g) for g in own_rows) and not any(matches(p, g) for g in read_only)]


def outside_manifest(paths: list[str], manifest: dict) -> list[tuple[str, str]]:
    """`[(path, why)]` for every path the manifest does not let the batch change: inside a `read_only` glob (it wins
    over `owns` and `own_rows`), or matched by no `owns` and no `own_rows` glob (an `own_rows` path is judged row by
    row, `out_of_claim_rows`). The decision of the row, pure."""
    owns, read_only = manifest.get("owns") or [], manifest.get("read_only") or []
    by_row = set(row_paths(paths, manifest))
    out = []
    for path in paths:
        hit = next((g for g in read_only if matches(path, g)), None)
        if hit:
            out.append((path, "read-only `%s`" % hit))
        elif path in by_row:
            continue
        elif not any(matches(path, g) for g in owns):
            out.append((path, "not in `owns`"))
    return out


def changed_rows(old_text: str, new_text: str) -> tuple[list[str], list[str]]:
    """`(removed, added)` lines of a map edit, compared as multisets (a moved line is no change): a rename, a resize or
    a split is a removed row plus added row(s), a deletion a removed row only, an addition an added row only."""
    def lines(text: str) -> Counter:
        return Counter(line.rstrip("\r") for line in text.split("\n") if line.strip())
    old, new = lines(old_text), lines(new_text)
    return sorted((old - new).elements()), sorted((new - old).elements())


def out_of_claim_rows(old_text: str, new_text: str, claims: _splits.Splits,
                      units: list[str]) -> tuple[int, list[str]]:
    """-> (rows judged, problems): every changed row of a map (`changed_rows`) lies at an address one of `units` claims
    in the row's own section (the `splits.txt` key without its extension), or it is a problem. A changed line that is
    no map row is a problem too: the map's other lines are nobody's own rows. Pure."""
    stems = {_splits.stem(naming.norm_unit(u.strip("/"))) for u in units if u.strip()}
    removed, added = changed_rows(old_text, new_text)
    problems = []
    for sign, line in [("-", x) for x in removed] + [("+", x) for x in added]:
        row = _symbols.parse_line(line)
        if row is None:
            problems.append("%s%s (not a map row)" % (sign, line.strip()[:80]))
            continue
        claim = claims.covering(row.section, row.address)
        if claim is None or _splits.stem(claim.unit) not in stems:
            owner = ("claimed by %s" % _splits.stem(claim.unit)) if claim else "claimed by no unit"
            problems.append("%s%s %s:0x%08X (%s)" % (sign, row.name, row.section, row.address, owner))
    return len(removed) + len(added), problems


def own_row_problems(main: str, paths: list[str], units: list[str]) -> tuple[int, list[str]]:
    """`out_of_claim_rows` for every `own_rows` map the batch changed: HEAD's text against the working tree's, the
    claims read from the working tree's `splits.txt` beside the map (a batch may register its unit in the same change).
    -> (rows judged, problems prefixed with the map's path)."""
    judged, problems = 0, []
    for path in paths:
        full = os.path.join(main, *path.split("/"))
        old = (Git(main).show("HEAD", path) or b"").decode("utf-8", "replace")
        try:
            with open(full, encoding="utf-8", errors="replace", newline="") as fh:
                new = fh.read()
        except OSError:
            new = ""
        splits_path = os.path.join(os.path.dirname(full), "splits.txt")
        if not os.path.isfile(splits_path):
            problems.append("%s: no splits.txt beside it to read the claims from" % path)
            continue
        n, bad = out_of_claim_rows(old, new, _splits.Splits.read(splits_path), units)
        judged += n
        problems.extend("%s: %s" % (path, x) for x in bad)
    return judged, problems


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
    by_row = row_paths(paths, manifest)
    units = b.unit_units or b.units
    judged, rows_bad = own_row_problems(b.main, by_row, units) if by_row else (0, [])
    detail = []
    if bad:
        detail.append("%d path(s) outside lane %s's manifest: %s"
                      % (len(bad), manifest["lane"], "; ".join("%s (%s)" % pw for pw in bad[:8])
                         + (" ... (%d more)" % (len(bad) - 8) if len(bad) > 8 else "")))
    if rows_bad:
        detail.append("%d map row(s) outside the claims of the batch's units (%s): %s"
                      % (len(rows_bad), ", ".join(units) or "none named", "; ".join(rows_bad[:8])
                         + (" ... (%d more)" % (len(rows_bad) - 8) if len(rows_bad) > 8 else "")))
    b.check(ROW, not bad and not rows_bad, " | ".join(detail),
            info="lane %s: %d path(s), all owned%s" % (manifest["lane"], len(paths),
                                                       ", %d own map row(s), all inside the batch's claims" % judged
                                                       if by_row else ""),
            remedy="move the edit out of the batch into an integrator request (`docs/tools/spec/integrate.md`) for "
                   "the file's owner (a foreign map row: a name-registry proposal, `tools/symbols/names.py`), or "
                   "extend the lane's manifest with the orchestrator's approval and re-run")
