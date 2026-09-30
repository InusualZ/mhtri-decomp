/* Demo for idea 20: Force a loop-invariant address through a `u32` local
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn lwz r3,84(r29) in folded     the field address is folded into the load displacement
 * EXPECT: insn lwz r3,84(r29) in via_u32     ... and routing it through a `u32` buf[1] does NOT prevent that here
 * EXPECT: insn lwz r3,84(r29) in via_scalar  ... nor does a plain `u32` local
 * EXPECT: size folded 0x58                   all four spellings compile to the same size
 * EXPECT: size via_u32 0x58
 * EXPECT: size via_scalar 0x58
 * EXPECT: size via_ptr 0x58
 * EXPECT: size via_buf_loop 0x50             the `while (i-- > 0)` pair is identical too
 * EXPECT: size folded_loop 0x50
 */
typedef unsigned int u32;

struct Module {
    int pad[21];
    int table[4]; /* +0x54 */
};

extern "C" {

void consume(int v);

void folded(Module *m, int count)
{
    for (int i = 0; i < count; i++)
        consume(m->table[0]);
}

void via_u32(Module *m, int count)
{
    u32 buf[1];
    buf[0] = (u32)m + 0x54;
    for (int i = 0; i < count; i++)
        consume(*(int *)buf[0]);
}

void via_scalar(Module *m, int count)
{
    u32 addr = (u32)m + 0x54;
    for (int i = 0; i < count; i++)
        consume(*(int *)addr);
}

void via_ptr(Module *m, int count)
{
    int *q = &m->table[0];
    for (int i = 0; i < count; i++)
        consume(*q);
}

void via_buf_loop(Module *m, int count)
{
    u32 buf[1];
    int i;
    buf[0] = (u32)m + 0x54;
    i = count;
    while (i-- > 0)
        consume(*(int *)buf[0] + i);
}

void folded_loop(Module *m, int count)
{
    int i = count;
    while (i-- > 0)
        consume(m->table[0] + i);
}

}
