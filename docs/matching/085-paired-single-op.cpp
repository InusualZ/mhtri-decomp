/* Demo for idea 85: no spelling of a float fill loop makes the C front end emit a paired-single store
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: absent psq_st                          neither the loop nor the four straight stores is emitted as a paired-single store
 * EXPECT: absent psq_l
 * EXPECT: count stfs 4 in fill_unrolled          a plain scalar `stfs` per element (the target of fn_8009A910 has psq_st there)
 */
extern "C" void fill_loop(float *p, float v, int n)
{
    for (int i = 0; i < n; i++)
        p[i] = v;
}

extern "C" void fill_unrolled(float *p, float v)
{
    p[0] = v;
    p[1] = v;
    p[2] = v;
    p[3] = v;
}
