/* Demo for idea 28: A kept `bl` to a tiny static names the unit's inlining setting
 * FLAGS: -O4,p -inline noauto
 * MWCC: Wii/1.3
 * EXPECT: count bl 2 in caller           `-inline noauto`: the un-marked static helper is a kept call, twice
 * EXPECT: reloc tiny_helper                the relocation the retail `bl fn_XXXXXXXX` corresponds to
 * EXPECT: size caller 0x48                 (under `-inline noauto` the same source is 0x14 B: both calls folded away)
 * EXPECT: count bl 0 in caller_inline      a function marked `inline` is still inlined under `noauto`
 */
extern "C" {

static int tiny_helper(int x)
{
    return x * 3 + 1;
}

int caller(int a, int b)
{
    return tiny_helper(a) + tiny_helper(b);
}

inline int marked_inline(int x)
{
    return x * 5 + 2;
}

int caller_inline(int a)
{
    return marked_inline(a);
}

}
