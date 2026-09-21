/*
 * Lobby/player-scene code: index a 0x14C-byte-stride table from the scene object and tail-call the handler.
 *
 * .text 0x801EC9E0-0x801EC9F8 - one function, fn_801EC9E0, 6 instructions and 0x18 bytes: read the scene
 * pointer out of a small-data global, take the table base at +0x10, index it by byte +0x8 of the argument
 * and tail-call fn_80269394. No frame, one SDA21 and one REL24 relocation.
 * The module is named from the data the function touches, not from a symbol name: the global it reads,
 * lbl_80794B28, sits in the .sbss run of lobby/effect resource pointers (lobby_eft_mdl_name_ptr,
 * lb_pig_name, pl_eft_func, set_demo_equip_data_func), and the code around it references lobby_w (.bss) and
 * the mangled sysSE_req__Fl. So src/lobby/ is a proposal, not a fact - rename it freely.
 * The range is provisional: one function so far.
 * Registered NonMatching in configure.py, lib lobby (Wii/1.3, cflags_lobby); the two-variant probe said so:
 * -O3 reproduces the retail window byte-for-byte while -O4,p hoists the byte load and splits the base
 * across r3. The cross-unit .sbss reference keeps its SDA21 form (verified in the relocation table).
 * Measured: fuzzy_match_percent 100.0 - 24 B / 6 instructions, byte-identical.
 * Residual: none.
 */

extern char *lbl_80794B28;
extern void fn_80269394(char *entry);

void fn_801EC9E0(unsigned char *obj)
{
    char *table = *(char **)(lbl_80794B28 + 0x10);

    fn_80269394(table + obj[8] * 332);
}
