"""The linker's message catalogue (RT_STRING) and the classification of its verbose and error output.
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import re
import struct


def decode_string_block(pe, rva, size):
    """A Windows RT_STRING block: uint16 length + length UTF-16LE WCHARs, x16.

    Returns ``[(string_id, text), ...]`` for the non-empty slots; slot *n* of
    the block is id ``block_id * 16 + n``.  This is where the linker keeps its
    own engine messages - including every phase name ``-v`` prints - which is
    why plain ASCII `strings` does not find them.
    """
    raw = pe.read_rva(rva, size)
    out = []
    i, slot = 0, 0
    while i + 1 < len(raw):
        n = struct.unpack_from("<H", raw, i)[0]
        i += 2
        if n == 0:
            slot += 1
            continue
        s = raw[i:i + 2 * n]
        i += 2 * n
        try:
            text = s.decode("utf-16le")
        except UnicodeDecodeError:
            text = s.decode("latin-1")
        out.append((slot, text))
        slot += 1
    return out


def message_catalogue(pe):
    """``{msgid: text}`` for the whole RT_STRING table.

    The id is **the linker's own id**, and it is the one `LoadStringA` is
    called with: blocks are 16 strings, the first block is named 1 and holds
    ids 0..15, so ``msgid = (block_name - 1) * 16 + slot``.  That is not a
    guess: `phases --prove` breaks at the linker's own message loader on a real
    link and reports the ``(id, string)`` pairs it really asks for, and the
    ids observed there (27 = ``Linking: '%c'``, 29 = ``Writing: '%c'``,
    41 = ``Optimizing: '%c'``) are exactly this formula's answer.  An earlier
    revision of this tool numbered from ``block_name * 16``, i.e. 16 too high -
    those labels were indices into the table, not the ids the linker uses.
    """
    msgs = {}
    for block_id, rva, size in pe.string_blocks():
        for slot, text in decode_string_block(pe, rva, size):
            msgs[(block_id - 1) * 16 + slot] = text
    return msgs


PHASE_LINE = re.compile(r"^#\s+Linking:\s|^#\s+Optimizing:\s|^#\s+Writing:\s|"
                        r"^#\s+Layout:\s|^#\s+Compiling:\s|^#\s+Link order")


ANY_PHASE = re.compile(r"^#\s{3}([A-Za-z][A-Za-z ]*?):\s")


def match_message(body, catalogue):
    """The catalogue entry a linker-emitted line came from, or ``None``.

    The catalogue stores a message the way the engine holds it - ``%c``/``%n``
    placeholders included, and often with the line breaks the printer wraps at -
    so both sides are compared with their whitespace collapsed.  The comparison
    is against the literal prefix up to the first placeholder, which is the same
    rule ``phases --prove`` uses to cross-check an observed ``(id, text)`` pair
    against the catalogue, and it is what makes ``diagnose`` able to say *which*
    message an error is.
    """
    body = body.strip()
    while body.startswith("#"):
        body = body.lstrip("#").strip()
    nb = " ".join(body.split())
    for msgid, text in catalogue.items():
        nt = " ".join(text.split())
        head = nt.split("%")[0]
        if head and nb.startswith(head.rstrip()):
            return msgid, text
    return None


def classify_phase(line, catalogue):
    """Name the message a verbose line came from, if the catalogue has it."""
    return match_message(line, catalogue)


def classify_diagnostics(text, catalogue, timeline=None):
    """Every linker diagnostic in a run's output, with its catalogue id and phase.

    ``mwldeppc`` prints an error as a banner (``### mwldeppc.exe Linker
    Error:``) followed by ``#``-prefixed body lines.  The body is matched
    against the message catalogue - the engine's own message - and, when the
    run printed its phase stream (``-v``), the last phase line before the
    diagnostic is attached to it.  A diagnostic whose text is not in the
    catalogue is reported as such rather than given a made-up id.
    """
    out = []
    last_phase = None
    in_error = False
    body = []

    def flush():
        nonlocal body, in_error
        if not body:
            in_error = False
            return
        joined = " ".join(body)
        hit = match_message(joined, catalogue)
        out.append({"text": joined,
                    "msgid": hit[0] if hit else None,
                    "message": hit[1] if hit else None,
                    "phase": last_phase[0] if last_phase else None,
                    "phase_line": last_phase[1] if last_phase else None})
        body = []
        in_error = False

    if timeline is None:
        timeline = parse_timeline(text, phase_kinds(catalogue))
    phase_at = {line.strip(): kind for kind, line in timeline}
    for line in text.splitlines():
        s = line.strip()
        if re.match(r"#+\s*(mwldeppc\.exe\s+)?\S+\s+(Error|Warning|Message)\s*:", s, re.I):
            flush()
            in_error = True
            continue
        if in_error and re.match(r"^#\s", line):
            body.append(s.lstrip("#").strip())
            continue
        if in_error:
            flush()
        if s in phase_at:
            last_phase = (phase_at[s], s)
    flush()
    return out


def phase_kinds(catalogue):
    """The engine's phase names, read off the catalogue.

    A phase message is the shape ``<Name>: '%c'`` - that is what the linker
    prints for ``Linking``, ``Optimizing``, ``Writing``, ``Layout`` and
    ``Compiling`` - so the set of names is *derived* rather than listed.  It
    matters: an error body line such as ``#   undefined: 'foo'`` also looks
    like ``<name>: ...``, and counting it as a phase would mis-attribute the
    very diagnostics this tool is trying to place.
    """
    kinds = set()
    for text in catalogue.values():
        m = re.match(r"^([A-Za-z][A-Za-z ]*):\s*'%c'", text.strip())
        if m:
            kinds.add(m.group(1))
    return kinds or {"Linking", "Optimizing", "Writing", "Layout", "Compiling"}


def parse_timeline(text, kinds=None):
    """The verbose stream as an ordered phase list.

    With ``kinds`` (the catalogue's phase names) only those count as phases; a
    diagnostic body line that merely looks like one is not a phase.
    """
    out = []
    for line in text.splitlines():
        m = ANY_PHASE.match(line)
        if not m:
            continue
        kind = m.group(1)
        if kinds is not None and kind not in kinds:
            continue
        out.append((kind, line.strip()))
    return out

