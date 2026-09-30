/* Demo for idea 44: A string pool in `.data` means the build was not `-str readonly`
 * FLAGS: -O4,p -inline auto -str reuse,pool
 * MWCC: Wii/1.3
 * EXPECT: section .data 0x18      the strings sit in .data (adding `readonly` moves them to .rodata)
 * EXPECT: section .rodata 0
 * EXPECT: contains @stringBase0   `pool` puts them behind one shared base symbol
 */
extern "C" {

void puts_(const char *s);

void greet(void)
{
    puts_("hello world");
    puts_("second line");
}

}
