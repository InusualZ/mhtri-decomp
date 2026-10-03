"""The DOL: its header's seven text and eleven data segments, and address-keyed reads of the image.
Spec: docs/tools/spec/lib-binary.md. CLI: none (library)."""
from __future__ import annotations

import os
import struct
from dataclasses import dataclass

HEADER = 0x100
TEXT_COUNT, DATA_COUNT = 7, 11


class DolError(ValueError):
    """The bytes are too short to hold a DOL header."""


@dataclass(frozen=True)
class Segment:
    """One header slot: `name` is `text0`..`text6` / `data0`..`data10` (the DOL has no section names)."""
    name: str
    kind: str  # "text" | "data"
    index: int
    address: int
    size: int
    offset: int

    @property
    def end(self) -> int:
        return self.address + self.size

    def contains(self, address: int, n: int = 1) -> bool:
        return self.address <= address and address + n <= self.end


class Dol:
    """A DOL image. `segments` are the header slots with a non-zero size, text first, in header order;
    `slots` are all eighteen, empty ones included."""

    def __init__(self, data: bytes, path: str | None = None) -> None:
        if len(data) < HEADER:
            raise DolError("not a DOL (shorter than its 0x100 header)")
        self.data = bytes(data)
        self.path = path
        h = self.data
        text_off = struct.unpack_from(">7I", h, 0x00)
        data_off = struct.unpack_from(">11I", h, 0x1C)
        text_addr = struct.unpack_from(">7I", h, 0x48)
        data_addr = struct.unpack_from(">11I", h, 0x64)
        text_size = struct.unpack_from(">7I", h, 0x90)
        data_size = struct.unpack_from(">11I", h, 0xAC)
        self.bss_address, self.bss_size, self.entry = struct.unpack_from(">III", h, 0xD8)
        slots = [Segment("text%d" % i, "text", i, text_addr[i], text_size[i], text_off[i]) for i in range(TEXT_COUNT)]
        slots += [Segment("data%d" % i, "data", i, data_addr[i], data_size[i], data_off[i]) for i in range(DATA_COUNT)]
        self.slots: tuple[Segment, ...] = tuple(slots)
        self.segments: tuple[Segment, ...] = tuple(s for s in slots if s.size)

    @classmethod
    def read(cls, source: str | os.PathLike | bytes | bytearray) -> "Dol":
        """Parse a path or the bytes of a DOL."""
        if isinstance(source, (bytes, bytearray, memoryview)):
            return cls(bytes(source))
        with open(source, "rb") as handle:
            return cls(handle.read(), os.fspath(source))

    # where
    def section_of(self, address: int) -> Segment | None:
        """The segment holding `address`, or None."""
        for seg in self.segments:
            if seg.address <= address < seg.end:
                return seg
        return None

    @property
    def text_ranges(self) -> list[tuple[int, int]]:
        return [(s.address, s.end) for s in self.segments if s.kind == "text"]

    @property
    def data_ranges(self) -> list[tuple[int, int]]:
        return [(s.address, s.end) for s in self.segments if s.kind == "data"]

    def in_text(self, address: int) -> bool:
        return any(s.kind == "text" and s.address <= address < s.end for s in self.segments)

    # reads
    def bytes_at(self, address: int, n: int) -> bytes | None:
        """The `n` bytes at `address`, or None unless they lie wholly inside one segment."""
        if n < 0:
            return None
        for seg in self.segments:
            if seg.address <= address and address + n <= seg.end:
                off = seg.offset + (address - seg.address)
                return self.data[off:off + n]
        return None

    def bytes_from(self, address: int, n: int) -> bytes | None:
        """Up to `n` bytes from `address` when `address` is inside a segment (the read may run past the
        segment's end, into whatever the file holds next), else None."""
        for seg in self.segments:
            if seg.address <= address < seg.end:
                off = seg.offset + (address - seg.address)
                return self.data[off:off + n]
        return None

    def word(self, address: int) -> int | None:
        """The big-endian u32 at `address`, or None when it is not mapped."""
        raw = self.bytes_at(address, 4)
        return None if raw is None else struct.unpack(">I", raw)[0]

    def words(self, address: int, n: int) -> list[int] | None:
        """`n` big-endian u32s from `address`, or None unless all lie inside one segment."""
        raw = self.bytes_at(address, 4 * n)
        return None if raw is None else list(struct.unpack(">%dI" % n, raw))

    def cstr(self, address: int, limit: int = 256) -> bytes:
        """The NUL-terminated bytes at `address` (at most `limit`), b"" when unmapped."""
        raw = self.bytes_from(address, limit) or b""
        return raw.split(b"\0", 1)[0]
