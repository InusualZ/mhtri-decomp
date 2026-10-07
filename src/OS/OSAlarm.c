/*
 * OS/OSAlarm.c - the OS alarm queue: alarm create/set/cancel, the decrementer exception handler and the shutdown
 *    cancel hook.
 * RANGE. .text 0x804CB440-0x804CBD10 (13 functions); .data 0x8061C0C8-0x8061C0D8; .sbss 0x80795310-0x80795318.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: `AlarmQueue` (.sbss 0x80795310, a `scope:local`
 *    static) is read by `__OSInitAlarm`, `InsertAlarm`, `OSCancelAlarm`, the decrementer callback, the shutdown
 *    hook and `__OSCancelInternalAlarms`, and by nothing outside the range; its `ShutdownFunctionInfo` (.data
 *    0x8061C0C8) is address-taken by `__OSInitAlarm`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; the bodies written here are measured with
 *    them.
 * NAMES. map names throughout; `cPhs_Set` (0x804CBC50) is the map's name for a 16-byte alarm accessor (it stores the
 *    user data at +0x28 and -1 at +0x04) and is kept.
 * RESIDUALS. `cPhs_Set` is the only written body; every other body is unwritten; the shutdown record .data 0x8061C0C8 (0x10 B)
 *    and `AlarmQueue` .sbss 0x80795310 (0x8 B) are claimed but not emitted.
 * SHAPES. none beyond what the bodies show.
 */

typedef struct OSAlarm_s {
    void *handler;         /* +0x00 */
    unsigned int tag;      /* +0x04 */
    unsigned char pad[0x20];
    unsigned int userData; /* +0x28 */
} OSAlarm;

/* 0x804CBC50 (0x10): stores `userData` at +0x28 and 0xFFFFFFFF at +0x04 (the tag). */
void cPhs_Set(OSAlarm *alarm, unsigned int userData)
{
    alarm->userData = userData;
    alarm->tag = -1;
}
