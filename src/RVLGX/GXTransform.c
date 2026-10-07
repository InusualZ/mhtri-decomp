/*
 * RVLGX/GXTransform.c - the SDK GX transform (projection, matrix loads, viewport, scissor, clip).
 *
 * RANGE. `.text` 0x804BA230-0x804BA9F0; `.sdata2` 0x8079D1F0-0x8079D200 (20 functions / 0x74C B).
 *   - `.sdata2` 0x8079D1F0..0x8079D200 (0.0f, 1.0f, 0.5f, 342.0f) is read by the project / viewport functions; the
 *     first function reads 0.0f/1.0f/0.5f, separating it from the fog pool before it
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (20 functions), the largest `fn_804BA230` at 0x804BA230 (0x188 B); `python
 *   tools/units/sweepcomments.py --unit RVLGX/GXTransform.c` lists them.
 */
