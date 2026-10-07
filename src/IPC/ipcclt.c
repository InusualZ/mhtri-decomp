/*
 * IPC/ipcclt.c - the SDK IPC client (IOS_Open/Close/Read/Write/Seek/Ioctl/Ioctlv, interrupt handler).
 *
 * RANGE. `.text` 0x804BB310-0x804BD070; `.bss` 0x80747300-0x807473C0; `.sdata` 0x80793EA8-0x80793EB0; `.sbss`
 *   0x80795240-0x80795258 (25 functions / 0x1CD8 B).
 *   - `.sdata` `__mailboxAck` / `hid` (0x80793EA8 / 0x80793EAC), `.bss` `__responses`, `__timeout_alarm` and the
 *     reboot buffer (0x80747300..0x807473C0) and `.sbss` 0x80795240..0x80795258 are read only here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * RESIDUALS. `strnlen` (0x804BB310) has no data and is placed here by position (the first client helper). No body is
 *   written (25 functions), the largest `IOS_IoctlvReboot` at 0x804BCD70 (0x2FC B); `python
 *   tools/units/sweepcomments.py --unit IPC/ipcclt.c` lists them.
 */
