/*
 * PAD/pad.c - the PAD library: pad state, origin update, sampling handler, `PADClamp` users and
 *    `__PADDisableRecalibration`.
 * RANGE. .text 0x804D82D0-0x804D9B3C (16 functions); .data 0x80629B38-0x80629B90; .bss 0x8074E3B0-0x8074E460; .sdata
 *    0x80793FE0-0x80794000; .sbss 0x80795410-0x80795440.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: the build string "<< RVL_SDK - PAD ... (0x4302_145) >>" opens .data at 0x80629B38; the pad state
 *    (.bss 0x8074E3B0..0x8074E460, .sbss 0x80795410..0x80795440, .sdata 0x80793FE0..0x80794000) is read only by
 *    these functions; `RSONotifyModuleLoaded` (0x804D9B3C) is the RSO notify thunk run.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut and not measured
 *    with bodies yet.
 * NAMES. `PADOriginUpdateCallback`, `SamplingHandler` and `__PADDisableRecalibration` are the map's names; the file
 *    name is a GUESS.
 * RESIDUALS. every body is unwritten.
 * SHAPES. none yet.
 */
