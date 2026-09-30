/* Demo for idea 45: `\n` in a string literal is LF - the claimed CRLF translation does not happen
 * FLAGS: -O4,p -inline auto -str reuse
 * MWCC: Wii/1.3
 * EXPECT: bytes .sdata 610a620a00     "a\nb\n" is emitted with LF bytes (61 0a 62 0a 00)
 * EXPECT: nobytes .sdata 610d0a       no CR is inserted before the LF
 * EXPECT: bytes .sdata 780d0a7900     an explicit "\r\n" does emit 0d 0a, so the check can see a CR
 */
extern "C" {

const char *newline_string(void)
{
    return "a\nb\n";
}

const char *explicit_crlf(void)
{
    return "x\r\ny";
}

}
