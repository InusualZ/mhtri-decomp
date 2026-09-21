/*
 * NetworkWiiMediator: the output-parameter accessors its state queries use.
 *
 * .text 0x80413F3C-0x80413F50 - one function, fn_80413F3C, 5 instructions: hand back the sub-object at
 * +0x78D and the constant 0x400 through the two output pointers. No frame, no callee-saved register, no
 * relocation.
 * The range is provisional: the surrounding functions are unsplit (fn_80413F30 before it, fn_80413F50
 * after), and the module comes from the C++ names around it - the mangled NetworkWiiMediator methods
 * reflectInit/reflectStart/reflectStop sit at 0x8041416C and getLanguage at 0x80413C40. This symbol is
 * unmangled, so the file stays C until more of the TU is reconstructed.
 * Registered NonMatching in configure.py, lib Network (Wii/1.3, cflags_base).
 * Measured: fuzzy_match_percent 100.0 - 20 B / 5 instructions, every instruction equal, no relocation.
 * Residual: none.
 */

void fn_80413F3C(char *self, char **subobject, unsigned int *limit)
{
    *subobject = self + 0x78D;
    *limit = 0x400;
}
