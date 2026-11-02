/* Demo for idea 282: An inlined callee's locals are numbered before the webs of the functions inlined inside it: an outer local that must colour last needs the caller's own body
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn addi r3,r31,1 in viaCallee    `enabled` is the inlined callee's local: numbered before `inner`'s web, coloured first, r31
 * EXPECT: insn addi r3,r30,1 in ownBody      the same code with `enabled` as the caller's own local: numbered below `inner`'s web, r30
 */
extern "C" {

int disable(void);
void restore(int level);
void clear(void);
void work(void);

}

static inline void inner(void)
{
    int level;

    level = disable();
    work();
    restore(level);
}

static inline int callee(void)
{
    int enabled;

    enabled = disable();
    clear();
    inner();
    restore(enabled);
    return enabled + 1;
}

int viaCallee(void)
{
    return callee();
}

int ownBody(void)
{
    int enabled;

    enabled = disable();
    clear();
    inner();
    restore(enabled);
    return enabled + 1;
}
