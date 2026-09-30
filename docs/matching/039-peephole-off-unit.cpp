/* Demo for idea 39: The peephole pass folds narrowing masks; `#pragma peephole off` keeps them unfused
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count clrlwi 0 in store_peep_on    the store's own mask is folded into `stb`
 * EXPECT: size store_peep_on 0x8
 * EXPECT: count clrlwi 1 in store_peep_off   unfused: mask, then store
 * EXPECT: size store_peep_off 0xc
 * EXPECT: insn rlwinm r3,r3,2,14,29 in index_peep_on   `clrlwi` + `slwi` fused into one rlwinm
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

void store_peep_on(Work *self, u32 value)
{
    self->byte = value;
}

u32 index_peep_on(u16 i)
{
    return (u32)i << 2;
}

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
