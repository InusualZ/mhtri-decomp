/* WENC/wenc.cpp - the Wii remote report encryption (`WENCGetEncodeData`).
 * RANGE. .text 0x804EB22C-0x804EB510 (1 function); .rodata 0x80573C40-0x80573C80; .sdata2 0x8079D3C0-0x8079D3C8.
 *   Edges: its .rodata table and .sdata2 literal are read only by this function; both edges are 4-aligned (the
 *   function starts at 0x804EB22C, so the TU compiled without 16-byte function alignment, unlike its neighbours,
 *   whose starts are all 16-aligned).
 * FLAGS. the `OS` lib block of configure.py (`cflags_os`); the missing function alignment is unmeasured until the
 *   body exists.
 * NAMES. `WENCGetEncodeData` is the map's name.
 * RESIDUALS. the body is unwritten (stub).
 */
