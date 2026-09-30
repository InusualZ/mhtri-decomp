/* Demo for idea 71: The condition's POLARITY decides the exit; an assigned boolean gives cntlzw/srwi.
 * FLAGS: -O4,p -inline auto -func_align 4
 * MWCC: Wii/1.3
 * EXPECT: count cntlzw 0 in same_direct       `if (memcmp(...) == 0)` is cmpwi + bne
 * EXPECT: insn cmpwi r3,0 in same_direct
 * EXPECT: count cntlzw 0 in same_not          `if (!memcmp(...))` compiles like `== 0`
 * EXPECT: count cntlzw 1 in same_assigned     an assigned boolean is cntlzw + srwi.
 * EXPECT: insn srwi. r0,r0,5 in same_assigned
 * EXPECT: size merge_ternary 0x74             ternary and if/else merge identically (no lever of their own) ...
 * EXPECT: size merge_ifelse 0x74
 * EXPECT: size exit_single 0x74               `if (ok == 0) {...}` + trailing return: one bne ...
 * EXPECT: size exit_early 0x78                `if (ok != 0) return ok;` adds a `b` (beq + b instead of one bne)
 */
typedef unsigned int u32;
typedef unsigned char u8;
typedef int s32;

extern "C" {

int memcmp(const void *a, const void *b, unsigned n);
void act(void);
u32 tick(void);
u32 poll(int s);
void fin(void);
extern u32 last;

int same_direct(const u8 *a, const u8 *b, int x)
{
    if (memcmp(a, b, 4) == 0) {
        act();
        return x;
    }
    return 0;
}

int same_not(const u8 *a, const u8 *b, int x)
{
    if (!memcmp(a, b, 4)) {
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

u32 merge_ternary(s32 sock)
{
    u32 ok = 1;
    if (sock != -1) {
        ok = ((u32)(tick() - last) < 10000) ? poll(sock) : 0;
        if (ok == 0) {
            fin();
        }
    }
    return ok;
}

u32 merge_ifelse(s32 sock)
{
    u32 ok = 1;
    if (sock != -1) {
        if ((u32)(tick() - last) < 10000)
            ok = poll(sock);
        else
            ok = 0;
        if (ok == 0) {
            fin();
        }
    }
    return ok;
}

u32 exit_single(s32 sock)
{
    u32 ok = 1;
    if (sock != -1) {
        ok = ((u32)(tick() - last) < 10000) ? poll(sock) : 0;
        if (ok == 0) {
            fin();
        }
    }
    return ok;
}

u32 exit_early(s32 sock)
{
    u32 ok = 1;
    if (sock != -1) {
        ok = ((u32)(tick() - last) < 10000) ? poll(sock) : 0;
        if (ok != 0)
            return ok;
        fin();
    }
    return ok;
}

}
