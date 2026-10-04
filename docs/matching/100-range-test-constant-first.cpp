/* Demo for idea 100: A signed range test keeps its two compares only when the upper bound is written constant-first
 * FLAGS: -O3 -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn cmplwi r3,31 in merged      `x < 0 || x >= 32`: both bounds folded into one unsigned compare
 * EXPECT: insn cmpwi r3,32 in kept         `x < 0 || 32 <= x`: the signed upper-bound compare survives
 * EXPECT: insn cmpwi r3,0 in kept          ... and so does the signed lower-bound one
 */
typedef int s32;

extern "C" {

void bad(s32 x);
void good(s32 x);

void merged(s32 x)
{
    if (x < 0 || x >= 32) {
        bad(x);
    } else {
        good(x);
    }
}

void kept(s32 x)
{
    if (x < 0 || 32 <= x) {
        bad(x);
    } else {
        good(x);
    }
}

}
