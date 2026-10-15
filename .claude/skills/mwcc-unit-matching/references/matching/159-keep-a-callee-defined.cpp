/* Demo for idea 131: Keep a callee defined ahead of its caller out of line: auto_inline off around the callee's definition
 * FLAGS: -O4,p -inline auto -lang c
 * MWCC: Wii/1.3
 * EXPECT: count bl 3 in callerFolded   -inline auto folds a callee defined ahead of its caller (three `other` calls)
 * EXPECT: count bl 2 in callerKept     auto_inline off around the definition keeps the caller's call
 * EXPECT: count cmpwi 2 in bigKept     ... while the callee's own static inline still folds into it
 * EXPECT: count cmpwi 1 in bigDont     dont_inline around the definition keeps the call but stops that fold too
 * EXPECT: reloc helper                 (bigDont calls the static inline out of line)
 */
typedef int s32;
typedef struct Conn { s32 state; s32 count; s32 result; } Conn;

void other(Conn* c, s32 v);

static inline s32 helper(Conn* c, s32 v) {
    if (c->state < 5) {
        other(c, v);
        c->result = v;
        return 0;
    }
    c->count++;
    return 1;
}

static s32 bigFolded(Conn* c, s32 v) {
    if (!helper(c, v)) {
        return 0;
    }
    other(c, c->count);
    return 1;
}

s32 callerFolded(Conn* c) {
    other(c, 0);
    return bigFolded(c, 7) != 0;
}

#pragma push
#pragma auto_inline off
s32 bigKept(Conn* c, s32 v) {
    if (!helper(c, v)) {
        return 0;
    }
    other(c, c->count);
    return 1;
}
#pragma pop

s32 callerKept(Conn* c) {
    other(c, 0);
    return bigKept(c, 7) != 0;
}

#pragma push
#pragma dont_inline on
s32 bigDont(Conn* c, s32 v) {
    if (!helper(c, v)) {
        return 0;
    }
    other(c, c->count);
    return 1;
}
#pragma pop
