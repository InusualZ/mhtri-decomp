/* Demo for idea 65: A 32-bit member at an ODD offset needs `#pragma pack(1)`, and the whole unit must be re-measured
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn stw r0,4(r3) in set_plain      unpacked: the u32 after a u8 aligns to +4
 * EXPECT: insn stw r4,1(r3) in set_packed     pack(1): the u32 sits at the odd offset +1 ...
 * EXPECT: insn stw r0,5(r3) in set_packed     ... and the next one at +5
 * EXPECT: insn li r3,8 in sz_plain
 * EXPECT: insn li r3,9 in sz_packed           the pack changes sizeof, so every user of the record moves
 */
typedef unsigned int u32;
typedef unsigned char u8;

struct Plain {
    u8 tag;
    u32 flags;
};

#pragma pack(1)
struct Packed {
    u8 tag;
    u32 flags;
    u32 state;
};
#pragma pack()

extern "C" {

void set_plain(Plain *p)
{
    p->flags = 1;
}

void set_packed(Packed *p)
{
    p->flags = 1;
    p->state = 2;
}

u32 sz_plain(void)
{
    return sizeof(Plain);
}

u32 sz_packed(void)
{
    return sizeof(Packed);
}

}
