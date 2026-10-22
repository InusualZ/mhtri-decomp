/*
 * lobby/lobby_scene.c - indexes the scene's 0x14C-byte-stride table (base at +0x10) by the argument's byte +0x8 and
 *   tail-calls `pl_obj_ready_ck` with the entry.
 * RANGE. .text 0x801EC9E0-0x801EC9F8 (1 function).  The extent is provisional: one function so far.
 * FLAGS. `cflags_lobby`; its `-O3` is this unit's measurement (docs/lobby.md).
 * NAMES. Module `lobby` is a GUESS from the data, not a symbol: `pl_global` sits in the `.sbss` run of lobby/effect
 *   resource pointers (`lobby_eft_mdl_name_ptr`, `lb_pig_name`, `pl_eft_func`, `set_demo_equip_data_func`), and the
 *   code around it references `lobby_w` and `sysSE_req__Fl`.
 *   GUESS (from each body and its callers): `lb_page_record_empty_ck`.
 * RESIDUALS. none.
 */

extern char *pl_global;
extern void pl_obj_ready_ck(char *entry);

void lb_page_record_empty_ck(unsigned char *obj)
{
    char *table = *(char **)(pl_global + 0x10);

    pl_obj_ready_ck(table + obj[8] * 332);
}
