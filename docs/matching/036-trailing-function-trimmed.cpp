/* Demo for idea 36: `__declspec(export)` sets the per-symbol "force active" flag in `.comment`
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: bytes .comment 0000001000000000   plain_fn's `.comment` entry: active flags 0x00 (the linker may trim it)
 * EXPECT: bytes .comment 0000001000080000   exported_fn's entry: byte 5 = 0x08, the linker keeps it
 * EXPECT: size plain_fn 0xc                 the code itself is the same either way
 * EXPECT: size exported_fn 0xc
 */
extern "C" {

void plain_fn(int *p)
{
    *p = 1;
}

__declspec(export) void exported_fn(int *p)
{
    *p = 2;
}

}
