/* Demo for idea 94: One literal pool per TU: a pool literal two units read means the units are one original TU
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: section .sdata2 12       one TU, one pool: 1.5f is pooled ONCE (2.5f, 1.5f, 3.5f = 12 B); `f` and `g` in two TUs would carry 8 + 8 = 16 B
 * EXPECT: bytes .sdata2 402000003fc0000040600000    2.5f, 1.5f, 3.5f in creation order (`f` reads 2.5f before 1.5f), never a second 1.5f
 * EXPECT: reloc @7                 `f` and `g` both relocate against the ONE 1.5f entry (the anonymous @N symbol)
 */
extern "C" {

float f(float x)
{
    return x * 1.5f + 2.5f;
}

float g(float x)
{
    return x * 1.5f + 3.5f;
}

}
