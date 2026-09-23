/*
 * Revolution SDK OS library: the alarm helper that stores a userData value and clears the tag.
 *
 * .text 0x804CBC50-0x804CBC60 - one function, cPhs_Set, 4 instructions and 0x10 bytes: store userData at
 * +0x28, store 0xFFFFFFFF at +0x04 (the tag). No frame, no callee-saved register, no relocation.
 * The name stays generated on purpose: the neighbourhood is the OSAlarm cluster (OSGetAlarmUserData at
 * 0x804CBC40, the tag/userData cancel loop at 0x804CBC60, __OSInitAlarm / InsertAlarm / OSSetAlarm /
 * OSCancelAlarm in the same retail region), and an unnamed helper in that cluster is what a file-static
 * function looks like - there is no link-time name to recover, so inventing one would be wrong.
 * The range is provisional: one function of that cluster so far.
 * Registered NonMatching in configure.py, lib OS (Wii/1.3, cflags_base); the two-variant probe said so:
 * -O4,p reproduces the retail window byte-for-byte while -O3 leaves the `li r0,-1` after the first store.
 * Measured: fuzzy_match_percent 100.0 - 16 B / 4 instructions, byte-identical.
 * Residual: none.
 */

typedef struct OSAlarm_s {
    void *handler;         /* +0x00 */
    unsigned int tag;      /* +0x04 */
    unsigned char pad[0x20];
    unsigned int userData; /* +0x28 */
} OSAlarm;

void cPhs_Set(OSAlarm *alarm, unsigned int userData)
{
    alarm->userData = userData;
    alarm->tag = -1;
}
