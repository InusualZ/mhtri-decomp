/* Demo for idea 270: Under global_optimizer off a block-scope local forces a frame pointer: declare every local at function scope
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn mr r31,r1 in blockLocal    the block-scope `level` makes the function keep a frame pointer
 * EXPECT: count mr 2 in functionLocal     the same body with `level` at function scope: only the two argument moves
 */
extern "C" {

void take(int* p);
void print(const char* format, ...);
int get(void);

}

#pragma push
#pragma global_optimizer off
void blockLocal(int k)
{
    if (k) {
        unsigned int level = get();

        take(0);
        print("%d", level);
    }
}

void functionLocal(int k)
{
    unsigned int level;

    if (k) {
        level = get();
        take(0);
        print("%d", level);
    }
}
#pragma pop
