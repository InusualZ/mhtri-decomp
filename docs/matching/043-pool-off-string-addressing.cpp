/* Demo for idea 43: `-pool off` does not remove @stringBase0 - dropping `pool` from `-str` does
 * FLAGS: -O4,p -inline auto -str reuse,pool -pool off
 * MWCC: Wii/1.3
 * EXPECT: contains @stringBase0        `-pool off` is on the line, the shared string base is still there
 * EXPECT: count lis 1 in greet         one lis for both strings (compile with `-str reuse` alone and it is two lis + two addi)
 * EXPECT: size greet 0x3c
 * EXPECT: section .data 0x18           the two strings' pool
 */
extern "C" {

void puts_(const char *s);

void greet(void)
{
    puts_("hello world");
    puts_("second line");
}

}
