/*
 * MSL_C/alloc.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804578FC..0x804642C8.  Sections of the candidate unit: extab 0x8001E400..0x8001E464; extabindex 0x8003F18C..0x8003F1C8; .text 0x804578FC..0x804642C8; .rodata 0x80572450..0x80573190; .data 0x8060E8E4..0x8060F538; .bss 0x806F4CC8..0x806F5020; .sdata 0x80793CD0..0x80793D00; .sbss 0x80794E00..0x80794E28; .sdata2 0x8079C9A8..0x8079CAB8.
 *
 * WHAT IT IS. a merged MSL library block: the tail of the Gecko exception runtime (`__unexpected`, `ExPPC_ThrowHandler`), the allocator
 *   (`__sys_free`, `Block_link`, `SubBlock_merge_next`, `deallocate_from_fixed_pools`), the file and stdio layer (`__close_all`,
 *   `__flush_all`, `fclose`, `fflush`, `_fseek`), number conversion (`__ull2dec`, `__num2dec_internal`), `memmove`/`memcmp`,
 *   the printf/scanf family (`parse_format`, `__pformatter`, `printf`, `sprintf`, `sscanf`), the string routines (`strcpy`,
 *   `strcmp`, `strchr`, `strstr`), `rand`, `atoi`, `wmemcpy`, `vswprintf`; 128 functions, 67 named.
 *
 * WHY IT SITS HERE. phase 1 grade guess: an unowned run from the registered end of `Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp` (0x804578FC) to
 *   0x804642C8, crossing the window edge 0x80460000 (the window e lane lands the whole unit).  phase1-e.md open question 7: the
 *   unit is several files (the Gecko edge is near 0x80458BD0 and 13 MSL file boundaries are candidates).  The name is the
 *   allocator file the run starts with after the Gecko head.
 *
 * UNKNOWN. every body and the file boundaries.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured; the MSL files may need different flags).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit MSL_C/alloc.cpp`), and the pass that writes the bodies defines them.
 */
