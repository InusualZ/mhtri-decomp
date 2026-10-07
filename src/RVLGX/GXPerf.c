/*
 * RVLGX/GXPerf.c - the SDK GX performance counters (`GXSetGPMetric`, `GXClearGPMetric`).
 *
 * RANGE. `.text` 0x804BA9F0-0x804BB220; `.data` 0x8061ACB8-0x8061ADA0 (2 functions / 0x82C B).
 *   - `.data` jump tables 0x8061ACB8 / 0x8061AD10 are read only by `GXSetGPMetric`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. No body is written (2 functions), the largest `GXSetGPMetric` at 0x804BA9F0 (0x81C B); `python
 *   tools/units/sweepcomments.py --unit RVLGX/GXPerf.c` lists them.
 */
