/* Demo for idea 38: An `s16` parameter with a compound assignment is what makes a field store raw
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains sth                        both spellings store the field raw
 * EXPECT: count clrlwi 0 in store_plain_s32   s32 parameter, plain assignment: no mask anywhere
 * EXPECT: count clrlwi 1 in store_compound_s16   s16 parameter, compound assignment: the operand is narrowed
 * EXPECT: size store_plain_s32 0x10
 * EXPECT: size store_compound_s16 0x14
 */
typedef unsigned short u16;
typedef signed short s16;
typedef int s32;

struct Work {
    char pad[10];
    u16 field;
};

extern "C" {

void store_plain_s32(Work *self, s32 amount)
{
    self->field = self->field + amount;
}

void store_compound_s16(Work *self, s16 amount)
{
    self->field += amount;
}

}
