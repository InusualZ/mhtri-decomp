/* Demo for idea 87: -sdata 0 makes every global absolute; an unsized extern (idea 64) does it per symbol
 * FLAGS: -O4,p -inline auto -sdata 0
 * MWCC: Wii/1.3
 * EXPECT: count lis 2 in both                    -sdata 0: both globals are lis+lwz (absolute), none is `@sda21`
 * EXPECT: absent li
 */
extern int g_small;
extern int g_other;

extern "C" int both(void)
{
    return g_small + g_other;
}
