/* Demo for idea 18: Named temporaries, declaration order and operand order steer the allocator
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn cmplw r0,r4 in cmp_field_first   `p->hash == hash`: field register first
 * EXPECT: insn cmplw r4,r0 in cmp_param_first   `hash == p->hash`: operands swapped, same meaning
 * EXPECT: insn cmplw r3,r4 in less_ab           `a < b`
 * EXPECT: insn cmplw r4,r3 in less_ba           `b > a` is the same test with the operands swapped
 * EXPECT: count lwz 4 in named_temp             a named local is loaded once, into its own callee-saved register (r30)
 * EXPECT: count lwz 5 in unnamed_temp           the unnamed expression is loaded twice: a different colouring
 */
typedef unsigned int u32;

struct Entry {
    u32 hash;
    u32 name_offset;
};

extern "C" {

int report(int v);

int cmp_field_first(const Entry *p, u32 hash)
{
    if (p->hash == hash)
        return report(1);
    return 0;
}

int cmp_param_first(const Entry *p, u32 hash)
{
    if (hash == p->hash)
        return report(1);
    return 0;
}

int less_ab(u32 a, u32 b)
{
    if (a < b)
        return report(1);
    return 0;
}

int less_ba(u32 a, u32 b)
{
    if (b > a)
        return report(1);
    return 0;
}

int named_temp(const Entry *p)
{
    u32 no = p->name_offset;
    return report(no) + report(no + 1);
}

int unnamed_temp(const Entry *p)
{
    return report(p->name_offset) + report(p->name_offset + 1);
}

}
