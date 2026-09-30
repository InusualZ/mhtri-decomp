/* Demo for idea 32: A pragma region is not local to the functions it covers
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: seq clrlwi slwi in aimed_at       the function the pragma was written for
 * EXPECT: seq clrlwi slwi in not_aimed_at   a neighbour inside the region changes too
 * EXPECT: insn rlwinm r3,r3,2,14,29 in after_reset   past the reset the fold is back
 */
typedef unsigned short u16;
typedef unsigned int u32;

extern "C" {

#pragma peephole off
u32 aimed_at(u16 i)
{
    return (u32)i << 2;
}

u32 not_aimed_at(u16 i)
{
    return (u32)i << 2;
}
#pragma peephole reset

u32 after_reset(u16 i)
{
    return (u32)i << 2;
}

}
