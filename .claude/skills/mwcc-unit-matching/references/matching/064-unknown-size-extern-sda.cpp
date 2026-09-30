/* Demo for idea 64: An unknown-size `extern` array is addressed absolutely - give it its size for the SDA form
 * FLAGS: -O4,p -inline auto
 * MWCC: Wii/1.3
 * EXPECT: count lis 1 in addr_unknown_size    `extern const char x[];` : lis + addi (absolute)
 * EXPECT: count addi 1 in addr_unknown_size
 * EXPECT: count lis 0 in addr_known_size      `extern const char x[4];` : one `li sym@sda21`
 * EXPECT: insn li r3,0 in addr_known_size
 * EXPECT: reloc lbl_sized
 */
extern "C" {

extern const char lbl_unknown[];
extern const char lbl_sized[4];

const char *addr_unknown_size(void)
{
    return lbl_unknown;
}

const char *addr_known_size(void)
{
    return lbl_sized;
}

}
