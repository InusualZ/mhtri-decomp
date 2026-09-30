/* Demo for idea 3: Read the target's disassembly, not just the diff
 * FLAGS: -O4,p -inline auto -use_lmw_stmw off
 * MWCC: Wii/1.3
 * EXPECT: contains _savegpr_14   off: the prologue saves r14.. through the EABI helper (the md's 'target' line)
 * EXPECT: contains _restgpr_14
 * EXPECT: absent stmw             ... and there is no stmw; with `-use_lmw_stmw on` the same source saves with stmw/lmw
 * EXPECT: absent lmw
 */
extern "C" {

int use(int);

/* enough live values across the calls to need r14..r31 */
int many(int a, int b, int c, int d, int e, int f, int g, int h)
{
    int r14 = use(a), r15 = use(b), r16 = use(c), r17 = use(d), r18 = use(e), r19 = use(f);
    int r20 = use(g), r21 = use(h), r22 = use(a + b), r23 = use(c + d), r24 = use(e + f);
    int r25 = use(g + h), r26 = use(a + c), r27 = use(b + d), r28 = use(e + g), r29 = use(f + h);
    int r30 = use(a + e), r31 = use(b + f);
    return r14 + r15 + r16 + r17 + r18 + r19 + r20 + r21 + r22 + r23 + r24 + r25 + r26 + r27 + r28
         + r29 + r30 + r31;
}

}
