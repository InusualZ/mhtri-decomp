/* Demo for idea 61: A kept `bl` inside one function: scope `#pragma dont_inline on` to it
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count bl 1 in folded         -inline auto folds the static helper into the case: only the `other` call is left
 * EXPECT: count bl 2 in kept           inside the pragma pair the helper stays a `bl`
 * EXPECT: count bl 1 in after          after `dont_inline off` the folding is back
 * EXPECT: reloc reset
 */
typedef int s32;
struct N { s32 state; s32 a; s32 b; s32 c; };

extern "C" {

void other(N *);

static void reset(N *n)
{
    n->a = 0;
    n->b = 0;
    n->c = 0;
    n->state = 1;
}

s32 folded(N *n)
{
    switch (n->state) {
    case 255:
        reset(n);
        other(n);
        return 1;
    default:
        break;
    }
    return 0;
}

#pragma dont_inline on
s32 kept(N *n)
{
    switch (n->state) {
    case 255:
        reset(n);
        other(n);
        return 1;
    default:
        break;
    }
    return 0;
}
#pragma dont_inline off

s32 after(N *n)
{
    switch (n->state) {
    case 255:
        reset(n);
        other(n);
        return 1;
    default:
        break;
    }
    return 0;
}

}
