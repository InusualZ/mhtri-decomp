/* Demo for idea 26: The target's section is part of the match
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: section .init 0xc            `__declspec(section ".init")` puts the function in the target's section
 * EXPECT: section .text 0xc            the unmarked function stays in .text: identical bytes, different section
 * EXPECT: section .mwcats.init 0       no .mwcats.<section> with the project's flags (`-pragma cats off`)
 */
extern "C" {

void plain_text(char *p)
{
    *p = 0;
}

__declspec(section ".init") void placed_in_init(char *p)
{
    *p = 0;
}

}
