/* Demo for idea 66: A narrow RETURN TYPE is visible at the caller
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count clrlwi 1 in forward_wide      callee returns s32, the caller narrows it to u16: a mask
 * EXPECT: count clrlwi 0 in forward_narrow    callee returns u16 already: no mask, a plain tail call
 * EXPECT: size forward_narrow 4
 */
typedef unsigned short u16;
typedef int s32;

extern "C" {

s32 flag_wide(void);
u16 flag_narrow(void);

u16 forward_wide(void)
{
    return flag_wide();
}

u16 forward_narrow(void)
{
    return flag_narrow();
}

}
