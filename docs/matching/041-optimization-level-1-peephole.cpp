/* Demo for idea 41: `#pragma optimization_level 1` does not turn the peephole off
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count clrlwi 0 in store_level1    the level moved, the fold is still there (raw store)
 * EXPECT: insn rlwinm r3,r3,2,14,29 in index_level1
 * EXPECT: count clrlwi 1 in store_peep_off  only the peephole pragma keeps the mask
 * EXPECT: seq clrlwi slwi in index_peep_off
 */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct Work {
    char pad[12];
    u8 byte;
};

extern "C" {

#pragma optimization_level 1
void store_level1(Work *self, u32 value)
{
    self->byte = value;
}

u32 index_level1(u16 i)
{
    return (u32)i << 2;
}
#pragma optimization_level reset

#pragma peephole off
void store_peep_off(Work *self, u32 value)
{
    self->byte = value;
}

u32 index_peep_off(u16 i)
{
    return (u32)i << 2;
}
#pragma peephole reset

}
