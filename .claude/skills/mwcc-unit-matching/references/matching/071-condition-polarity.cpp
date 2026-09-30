/* Demo for idea 71: The condition's POLARITY decides the exit - a ternary merges arms where if/else does not
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count cntlzw 0 in same_direct       `if (memcmp(...) == 0)` is cmpwi + bne
 * EXPECT: insn cmpwi r3,0 in same_direct
 * EXPECT: count cntlzw 1 in same_assigned     an assigned boolean is cntlzw + srwi.
 * EXPECT: insn srwi. r0,r0,5 in same_assigned
 * EXPECT: size merge_ternary 0x18             ternary and if/else merge identically in isolation
 * EXPECT: size merge_ifelse 0x18
 */
typedef unsigned char u8;
typedef int s32;

extern "C" {

int memcmp(const void *a, const void *b, unsigned n);
void act(void);
int send(int x);

int same_direct(const u8 *a, const u8 *b, int x)
{
    if (memcmp(a, b, 4) == 0) {
        act();
        return x;
    }
    return 0;
}

int same_assigned(const u8 *a, const u8 *b, int x)
{
    s32 same = (memcmp(a, b, 4) == 0);
    if (same) {
        act();
        return x;
    }
    return 0;
}

int merge_ternary(int c, int x)
{
    int ok = c ? send(x) : 0;
    return ok;
}

int merge_ifelse(int c, int x)
{
    int ok;
    if (c)
        ok = send(x);
    else
        ok = 0;
    return ok;
}

}
