/* Demo for idea 58: A compiler-synthesised pool entry can be claimed only while your unit is its sole referencer
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: contains lfd                     an int->double conversion loads the 0x4330000080000000 magic from the pool
 * EXPECT: section .sdata2 16               8 B for the synthesised magic (an anonymous @N entry) + 8 B for the named constant below
 * EXPECT: contains lbl_8079A008            naming the constant in source does not REMOVE the pool copy, it adds a second one
 * EXPECT: size lbl_8079A008 8
 */
extern "C" {

extern const double lbl_8079A008 = 4503601774854144.0;

double to_double(int n)
{
    return (double)n;
}

}
