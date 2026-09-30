/* Demo for idea 38: A compound assignment with an operand of the field's own type stores the field raw
 * FLAGS: -O3 -inline noauto -opt nopeephole
 * MWCC: Wii/1.3
 * EXPECT: count extsh 0 in add_compound_s16   s16 field, s16 operand, `+=`: no conversion anywhere
 * EXPECT: size add_compound_s16 0x10
 * EXPECT: count extsh 1 in add_assign_s16     the same sum written `f = f + a`: the store is narrowed first
 * EXPECT: size add_assign_s16 0x14
 * EXPECT: count extsh 1 in add_compound_s32   a wider operand is narrowed to the field type before the add
 * EXPECT: count clrlwi 1 in add_compound_s16_u16field   u16 field, s16 operand: the operand is masked
 * EXPECT: count clrlwi 0 in add_compound_u16_u16field   u16 field, u16 operand: raw
 */
typedef unsigned short u16;
typedef signed short s16;
typedef int s32;

struct Work {
    char pad[10];
    s16 angle;
    u16 flags;
};

extern "C" {

void add_compound_s16(Work *self, s16 amount)
{
    self->angle += amount;
}

void add_assign_s16(Work *self, s16 amount)
{
    self->angle = self->angle + amount;
}

void add_compound_s32(Work *self, s32 amount)
{
    self->angle += amount;
}

void add_compound_s16_u16field(Work *self, s16 amount)
{
    self->flags += amount;
}

void add_compound_u16_u16field(Work *self, u16 amount)
{
    self->flags += amount;
}

}
