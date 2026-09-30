/* Demo for idea 27: Same instructions, different order names the `-O` level - probe both, per unit
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: section .text 0x60           `-O4,p` implies 16-byte function packing: 3 functions of 8 B land at 0x0/0x10/0x20
 * EXPECT: order .text second_two < third_three
 * EXPECT: size call_then_load 0x30      the same 12 instructions under -O3 (verified: .text 0x48, unaligned): only the packing differs
 * EXPECT: insn lwz r3,8(r31) in call_then_load   the reload sits before the LR reload at both `-O` levels in this shape
 */
extern "C" {

int first_one(void)
{
    return 1;
}

int second_two(void)
{
    return 2;
}

int third_three(void)
{
    return 3;
}

struct Node { int a; int b; int c; };

int call_then_load(Node *n)
{
    extern void poke(Node *);
    poke(n);
    return n->c;
}

}
