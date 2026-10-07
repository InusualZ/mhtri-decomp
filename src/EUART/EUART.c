/*
 * EUART/EUART.c - the SDK EXI UART debug output (`EUARTInit`, `InitializeUART`, `WriteUARTN`).
 *
 * RANGE. `.text` 0x804AFB50-0x804AFED0; `.sbss` 0x80795188-0x80795198; `.sdata2` 0x8079D0E8-0x8079D0F0 (3 functions /
 *   0x374 B).
 *   - four `.sbss` statics 0x80795188..0x80795198 and `.sdata2` 0x8079D0E8 are read only by these three functions;
 *     `WriteUARTN` calls into the EXI bus driver that follows
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_os`), the block of the unit this range was cut
 *   from; unmeasured until bodies exist.
 * NAMES. the unit name is a GUESS (the library's first source file).
 * RESIDUALS. No body is written (3 functions), the largest `WriteUARTN` at 0x804AFCA0 (0x230 B); `python
 *   tools/units/sweepcomments.py --unit EUART/EUART.c` lists them.
 */
