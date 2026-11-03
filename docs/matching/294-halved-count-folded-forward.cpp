/* Demo for idea 294: A halved count folded forward takes the pipe base's register: cast the halving into the initialiser and or the word in one nested expression
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: insn or r0,r0,r3 in folded    `reg |= ..` writes the web again: the second or's destination is its first operand
 * EXPECT: insn or r0,r3,r0 in nested    one nested expression: the second or is a fresh temp and takes the normal term's register
 * EXPECT: absent srawi                  the halvings stay unsigned in both spellings (the cast keeps the shift an srwi)
 */
extern "C" {

struct Desc {
    unsigned int lo; /* +0x0 */
    unsigned int hi; /* +0x4 */
    unsigned char hasNrms;   /* +0x8 */
    unsigned char hasBiNrms; /* +0x9 */
};

extern Desc* desc;

union Pipe {
    unsigned char u8; /* +0x0 */
    unsigned int u32; /* +0x0 */
};

#define PIPE (*(volatile Pipe*)0xCC008000)

/* The halvings as separate statements and the word built in two statements. */
void folded(void)
{
    unsigned int nCols;
    unsigned int nNrm;
    unsigned int nTex;
    unsigned int reg;

    nNrm = desc->hasBiNrms ? 2 : (desc->hasNrms != 0);
    nCols = 33 - __cntlzw((desc->lo >> 13) & 0xF);
    nCols /= 2;
    nTex = 33 - __cntlzw(desc->hi & 0xFFFF);
    nTex /= 2;
    PIPE.u8 = 0x10;
    reg = nCols | (nTex << 4);
    reg |= nNrm << 2;
    PIPE.u32 = 0x1008;
    PIPE.u32 = reg;
}

/* The halvings cast into the initialisers and the word as one nested expression. */
void nested(void)
{
    unsigned int nCols;
    unsigned int nNrm;
    unsigned int nTex;
    unsigned int reg;

    nNrm = desc->hasBiNrms ? 2 : (desc->hasNrms != 0);
    nCols = (unsigned int)(33 - __cntlzw((desc->lo >> 13) & 0xF)) / 2;
    nTex = (unsigned int)(33 - __cntlzw(desc->hi & 0xFFFF)) / 2;
    PIPE.u8 = 0x10;
    reg = (nTex << 4) | (nCols | (nNrm << 2));
    PIPE.u32 = 0x1008;
    PIPE.u32 = reg;
}

}
