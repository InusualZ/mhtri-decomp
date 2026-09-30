/* Demo for idea 86: objdump prints the underlying rlwinm, never the extrwi/clrlslwi alias
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count rlwinm 1 in extract_field        `(x>>8)&0xFF` is `extrwi r3,r3,8,16` to a reader, `rlwinm r3,r3,24,24,31` to objdump
 * EXPECT: absent extrwi                          the alias never appears in the disassembly, so counting it proves nothing
 * EXPECT: absent clrlslwi
 * EXPECT: insn rlwinm r3,r3,2,14,29 in clear_and_shift   the same for `clrlslwi r3,r3,16,2`
 * EXPECT: contains clrlwi                       ... but it does print other aliases (clrlwi, srwi, slwi): those are countable
 */
extern "C" unsigned extract_field(unsigned x)
{
    return (x >> 8) & 0xFF;
}

extern "C" unsigned low_byte(unsigned x)
{
    return x & 0xFF;
}

extern "C" unsigned clear_and_shift(unsigned x)
{
    return (x & 0xFFFF) << 2;
}
