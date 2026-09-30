/* Demo for idea 7: Use scratch files to attribute an instruction choice
 * FLAGS: -O3 -opt nopeephole
 * MWCC: Wii/1.3
 * EXPECT: seq srwi clrlwi in by_cast      the cast form stays a shift plus a mask when the peephole pass is off
 * EXPECT: count rlwinm 0 in by_cast
 * EXPECT: insn rlwinm r3,r3,16,24,31 in by_mask   the explicit mask is already one rlwinm
 * EXPECT: size by_cast 0xc
 * EXPECT: size by_mask 0x8
 */
extern "C" {

unsigned char by_cast(unsigned int x)
{
    return (unsigned char)(x >> 16);
}

unsigned int by_mask(unsigned int x)
{
    return (x >> 16) & 0xff;
}

}
