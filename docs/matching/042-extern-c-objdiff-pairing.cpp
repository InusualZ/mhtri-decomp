/* Demo for idea 42: A C++ free function needs `extern "C"` so its symbol is the plain map name
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: size fn_plain_c 0x8              the extern "C" definition is emitted under exactly this name
 * EXPECT: absent fn_plain_c__Fi            ...and has no mangled twin
 * EXPECT: size fn_mangled__Fi 0x8          without extern "C" the symbol carries the argument list
 */
extern "C" int fn_plain_c(int x)
{
    return x + 1;
}

int fn_mangled(int x)
{
    return x + 1;
}
