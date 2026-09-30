/* Demo for idea 66: A narrow RETURN TYPE is visible at the caller
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count clrlwi 1 in forward_wide      callee returns s32, the caller narrows it to u16: a mask
 * EXPECT: count clrlwi 0 in forward_narrow    callee returns u16 already: no mask, a plain tail call
 * EXPECT: size forward_narrow 4
 * EXPECT: count clrlwi 0 in store_wide        the STORE of either result is a raw sth: the mask is not at the store ...
 * EXPECT: count clrlwi 0 in store_narrow
 * EXPECT: count clrlwi 1 in use_wide          ... it appears where the value is USED again (t + 1) when a wide callee fed a u16
 * EXPECT: count clrlwi 0 in use_narrow        a u16 callee feeding a u16 costs nothing
 * EXPECT: count clrlwi 1 in use_narrow_s32    a u16 callee read into an s32 is masked again (return type and use disagree)
 */
typedef unsigned short u16;
typedef int s32;

struct S {
    u16 f;
    s32 w;
    u16 g;
};

extern "C" {

s32 flag_wide(void);
u16 flag_narrow(void);

u16 forward_wide(void)
{
    return flag_wide();
}

u16 forward_narrow(void)
{
    return flag_narrow();
}

void store_wide(S *s)
{
    s->f = flag_wide();
}

void store_narrow(S *s)
{
    s->f = flag_narrow();
}

void use_wide(S *s)
{
    u16 t = flag_wide();
    s->f = t;
    s->g = t + 1;
}

void use_narrow(S *s)
{
    u16 t = flag_narrow();
    s->f = t;
    s->g = t + 1;
}

void use_narrow_s32(S *s)
{
    s32 t = flag_narrow();
    s->f = t;
    s->g = t + 1;
}

}
