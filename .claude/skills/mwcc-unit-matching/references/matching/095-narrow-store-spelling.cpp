/* Demo for idea 95: Under peephole off, a narrow store wants a compound assignment and a byte is a mask, not a cast
 * FLAGS: -O3 -inline noauto
 * MWCC: Wii/1.3
 * EXPECT: count clrlwi 1 in step_cast     a (u8)(x + 1) store keeps its clrlwi
 * EXPECT: count clrlwi 0 in step_compound   a compound assignment stores the low byte directly
 * EXPECT: count clrlwi 2 in bytes_cast    (u8)(v >> 16) is srwi + clrlwi
 * EXPECT: count clrlwi 0 in bytes_mask    (v >> 16) & 0xFF is a single rlwinm
 */
#pragma peephole off

struct Rec {
    unsigned char step;
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
};

extern "C" void step_cast(Rec* r)
{
    r->step = (unsigned char)(r->step + 1);
}

extern "C" void step_compound(Rec* r)
{
    r->step += 1;
}

extern "C" void bytes_cast(Rec* r, unsigned int v)
{
    r->b0 = (unsigned char)(v >> 16);
    r->b1 = (unsigned char)(v >> 8);
}

extern "C" void bytes_mask(Rec* r, unsigned int v)
{
    r->b0 = (v >> 16) & 0xFF;
    r->b1 = (v >> 8) & 0xFF;
}
