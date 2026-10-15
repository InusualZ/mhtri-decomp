/* Demo for idea 132: A void function's two-condition early exit is one || return, not two returns
 * FLAGS: -O4,p -inline auto -lang c
 * MWCC: Wii/1.3
 * EXPECT: seq beq bne b bl in orReturn       one `||` return: beq to the exit, bne into the body, b to the exit
 * EXPECT: count b 0 in twoReturns           two early returns: two bne/beq to the exit, no unconditional branch
 * EXPECT: size orReturn 100
 * EXPECT: size twoReturns 96
 */
typedef int s32;
typedef unsigned char u8;
typedef struct Buf { u8* data; s32 size; s32 len; } Buf;

void copy(u8* dst, const u8* src, s32 len);

void orReturn(Buf* b, const u8* src, s32 len) {
    if (src == 0 || len == 0) {
        return;
    }
    copy(b->data + b->len, src, len);
    b->len += len;
}

void twoReturns(Buf* b, const u8* src, s32 len) {
    if (src == 0) {
        return;
    }
    if (len == 0) {
        return;
    }
    copy(b->data + b->len, src, len);
    b->len += len;
}
