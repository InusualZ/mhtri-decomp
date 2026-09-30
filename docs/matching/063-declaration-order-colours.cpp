/* Demo for idea 63: A local's DECLARATION ORDER colours registers - locals are coloured before parameters
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn lwz r31,0(r4) in decl_first    `messages` declared first: it gets r31, the loop counter r30
 * EXPECT: insn lwz r30,0(r4) in decl_last     declared second: the colours mirror (messages r30, counter r31)
 * EXPECT: insn mr r29,r3 in decl_first        the parameter is coloured after every local: r29 in both spellings
 * EXPECT: insn mr r29,r3 in decl_last
 */
typedef unsigned int u32;
typedef int s32;

extern "C" {

extern const char *table[];
void use(const char *a, u32 b, s32 c);

void decl_first(u32 group)
{
    const char *messages = table[0];
    s32 i;
    for (i = 0; i < 3; i++)
        use(messages, group, i);
}

void decl_last(u32 group)
{
    s32 i;
    const char *messages = table[0];
    for (i = 0; i < 3; i++)
        use(messages, group, i);
}

}
