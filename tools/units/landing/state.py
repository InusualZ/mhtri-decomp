"""The invocation's allowances (`--allow-rule10/12`, `--allow-orphan`), `--unit-rename` pairs, `--manifest`, their record.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

from tools.lib.lanes import naming


ALLOW_RULE10: list[str] = []


def set_allow_rule10(keys: list[str] | None) -> None:
    """Record the rule-10 keys *this invocation* accepts deliberately - the command's own audit trail.

    The row lives in `verify()`, which `land()` and the CLI's `--dry-run` path both reach, so the value
    travels on the module rather than through two more signatures. It is set only from the command line and
    printed by the row; nothing in a file can grant it, which is the difference between this and the
    `rule 7 deferred` key the no-exemption ruling removed.
    """
    global ALLOW_RULE10                                                   # noqa: PLW0603 - one invocation
    ALLOW_RULE10 = [a.strip() for a in (keys or []) if a.strip()]


ALLOW_ORPHAN: list[str] = []


def set_allow_orphan(addresses: list[str] | None) -> None:
    """Record the data addresses *this invocation* accepts as deliberately left unclaimed.

    Mirrors `set_allow_rule10`: set only from a command line (`--allow-orphan 0x8079B83C`), printed by the
    row, never a key in a file. The address is the orphan data object's (or a byte inside a shrunk claim);
    an allowance that matches nothing excuses nothing, so the refusal it was meant for stands.
    """
    global ALLOW_ORPHAN                                                   # noqa: PLW0603 - one invocation
    ALLOW_ORPHAN = [a.strip() for a in (addresses or []) if a and a.strip()]


UNIT_RENAMES: dict[str, str] = {}


UNIT_RENAME_LISTS: dict[str, list[str]] = {}


UNIT_SURVIVORS: set[str] = set()   # units named in --units: still registered after the batch, so a donor among them keeps its own entry


def set_unit_renames(pairs: list[str] | None, survivors: list[str] | None = None) -> None:
    """Record `OLD=NEW` unit renames *this invocation* declares (a batch that `git mv`s or folds registered units).

    The base snapshots (undefrefs, orphans) are keyed by unit name, so a renamed unit would read as a NEW
    unit and every pre-existing finding it carries as an addition. The pairs travel on the module, come only
    from a command line (`--unit-rename Network/fn_803D3CE8=Network/NetworkSessionManager`), and the landing
    log prints them; nothing in a file can declare one. Several OLDs may share one NEW (a fold), and ONE OLD
    may be declared onto several NEWs (a unit split across absorbers): the snapshot merge (`rename_snapshot_keys`)
    copies its entry to every NEW, while `UNIT_RENAMES` (handed to the data-closure row, which derives its own
    fold map) keeps only the OLDs with exactly one target so an explicit pair never narrows a derived map.
    `OLD=` with no NEW says the unit's base entries go with it.
    """
    global UNIT_RENAMES, UNIT_RENAME_LISTS, UNIT_SURVIVORS                # noqa: PLW0603 - one invocation
    UNIT_SURVIVORS = {naming.norm_unit(u.strip('/')) for u in (survivors or []) if u.strip()}
    lists: dict[str, list[str]] = {}
    for item in pairs or []:
        if "=" in item:
            old, new = item.split("=", 1)
            if old.strip():
                key = naming.norm_unit(old.strip().strip("/"))
                tgt = naming.norm_unit(new.strip().strip("/")) if new.strip() else ""
                if tgt not in lists.setdefault(key, []):
                    lists[key].append(tgt)
    UNIT_RENAME_LISTS = lists
    # a SURVIVING donor (still registered after the batch) is not a rename for the data row: its base keys stay
    # under its own name there (the row derives the fold map itself); only the undefrefs merge uses the pair
    UNIT_RENAMES = {k: v[0] for k, v in lists.items() if len(v) == 1 and k not in UNIT_SURVIVORS}


def rename_snapshot_keys(snapshot: dict) -> dict:
    """The base snapshot with every renamed unit's key moved to its new name(s) (declared renames only).

    Several OLDs declared onto one NEW (a fold) are **merged** under NEW - refs unioned, the first entry's other
    fields kept - never overwritten by whichever came last; one OLD declared onto several NEWs is copied to each;
    `OLD=` (no NEW) drops the entry with the unit.
    """
    if not UNIT_RENAME_LISTS or not snapshot:
        return snapshot
    out: dict = {}
    for k, v in sorted(snapshot.items(), key=lambda kv: kv[0] in UNIT_RENAME_LISTS):      # NEW's own entries first
        targets = list(UNIT_RENAME_LISTS.get(k) or [k])
        if k in UNIT_SURVIVORS and k not in targets:
            targets.append(k)
        for nk in targets:
            if not nk:
                continue
            if nk not in out:
                out[nk] = v
            elif isinstance(out[nk], dict) and isinstance(v, dict):
                out[nk] = dict(out[nk], refs=sorted(set(out[nk].get("refs") or []) | set(v.get("refs") or [])))
    return out


MANIFEST: str | None = None


def set_manifest(spec: str | None) -> None:
    """Record the lane manifest *this invocation* judges the batch against (`--manifest <path|slug>`).

    Travels on the module like the allowances: the row (`rows/manifest.py`) lives in `verify`, and the landing log
    records the id. Set only from a command line; `None` (the default) keeps the gate without the row."""
    global MANIFEST                                                       # noqa: PLW0603 - one invocation
    MANIFEST = spec.strip() if spec and spec.strip() else None


ALLOW_RULE12: list[str] = []


def set_allow_rule12(tokens: list[str] | None) -> None:
    """Record the rule-12 tokens *this invocation* accepts deliberately - the command's own audit trail.

    Mirrors `set_allow_rule10`: the value travels on the module (the row lives in `verify`, which both
    `land()` and the CLI's `verify` path reach), it is set **only** from a command line - never from a key
    in a file, which is what the no-exemption ruling removed - and the row prints it, so the landing log
    carries the token it excused. The token is the at-fault symbol name rule 12 fires on (the `extern`'s
    identifier), the rename-stable key `stylelint --list-added` prints.
    """
    global ALLOW_RULE12                                                   # noqa: PLW0603 - one invocation
    ALLOW_RULE12 = [t.strip() for t in (tokens or []) if t and t.strip()]


def allowances(allow_regression: list[str] | None = None, check_outbox: bool = True,
               no_selftests: bool = False) -> dict:
    """Every allowance this invocation grants, as the landing log's `allow` (`lib.lanes.landlog.ALLOW_CLASSES`).

    The flag arguments are the ones that travel as parameters (`--allow-regression`, `--no-outbox`,
    `--no-selftests`); the rest is this module's state. A list class keeps the command line's order with
    duplicates dropped; a flag class is `True`; an unused class is absent, so a landing with none is `{}`.
    """
    def uniq(items) -> list[str]:
        return list(dict.fromkeys(i.strip() for i in (items or []) if i and i.strip()))

    out: dict = {}
    for cls, items in (("regression", allow_regression), ("rule10", ALLOW_RULE10), ("rule12", ALLOW_RULE12),
                       ("orphan", ALLOW_ORPHAN)):
        if uniq(items):
            out[cls] = uniq(items)
    if not check_outbox:
        out["no_outbox"] = True
    if no_selftests:
        out["no_selftests"] = True
    pairs = ["%s=%s" % (old, new) for old, news in UNIT_RENAME_LISTS.items() for new in news]
    if pairs:
        out["unit_renames"] = pairs
    return out


def allow_lines(allow: dict) -> list[str]:
    """One `allow: <class> <entries>` line per allowance class - the commit body's record (`allowances`)."""
    lines = []
    for cls, value in allow.items():
        lines.append("allow: %s%s" % (cls, "" if value is True else " " + ", ".join(value)))
    return lines
