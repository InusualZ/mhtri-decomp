/* Demo for idea 29: A claimed literal pool: declare the constants, never define them
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: reloc lbl_pool_const          the `extern` constant is loaded through the map's own symbol (`lfs f0,lbl@sda21`)
 * EXPECT: section .sdata2 4              only the literal 2.5f made a pool entry of our own: the extern added none
 * EXPECT: size use_extern 0xc
 * EXPECT: size use_literal 0xc           same code size; the difference is which symbol the load names
 */
extern "C" {

extern float lbl_pool_const;

float use_extern(float x)
{
    return x * lbl_pool_const;
}

float use_literal(float x)
{
    return x * 2.5f;
}

}
