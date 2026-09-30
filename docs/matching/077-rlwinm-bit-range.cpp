/* Demo for idea 77: `rlwinm x,x,0,MB,ME` keeps an inclusive BIT RANGE - so `MB=ME` is a single-bit test
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn rlwinm r3,r0,0,24,24 in bit7_mask      `x & 0x80` keeps bit 24 in place (mask 0x80)
 * EXPECT: insn rlwinm r3,r0,25,31,31 in bit7_test     `(x & 0x80) != 0` rotates it to bit 0
 * EXPECT: count rlwinm 0 in nonzero_test              a plain non-zero test is no mask at all
 * EXPECT: insn rlwinm r3,r3,0,20,23 in mid_field      bits 20..23 kept in place: a field mask, MB..ME spans four bits
 * EXPECT: insn clrlwi r3,r3,24 in low_byte            MB=24,ME=31, SH=0: objdump spells this one `clrlwi` (still an rlwinm)
 */
typedef unsigned char u8;

struct State {
    char pad[8];
    u8 flags;
};

extern "C" {

int bit7_mask(State *s)
{
    return s->flags & 0x80;
}

int bit7_test(State *s)
{
    return (s->flags & 0x80) != 0;
}

int nonzero_test(State *s)
{
    return s->flags != 0;
}

unsigned low_byte(unsigned x)
{
    return x & 0xFF;
}

unsigned mid_field(unsigned x)
{
    return x & 0x0F00;
}

}
