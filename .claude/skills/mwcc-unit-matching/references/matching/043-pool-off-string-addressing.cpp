/* Demo for idea 43: Retail's per-string lis/addi addressing means the unit was built without string pooling
 * FLAGS: -O4,p -inline auto -str reuse
 * MWCC: Wii/1.3
 * EXPECT: absent @stringBase0          no shared string base symbol
 * EXPECT: count lis 2 in greet         one lis/addi pair per string
 * EXPECT: size greet 0x34
 * EXPECT: section .data 0x18           the pool itself is still emitted (two 12-byte strings)
 */
extern "C" {

void puts_(const char *s);

void greet(void)
{
    puts_("hello world");
    puts_("second line");
}

}
