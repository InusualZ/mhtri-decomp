/* Demo for idea 43: Two levers behind one shared base register - `-str ...pool` merges strings, `-pool off` un-shares distinct objects
 * FLAGS: -O4,p -inline auto -str reuse,pool -pool off
 * MWCC: Wii/1.3
 * EXPECT: contains @stringBase0        `-str reuse,pool`: the three strings are ONE merged object, so `-pool off` cannot split them
 * EXPECT: count lis 1 in strings3      one lis, then `addi r31,r31,..` / `addi r3,r31,delta`
 * EXPECT: count lis 3 in tables3       `-pool off`: each table is addressed by its own lis/addi pair (default `-pool` on: ONE lis)
 * EXPECT: reloc tabA                  the table is named by its own relocation pair ...
 * EXPECT: absent ...rodata.0           ... and not through the section-relative shared base `-pool` on would use
 * EXPECT: absent ...data.0
 */
extern "C" {

void puts_(const char *s);
int work(int i);

/* Three strings of one section: with `-str reuse,pool` they are one `@stringBase0` object. */
void strings3(void)
{
    puts_("first string here");
    puts_("second string here");
    puts_("third string here");
}

/* Three distinct objects of one section: `-pool` (default on) addresses all three off ONE base register. */
static const int tabA[8] = {1, 2, 3, 4, 5, 6, 7, 8};
static const int tabB[8] = {9, 8, 7, 6, 5, 4, 3, 2};
static const int tabC[8] = {3, 1, 4, 1, 5, 9, 2, 6};

int tables3(int n)
{
    int a = tabA[work(n) & 7];
    int b = tabB[work(a) & 7];
    int c = tabC[work(b) & 7];
    return a + b + c;
}

}
