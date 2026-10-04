"""The gate's commit message file (`.git/land_msg.txt`): only a green gate writes it, every refusal clears it.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os


def land_message_path(main: str) -> str:
    """The gate's commit message. Only a green gate writes it (`write_land_message`); a red one clears it."""
    return os.path.join(main, ".git", "land_msg.txt")


def write_land_message(main: str, body: str) -> str:
    path = land_message_path(main)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(body)
    return path


def message_body_with_subject(body: str, subject: str) -> str:
    """Replace the gate message's subject line with a deliberate `--message`, keeping the gate's body.

    `verify` writes `land: <units>` followed by the ledger and gate summary. A caller's `--message` replaces
    only that first line, so the landed commit still records the ledger delta it was gated against.
    """
    _first, sep, rest = body.partition("\n")
    return subject + sep + rest


def clear_land_message(main: str) -> str | None:
    """Remove a stale message so a failed gate cannot be committed through `git commit -F .git/land_msg.txt`."""
    path = land_message_path(main)
    if os.path.exists(path):
        os.remove(path)
        return path
    return None


def message_error(subject: str | None) -> str | None:
    """Refuse an empty or whitespace-only `--message` before the gate does any work.

    The incident (2026-09-24, `71c244f9`): `--message "$(cat /tmp/msg1.txt)"`, where the shell's `/tmp` is not
    the one the file was written to, expands to the empty string. An empty override used to be dropped by
    `if subject:`, so the batch landed under the gate's fallback subject `land: <units>` instead of the message
    the worker wrote. Omitting `--message` is how a caller deliberately asks for that fallback; an empty or
    blank argument is the caller's bug, refused here - loudly, naming the argument - at the same cost as a lint
    refusal, and before `verify` runs.
    """
    if subject is None:
        return None
    if not subject.strip():
        kind = "empty" if subject == "" else "whitespace-only"
        return ("--message is %s (%r): pass the subject you meant, or omit --message to use the gate's "
                "default subject" % (kind, subject))
    return None


def answer_line(output: str) -> str:
    """The `LANDED`/`REFUSED` answer line of a `land` run: the last non-empty line (LEDGER sits above it)."""
    lines = [ln for ln in output.splitlines() if ln.strip()]
    return lines[-1] if lines else ""
