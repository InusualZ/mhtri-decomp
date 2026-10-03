"""Stamped caches: a JSON artefact keyed by the signature of its inputs, with a four-state freshness.
Spec: docs/tools/spec/lib-cache.md. CLI: none (library)."""
from __future__ import annotations

import hashlib
import json
import os
from typing import Any, Callable, Iterable, Sequence

from tools.lib import text

STATES = ("missing", "unstamped", "stale", "fresh")


def content_hash(path: str | os.PathLike, algo: str = "sha1") -> str:
    """The hex digest of a file's bytes."""
    h = hashlib.new(algo)
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def stat_key(path: str | os.PathLike) -> list[int] | None:
    """`[mtime_ns, size]` of a file, or None when it cannot be stat'ed - the cheap per-file key."""
    try:
        st = os.stat(path)
    except OSError:
        return None
    return [st.st_mtime_ns, st.st_size]


def stat_digest(paths: Iterable[str | os.PathLike], base: str | os.PathLike, algo: str = "sha1") -> str:
    """One digest over every file's name (relative to `base`, forward-slashed), size and mtime.

    The signature for a large dump (thousands of files): a content hash would cost more than the cache
    saves, and any rewrite of a file moves its mtime.
    """
    h = hashlib.new(algo)
    for path in paths:
        st = os.stat(path)
        name = os.path.relpath(path, base).replace("\\", "/")
        h.update(("%s\0%d\0%d\n" % (name, st.st_size, st.st_mtime_ns)).encode("utf-8"))
    return h.hexdigest()


class Stamped:
    """A JSON cache at `path` whose payload is valid only for the inputs it was built from.

    `inputs` is a list of paths (or a callable returning one, for an input set that is discovered).
    `mode="content"` signs each input by its bytes (sha256); `mode="stat"` by name, size and mtime (for a
    large dump). `schema` is mixed in, so a format change is `stale`, never misread. The cache never
    rebuilds itself: `--rebuild` is the caller's flag.
    """

    def __init__(self, path: str | os.PathLike, inputs: Sequence[str | os.PathLike] | Callable[[], Sequence],
                 mode: str = "content", schema: Any = None, base: str | os.PathLike | None = None) -> None:
        if mode not in ("content", "stat"):
            raise ValueError("mode is 'content' or 'stat', not %r" % mode)
        self.path = os.fspath(path)
        self._inputs = inputs
        self.mode = mode
        self.schema = schema
        self.base = os.fspath(base) if base is not None else None

    def inputs(self) -> list[str]:
        items = self._inputs() if callable(self._inputs) else self._inputs
        return sorted(os.fspath(p) for p in items)

    def signature(self) -> str:
        """The digest of the schema plus every input; a missing input signs as absent."""
        files = self.inputs()
        base = self.base or os.path.commonpath([os.path.abspath(p) for p in files] or [os.getcwd()])
        h = hashlib.sha256(json.dumps(self.schema, sort_keys=True, default=str).encode("utf-8"))
        for p in files:
            rel = os.path.relpath(os.path.abspath(p), base).replace("\\", "/")
            if not os.path.exists(p):
                h.update(("%s\0absent\n" % rel).encode("utf-8"))
            elif self.mode == "stat":
                st = os.stat(p)
                h.update(("%s\0%d\0%d\n" % (rel, st.st_size, st.st_mtime_ns)).encode("utf-8"))
            else:
                h.update(("%s\0%s\n" % (rel, content_hash(p, "sha256"))).encode("utf-8"))
        return h.hexdigest()

    def _read(self) -> dict | None:
        try:
            with open(self.path, encoding="utf-8") as fh:
                data = json.load(fh)
        except (OSError, ValueError):
            return None
        return data if isinstance(data, dict) else None

    def state(self) -> str:
        """`missing` (no file), `unstamped` (unreadable or no signature), `stale` or `fresh`."""
        if not os.path.exists(self.path):
            return "missing"
        data = self._read()
        if data is None or "signature" not in data or "payload" not in data:
            return "unstamped"
        return "fresh" if data["signature"] == self.signature() else "stale"

    def load(self) -> Any:
        """The payload when the cache is fresh, else None."""
        data = self._read()
        if data is None or data.get("signature") != self.signature() or "payload" not in data:
            return None
        return data["payload"]

    def save(self, payload: Any) -> None:
        """Write `payload` stamped with the current signature (atomically)."""
        body = {"schema": self.schema, "signature": self.signature(), "payload": payload}
        text.atomic_write(self.path, json.dumps(body, sort_keys=True) + "\n")
