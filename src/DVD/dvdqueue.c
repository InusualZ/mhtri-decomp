/*
 * DVD/dvdqueue.c - the SDK DVD waiting queue (clear, push, pop, check, dequeue).
 *
 * RANGE. `.text` 0x804AB040-0x804AB2C0; `.bss` 0x807466F0-0x80746720 (6 functions / 0x25C B).
 *   - six functions whose only data is `WaitingQueue` (.bss 0x807466F0); the neighbours on both sides read disjoint
 *     data (clean seam at both edges)
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the queue helpers are named in the SDK queue scheme).
 * RESIDUALS. No body is written (6 functions), the largest `__DVDPopWaitingQueue` at 0x804AB0F0 (0xA0 B); `python
 *   tools/units/sweepcomments.py --unit DVD/dvdqueue.c` lists them.
 */
