/*
 * The game's root translation unit: main(), the pre-main latches, and the init/video/mode helpers.
 *
 * .text 0x8003F200-0x80040478 - 47 functions, 4728 B, one file. `__start` calls `main` (0x8003F218, the DOL
 * entry point); the first 24 B are the two pre-main latches `fn_8003F200`/`fn_8003F20C` that set the bytes
 * main polls (0x807947A5/A6); then the GQR/VI/TV setup, the memory-heap creation, the wide-mode /
 * brightness / screen-size accessors, and finally the `Screen_w` flag/rectangle accessors and the two
 * expansion-heap wrappers that sys_mem.cpp's `operator new`/`operator delete` call.
 *
 * Batch 4 extended the split from the 0x80040360 cut to the real seam 0x80040478 (`tudiscover`'s boundary,
 * commit 845a6d8): the `ck_WideMode`/`Screen_w` group belongs to this file, and 0x80040478 is where
 * sys_mem.cpp's `__nw__FUl` starts.
 *
 * Attribution (batch 2, step 1): `tudiscover at 0x8003F218` calls the whole range one TU (two must-link
 * anchors); main.cpp is first in link order, so its `.data` pool at 0x8057C820 is the first fragment of
 * `.data` and `OSPanic` in `fn_8003F730` names `__FILE__` from that pool, which no other file in the range
 * does; and the `.sdata2` label set 0x80795AA0-0x80795AD8 is shared across `main` <-> `fn_8003F9E4` <->
 * `fn_8004030C`, which a per-TU literal pool cannot be split across. The runtime dump names the untitled
 * ones: `fn_8003F620` = create_memory_heaps, `fn_8003F728` = mem_create_exp_heap, `fn_8003F730` = render
 * init, `fn_8003F940` = GXRenderModeObj copy, `fn_8003F9E4` = TV-mode re-init, `fn_8003FCCC` = GX/VI setup,
 * `fn_804E7110` = AIRegisterDMACallback.
 *
 * Identified but **not claimed** yet: the unit's remaining data fragments (`.sbss`, `.sdata`, `.data`,
 * `.bss`). A split has to be section-aligned
 * (`.sbss`/`.data` are 16/32) and the retail fragment ends are not visible from the symbol map, so they are
 * claimed later, with a measurement (playbook 23) - the run-time globals `set_widemode_flag` (.sbss
 * 0x80794790), `set_widemode_param` (.sdata 0x80790E21), `system_w` (0x806585E0) and `Screen_w`
 * (0x8065903C), the `.data` pool at 0x8057C820, `.sdata2` 0x80795AA0-0x80795AD8, and the unwind pair
 * `@etb_800066E0` / `@eti_8001E480`. The `.text` and `.sdata2` ranges are claimed in `splits.txt` now, so
 * this file only *declares* the pool labels; the constants MWCC synthesises for the int->float conversions
 * still live in our object's own pool under `@NN` names, so each `lfd` that retail points at
 * `lbl_80795AC0`/`lbl_80795AD0` is one ARG row (playbook 23; four sites: `main`, `fn_8003FEBC`,
 * `fn_8003FF98`, `fn_8004030C`).
 *
 * Flags - measured this batch against `build/RMHE08/obj/main.o`, all of it per-library for `main` (never
 * `cflags_base`):
 *   * `-O4,p` -> `-O3`. Decisive: `-O4,p` gives `main` 71.12 %, `fn_8003FC64` 65.62 %,
 *     `change_widemode_req__FUc` 58.00 %, `fn_8003F58C` 79.78 %; `-O3` gives 96.70 / 99.58 / 100 / 90.81.
 *   * **no `-func_align` flag is needed**: `-O3` already packs functions on 4 B, so the target's starts
 *     (`fn_8003F20C` at .text+0xC, `main` at +0x18, `SetSystemVcnt__Fl` at +0x30C) come out right. The
 *     `-O4,p` alternative would have needed `-func_align 4`, since `-O4,p` implies `-func_align 16`
 *     (+0x10/+0x20 there) - one more reason `-O3` is the retail setting.
 *   * `-inline off` -> **`-inline noauto`** (or plain `-inline on`, this compiler's default). With
 *     `-inline auto` (cflags_base's) the retail call to `fn_8003F554` inside `fn_8003F52C`/`fn_8003F564` is
 *     inlined (74.00 % on both, 48 B vs 40 B) and `change_widemode_req__FUc` is inlined into
 *     `change_widemode_req_default__Fv` (21.18 %); `-inline off` *and* `-inline noauto` both put them back at
 *     100 %. The two are not equivalent for `fn_8003F940`: its body is the retail aggregate
 *     `GXRenderModeObj` copy, which MWCC lowers to a call to the implicitly-*inline* copy-assignment
 *     operator under `-inline off` (4 B `bl`, 0.98 %) and inlines under `-inline noauto` (164 B, 100.00 %).
 *     Measured on the whole unit, `-inline noauto` moves nothing else (main 96.70, fn_8003F58C 90.81,
 *     fn_8003FC64 99.58, every other function unchanged), so `cflags_main` carries `-inline noauto` and
 *     `fn_8003F940` is at 100 %.
 *   * `-use_lmw_stmw` stays **off**: the target's 4728 B `.text` has no `lmw`/`stmw` at all (objdump), and
 *     `-use_lmw_stmw on` measures byte-identical to off.
 *
 * Load-bearing source shapes:
 *   * `.text` is the plain default section - no pragma here, unlike the `.init` units.
 *   * the unnamed `fn_*` entries are `extern "C"`: their map names are unmangled while the file is C++
 *     (`SetSystemVcnt__Fl`, `change_widemode_req__FUc`, `get_tv_mode__Fv`, `hbm_InitGX__Fv` are the mangled
 *     ones and are written by their *source* names so the compiler emits the map's spelling), so a plain C++
 *     definition of `fn_8003F200` would emit `fn_8003F200__Fv` and objdiff would pair nothing.
 *   * `#pragma peephole off` is scoped, and scoped per statement range, because the peephole is what makes
 *     three different retail shapes disappear: it deletes the dead sign-extend of `change_widemode_req`'s
 *     byte parameter, fuses that function's `srwi`+`clrlwi` into one `rlwinm`, and (the same fusion) turns
 *     `clrlwi r0,r0,31; cmpwi r0,0` into the record form `clrlwi.` in `fn_8003FE30`. It also strips the
 *     `clrlwi r0,r3,24` that retail keeps in front of `stb` in `fn_8003F730` and at the head of
 *   * the unit's extab/extabindex are emitted by the lib's -Cpp_exceptions on: the target carries extab 0x90 +
 *     extabindex 0xD8 (18 unwind-only records, one per framed function) and our object emitted none. With the
 *     flag both sections equal the target's exactly and no function's .text moves.
 *     `fn_8003F9E4` - while the rest of the unit needs the peephole *on* (it is what removes `main`'s
 *     redundant `clrlwi`s in front of the two `sth r0,0x3e(r4)`; with the whole-unit `-opt nopeephole` those
 *     two appear and `main` drops to 94.49 %). `fn_8003F9E4` shows why the reset can sit *inside* a function:
 *     its head needs the truncation kept (peephole off) but the `Screen_w` tail and the trailing
 *     `stb r31, lbl_80794791` need it removed (peephole on), and both halves measure exactly when the reset
 *     is placed between them (85.70 % with the pragma over the whole body, 95.73 % with the mid-function
 *     reset). `#pragma peephole off` is the only spelling this compiler honours (`opt_peephole`/`peep` parse
 *     but do nothing).
 *   * `main`'s loop shape - four per-frame calls, three `system_w` byte polls, the two latch polls, the three
 *     function-pointer calls through `system_w`, and the two trailing `if`s - is what puts every `continue`
 *     on the loop head and keeps `&system_w` and the constant `1` in r31/r30.
 *   * `lbl_8079479C` is a **signed** 32-bit global: retail converts it with the `xoris`/`0x4330000080000000`
 *     (signed) idiom, not the `0x4330000000000000` (unsigned) one - MWCC emits them the opposite way round
 *     from the obvious guess, so `u32` there costs 2 instructions and `lfd`s the wrong constant. The same
 *     holds for `lbl_807947A0` and for `fn_8003F524`'s return: the two frame-wait loops compare with `cmpw`
 *     (signed), and declaring them `u32` costs `cmplw` + a REPLACE row (`fn_80040144` 92.97 -> 100.00 %).
 *   * the TV-standard chains in `fn_8003F730`/`fn_8003F9E4` are `switch`es, not `if`/`else if` chains: MWCC
 *     lays a switch out as all the tests first (`beq` off to each case block) and the blocks after them,
 *     while an `if`/`else if` chain inlines the first block right after its test. That one shape is worth
 *     `fn_8003F730` 89.36 -> 96.25 % and `fn_8003F9E4` 85.70 -> 92.75 %.
 *   * a value that has to survive a call lives in a *named* local, because the compiler then cannot
 *     rematerialise it into a volatile register: `GXRenderModeObj* mode = Rmode;` in `fn_8003FEBC`
 *     (92.00 -> 98.00 %) and `u32 ticks = (OS_BUS_CLOCK >> 2) / 1000;` inside `fn_80040144`'s wait loops
 *     (argument-setup order: retail computes the tick count before the `li r3,0`).
 *   * `fn_8003F620`'s second framebuffer is written as `lbl_80794770[1] = (u32)lbl_80794770[0] + 0xA5000;`
 *     - reading the base back through the global keeps it in a register and gives retail's
 *     `addis`+`addi` delta pair; folding the two literals costs 4 ARG rows (98.92 -> 100.00 %).
 *   * `OSSetPeriodicAlarm` takes 7 register arguments in this SDK (the dump's own body reads `alarm[6]`,
 *     `alarm[7]` and a 64-bit time from the middle pair), so it is declared here as
 *     `(void* alarm, u32 unk, u64 time, u32 arg5, u32 arg6, void* handler)`; the retail call is
 *     `(&alarm, 0xF0000, OSGetTime(), 0, 0xF7314, fn_8003F52C)`.
 *
 * All 47 functions are written (address order, .text 0x8003F200-0x80040478): F200, F20C, F218(main), F4D8,
 * F50C, F524, F52C, F554, F564, F58C, F620, F728, F730, F940, F9E4, FBE8, FBFC, FC04, FC48, FC50, FC58,
 * FC5C, FC64, FCC4, FCCC, FE20, FE24, FE30, FEBC, FF98, 40144, 4026C, 40274, 40280, 4028C, 4029C,
 * get_ScreenSize, 4030C, 40360, 403AC, ck_WideMode, 403DC, 403F8, 40414, 4041C, 40420, 40460. Nothing is
 * left to add: 36 of the 47 are at 100 % and the residuals below are what keeps the rest off 100 %.
 *
 * Residual (all measured with the committed `cflags_main`, i.e. `-O3 -inline noauto`):
 *   * `fn_80040360` 99.47 % - 76 B both, 19 instructions, rows 6-7 only: retail `lis r4, Screen_w@ha;`
 *     `addi r31, r4, Screen_w@l`, ours `lis r31, Screen_w@ha; addi r31, r31, Screen_w@l` - the same
 *     lis-scratch-vs-coalesced residual as `fn_8004029C`. Six source shapes tried (a named `ScreenWork*`
 *     local, a `u32` base local, a `char*` base local, an `f32*` parameter, a `_MH_VEC2*` source local, and
 *     two spelled-out `(char*)&Screen_w + off` expressions), all byte-identical, so it is an allocator
 *     tie-break, not source-shaped.
 *   * `fn_8003F58C` 90.81 % - 148 B both. Two argument-setup order swaps (`mr r5,r3` before `mr r6,r4`;
 *     `addi r8,r4,0x7314` before `li r7,0`) and one coalesced `lis r9` where retail uses a scratch `r4`.
 *   * `main` 96.70 % - 704 B target vs 712 B ours. First divergence is instruction 19: retail emits
 *     `lis r3, Screen_w@ha; addi r4, r3, Screen_w@l; li r0,640; sth r0,Screen_w@l(r3); li r0,448;
 *     sth r0,2(r4)`, we emit `li r0,640` one slot earlier and the `addi r4` one slot later (same six
 *     instructions, scheduler tie-break). The other two are allocator tie-breaks of the same kind: the
 *     `&Screen_w` base is re-materialized into r3 at the `if/else` merge (+2 instructions, `main`'s whole
 *     8 B overshoot) where retail keeps r4, and the pool `lfd` above. Source variants tried for the
 *     re-materialization: a local `s32 vcnt` temp, the conversion reading `Screen_w.w16` back, explicit
 *     `(s32)`/`(char)` casts - no change.
 *   * `fn_8003F730` 96.25 % - 528 B both, 8 ARG rows and nothing else. All of them are the r30/r31 split
 *     between the `arg` parameter and the `_f_bss` base in the `arg != 0` body: retail keeps the argument in
 *     r30 and the base in r31, we do it the other way round (`mr r30,r3` vs `mr r31,r3`). Tried: a
 *     top-of-function `GXRenderModeObj* mode = (GXRenderModeObj*)_f_bss;` local (worse - 81.80 %, the base
 *     is hoisted and a third callee-saved register appears) and a `void*` parameter (no change).
 *   * `fn_8003F9E4` 95.73 % - 524 B target vs 516 ours. The `Screen_w` base materialisation order at the top
 *     of the `Screen_w` block (the same tie-break as `main`, 4 rows) and, after the `if`/`else` merge, the
 *     base re-materialized into r3 for the `f44`/`f48`/`f52`/`f56` stores where retail keeps the r4 it
 *     loaded before the first `sth` (6 rows).
 *   * `fn_8003FCCC` 95.06 % - 340 B target vs 332 ours: retail's frame is 0x30 (it saves r31 and keeps the
 *     zero for the `copySize` local in it from `li r31,0` at the top) where ours is 0x20 and re-materialises
 *     `li r0,0` at the `stw r0,8(r1)`. Moving the `u32 copySize = 0;` declaration to the top of the function
 *     does not stop the sink. Plus one pool `lfd` ARG row.
 *   * `fn_8003FEBC` 98.00 % / `fn_8003FF98` 99.02 % - one instruction each: retail initialises *two*
 *     int->float conversion temps (`lis r0,0x4330` twice, stack slots 16/24 or 8/16) where our build CSEs
 *     the second `lis` away, plus one pool `lfd` ARG row each.
 *   * `fn_8003FC64` 99.58 % - one `lis r4` vs `lis r3` scratch-register choice on the warning-counter base.
 *   * `fn_8004029C` 99.52 % - `lis r3, Screen_w@ha; addi r31, r3, Screen_w@l` in retail vs our
 *     `lis r31, Screen_w@ha; addi r31, r31, Screen_w@l`. `fn_8004030C` 99.76 % - one pool `lfd` ARG row.
 */

#include "types.h"

/* The retail `.sbss`/`.bss`/`.sdata2` labels this unit references; they live in the unclaimed scaffolding
 * until the data ranges are split, so these are declarations only and objdiff pairs the relocations by the
 * map's own names. */
extern u8 lbl_80794780;
extern u8 lbl_80794781;
extern u8 lbl_80794782;
extern u8 lbl_80794784;
extern s32 lbl_8079479C;
extern u8 lbl_807947A5;
extern u8 lbl_807947A6;

extern const f32 lbl_80795AA0;
extern const f32 lbl_80795AA4;
extern const f32 lbl_80795AA8;
extern const f32 lbl_80795AAC;
extern const f32 lbl_80795AB0;
extern const f32 lbl_80795AB4;
extern const f32 lbl_80795AB8;
extern const f32 lbl_80795ABC;
extern const f64 lbl_80795AC0;
extern const f32 lbl_80795AC8;
extern const f32 lbl_80795ACC;
extern const f64 lbl_80795AD0;

/* The active render mode (`GXRenderModeObj`), the SDK's own layout: the 4-byte TV mode, the 7 16-bit
 * geometry fields, then the interlace/field/AA bytes, the 12x2 sample pattern and the 7-byte V filter. */
typedef struct {
    u32 viTVMode;
    u16 fbWidth;
    u16 efbHeight;
    u16 xfbHeight;
    u16 viXOrigin;
    u16 viYOrigin;
    u16 viWidth;
    u16 viHeight;
    u32 xfbMode;
    u8 field_rendering;
    u8 aa;
    u8 sample_pattern[12][2];
    u8 vfilter[7];
} GXRenderModeObj;

/* A pair of floats; `get_ScreenSize` is the only user, and its map name pins the tag to `_MH_VEC2`. */
struct _MH_VEC2 {
    f32 x;
    f32 y;
};

extern GXRenderModeObj* Rmode;
extern u32 restart;

/* Screen geometry/calibration block (0x54 B in retail). */
typedef struct {
    u16 w0;
    u16 w2;
    f32 f4;
    f32 f8;
    f32 f12;
    u32 w16;
    f32 f20;
    u8 b24;
    u8 b25;
    u8 unk26;
    u8 pad27[17];
    f32 f44;
    f32 f48;
    f32 f52;
    f32 f56;
    u16 h60;
    u16 h62;
    u8 pad64[8];
    f32 fa72[3];
} ScreenWork;

extern ScreenWork Screen_w;

/* Game/system state block (0xA5C B in retail). */
typedef struct {
    u8 pad0[1];
    u8 unk1;
    u8 pad2[6];
    u8 unk8;
    u8 pad9[2138];
    u8 unk2147;
    u8 unk2148;
    u8 unk2149;
    u8 pad2150[8];
    u8 unk2158;
    u8 unk2159;
    u8 pad2160[1];
    u8 unk2161;
    u8 pad2162[102];
    u32 (*unk2264)(void);
    u8 pad2268[4];
    void (*unk2272)(void);
    void (*unk2276)(void);
    u8 pad2280[73];
    u8 unk2353;
} SystemWork;

extern SystemWork system_w;

extern char _f_text[];
extern char _f_bss[];
extern char _f_data[];

/* The unit's own small-data, .bss and .rodata scaffolding, plus the two GX render-mode tables the TV-mode
 * switch picks from. All of it is claimed by this unit's split (see the file header). */
extern u32 lbl_80794760;
extern u32 lbl_80794764;
extern void* lbl_80794770[2];
extern void* lbl_80794778;
extern u32 lbl_8079477C;
extern u8 lbl_80794783;
extern u8 lbl_80794785;
extern void* lbl_80794788;
extern u32 lbl_8079478C;
extern u8 lbl_80794791;
extern u16 lbl_80794792;
extern u16 lbl_80794794;
extern u8 lbl_807947A4;
extern void* lbl_807947AC;
extern u32 lbl_807947B0;
extern void* lbl_8079483C;
extern u32 lbl_80794868;
extern void* lbl_80794A20;
extern u8 lbl_80790E20;
extern u32 lbl_80658580[3];
extern u32 lbl_8065858C[3];
extern u32 lbl_80658540[];
extern char lbl_8057C82C[];
extern char lbl_8057C850[];
extern GXRenderModeObj lbl_8061A9C0;
extern GXRenderModeObj lbl_8061A9FC;
extern GXRenderModeObj lbl_8061AA74;

/* --- the SDK and runtime entry points this unit's init/video helpers call --- */
extern "C" void* OSGetMEM1ArenaLo(void);
extern "C" void* OSGetMEM1ArenaHi(void);
extern "C" void* OSGetMEM2ArenaLo(void);
extern "C" void* OSGetMEM2ArenaHi(void);
extern "C" void OSSetMEM1ArenaLo(void* lo);
extern "C" void MEMInitAllocatorForExpHeap(void* allocator, void* heap, int align);
extern "C" u8 fn_804DCB40(void);
extern "C" u8 fn_804DC9E0(void);
extern "C" u8 SCGetLanguage(void);
extern "C" u32 fn_804E8DA0(void);
extern "C" void fn_804E8D40(void);
extern "C" u32 VIGetTvFormat(void);
extern "C" void OSPanic(const char* file, int line, const char* msg, ...);
extern "C" void OSReport(const char* fmt, ...);
extern "C" void fn_804B6C00(void* src, void* dst, u32 offset, u32 size);
extern "C" void fn_8003F940(GXRenderModeObj* dst, GXRenderModeObj* src);
extern "C" u32 GXInit(void* base, u32 size);
extern "C" void fn_804BA7A0(f32, f32, f32, f32, f32, f32);
extern "C" void fn_804BA7E0(f32, f32, f32, f32, f32, f32);
extern "C" void fn_804BA830(u32, u32, u32, u32);
extern "C" void fn_804B7270(void* buf, u32 mask);
extern "C" void fn_804B6D60(u32, u32, u32, u32);
extern "C" void fn_804B6DE0(u32, u32);
extern "C" void fn_804B6F70(u32, u32);
extern "C" void fn_804B71A0(void);
extern "C" void GXSetCopyFilter(u8 aa, u8* pattern, u32 enable, u8* filter);
extern "C" void GXSetDispCopyGamma(u32);
extern "C" void fn_804B9FC0(u32);
extern "C" void fn_804B9FF0(u32, u32);
extern "C" void fn_804B7500(void* fb, u32);
extern "C" void GXDrawDone(void);
extern "C" void fn_804B64B0(void*);
extern "C" void fn_804E7F60(void*);
extern "C" void fn_804E8AB0(void*);
extern "C" void fn_804E8BB0(void);
extern "C" void fn_804B5B20(void);
extern "C" void GXInvalidateTexAll(void);
extern "C" void GXSetAlphaCompare(u32, u32, u32, u32, u32);
extern "C" void GXSetZMode(u32, u32, u32);
extern "C" void GXSetAlphaUpdate(u32);
extern "C" void VISetBlack(u32);
extern "C" void VIFlush(void);
extern "C" void fn_804E79D0(void);
extern "C" void fn_804EAC70(u8);
extern "C" void fn_80477150(void);
extern "C" void fn_804D4CA0(u32, u32);
extern "C" s8 fn_800CF384(void);

/* The bus clock lives in the DOL header at a fixed address; the frame wait converts it to ticks. */
#define OS_BUS_CLOCK (*(volatile u32*)0x800000F8)

extern "C" void OSInit(void);
extern "C" void DVDInit(void);
extern "C" void NANDInit(void);
extern "C" void OSRestart(u32 resetCode);
extern "C" void* memset(void* dst, int val, u32 size);

extern "C" void fn_8003F4D8(void);
extern "C" void fn_8003F58C(u8 arg);
extern "C" void fn_8003F620(void);
extern "C" void* fn_8003F728(void* startAddress, u32 size);
extern "C" void fn_8003F730(s32 arg);
extern "C" void fn_8003FCCC(void);
extern "C" void fn_8003FE24(void);
extern "C" void fn_8003FE30(void);
extern "C" void fn_8003FEBC(void);
extern "C" void fn_8003FF98(void);
extern "C" void fn_80040144(void);
extern "C" void fn_800408A8(void);
extern "C" void fn_80040FDC(void);
extern "C" void fn_80041304(void);
extern "C" void fn_80044A54(void);
extern "C" void fn_80046C80(void);
extern "C" void fn_80046D34(void);
extern "C" int fn_804ED620(void);
extern "C" void fn_804E7480(void);
extern "C" void fn_804D2520(void);
extern "C" void fn_804D56B0(void* arg);
extern "C" void fn_804D57A0(void* arg);
extern "C" void fn_8043F290(void);
extern "C" void fn_800D8438(void);

/* C++ symbols: declared by their real (unmangled) source names so the compiler emits the map's mangled
 * forms, exactly as the retail callers do. */
extern void SetSystemVcnt(long vcnt);
extern void bgm_stop_all(void);

/* Raises the video-mode-change latch the game loop polls. */
extern "C" void fn_8003F200(void)
{
    lbl_807947A5 = 1;
}

/* Raises the restart-requested latch the game loop polls. */
extern "C" void fn_8003F20C(void)
{
    lbl_807947A6 = 1;
}

int main(void)
{
    fn_8003F4D8();
    OSInit();
    DVDInit();
    fn_804E7480();
    fn_8003F730(0);
    fn_8003F620();
    fn_8003FCCC();
    fn_8003FE30();
    fn_8003F58C(lbl_80794784);
    SetSystemVcnt(2);
    fn_8003FE24();

    Screen_w.w0 = 640;
    Screen_w.w2 = 448;
    if (lbl_80794781 == 1) {
        f32 fb = lbl_80795AA0;
        Screen_w.f4 = fb;
        Screen_w.f8 = lbl_80795AA4;
        Screen_w.f12 = lbl_80795AA8;
        Screen_w.h60 = (s32)fb;
        Screen_w.h62 = Rmode->efbHeight;
        Screen_w.b25 = 1;
    } else {
        f32 fb = lbl_80795AAC;
        Screen_w.f4 = fb;
        Screen_w.f8 = lbl_80795AA4;
        Screen_w.f12 = lbl_80795AB0;
        Screen_w.b25 = 0;
        Screen_w.h60 = (s32)fb;
        Screen_w.h62 = Rmode->efbHeight;
    }
    Screen_w.w16 = lbl_8079479C;
    Screen_w.f20 = lbl_80795AB4 / (f32)lbl_8079479C;
    Screen_w.b24 = lbl_80794780;
    Screen_w.f44 = lbl_80795AB8;
    Screen_w.f48 = lbl_80795ABC;
    Screen_w.f52 = Screen_w.f4 - Screen_w.f44;
    Screen_w.f56 = Screen_w.f8 - Screen_w.f48;

    fn_804D56B0(_f_text);
    fn_804D57A0((void*)fn_8003F20C);
    lbl_807947A5 = 0;
    lbl_807947A6 = 0;
    NANDInit();
    memset(&system_w, 0, 2652);
    system_w.unk2147 = 0xFF;
    system_w.unk2148 = 0xFF;
    system_w.unk8 = 1;
    if ((u32)(lbl_80794782 - 1) <= 5) {
        system_w.unk1 = lbl_80794782;
    } else {
        system_w.unk1 = 1;
    }

    fn_80041304();
    fn_800408A8();
    fn_80040FDC();
    fn_80046C80();

    while (1) {
        fn_8003FEBC();
        fn_80046D34();
        fn_8003FF98();
        fn_80040144();
        if (system_w.unk2161 != 0) {
            continue;
        }
        if (system_w.unk2149 != 0) {
            continue;
        }
        if (system_w.unk2353 != 0) {
            continue;
        }
        if (lbl_807947A6 != 1 && lbl_807947A5 != 1) {
            continue;
        }
        if (lbl_807947A5 != 0) {
            if (system_w.unk2264() == 1) {
                system_w.unk2272();
                system_w.unk2161 = 1;
                continue;
            }
        }
        if (fn_804ED620() == 3) {
            fn_80044A54();
        }
        if (lbl_807947A5 != 0 && system_w.unk2158 != 0) {
            system_w.unk2276();
        }
        fn_800D8438();
        bgm_stop_all();
        fn_8043F290();
        if (lbl_807947A5 != 0) {
            OSRestart(restart);
        }
        if (lbl_807947A6 != 0) {
            fn_804D2520();
        }
    }
}

/* ---- 0x8003F4D8-0x8003F564: GQR/VI counters and the periodic wide-mode request ---- */

extern u32 lbl_80794798;
extern s32 lbl_807947A0;

extern "C" u32 fn_804AA710(void);

/* Sets GQR2-GQR5 to the (4,4)...(7,7) quantisation pairs. */
asm void fn_8003F4D8(void)
{
    nofralloc
    li r3, 4
    oris r3, r3, 4
    mtspr 914, r3
    li r3, 5
    oris r3, r3, 5
    mtspr 915, r3
    li r3, 6
    oris r3, r3, 6
    mtspr 916, r3
    li r3, 7
    oris r3, r3, 7
    mtspr 917, r3
    blr
}

/* Stores the system VI clock counter, clamped to at least 1. */
void SetSystemVcnt(long vcnt)
{
    lbl_8079479C = (vcnt > 0) ? vcnt : 1;
}

/* Reads the system VI clock counter back. */
extern "C" s32 fn_8003F524(void)
{
    return lbl_8079479C;
}

extern "C" void fn_8003F554(void);

/* Latches the VI-clock alarm handle the periodic handler will install. */
extern "C" void fn_8003F52C(void)
{
    fn_8003F554();
    lbl_80794798 = fn_804AA710();
}

/* Bumps the periodic request counter. */
extern "C" void fn_8003F554(void)
{
    lbl_807947A0++;
}

/* Latches the VI-clock alarm handle for the second periodic slot. */
extern "C" void fn_8003F564(void)
{
    fn_8003F554();
    lbl_80794798 = fn_804AA710();
}

/* ---- 0x8003F58C-0x8003F730: the periodic alarm and the memory arenas ---- */

extern char lbl_80658550[];

/* The map has no name for 0x804E7110; the shared runtime dump calls it AIRegisterDMACallback (it swaps
the single DMA-callback pointer at 0x80795654). The relocation has to carry the map's name. */
extern "C" void* fn_804E7110(void* callback);
extern "C" void OSCancelAlarm(void* alarm);
extern "C" u64 OSGetTime(void);
extern "C" void OSSetPeriodicAlarm(void* alarm, u32 unk, u64 time, u32 arg5, u32 arg6, void* handler);
extern "C" void* MEMCreateExpHeapEx(void* startAddress, u32 size, u16 option);

/* Arms or disarms the periodic alarm that keeps the VI-clock request counter running, and swaps the
 * audio DMA callback that services it. */
extern "C" void fn_8003F58C(u8 arg)
{
    fn_804E7110(0);
    OSCancelAlarm(lbl_80658550);
    if (arg == 0) {
        fn_804E7110((void*)fn_8003F564);
        OSCancelAlarm(lbl_80658550);
    } else {
        fn_804E7110(0);
        OSSetPeriodicAlarm(lbl_80658550, 0xF0000, OSGetTime(), 0, 0xF7314, (void*)fn_8003F52C);
    }
}

/* Records the two memory arenas, carves the main exp heap out of MEM1 and the two MEM2 sub-heaps, and
 * installs the allocator that serves it. */
extern "C" void fn_8003F620(void)
{
    void* mem1Lo = OSGetMEM1ArenaLo();
    void* mem1Hi = OSGetMEM1ArenaHi();

    lbl_80658580[0] = 0x80810000;
    lbl_80658580[1] = (u32)mem1Lo;
    lbl_80658580[2] = (u32)mem1Hi;
    lbl_80794760 = 0x80E2C600;
    lbl_80794770[0] = (void*)0x80EAC600;
    lbl_80794770[1] = (void*)((u32)lbl_80794770[0] + 0xA5000);
    lbl_8079478C = 0x1DF800;
    lbl_807947AC = (void*)0x80C4CE00;

    void* heap = MEMCreateExpHeapEx((void*)0x80C4CE00, 0x1DF800, 4);
    lbl_80794788 = heap;
    MEMInitAllocatorForExpHeap(lbl_80658540, heap, 8);
    lbl_807947B0 = 0x43CE00;
    OSSetMEM1ArenaLo((void*)0x81700000);
    OSGetMEM1ArenaLo();

    void* mem2Lo = OSGetMEM2ArenaLo();
    void* mem2Hi = OSGetMEM2ArenaHi();
    lbl_8065858C[1] = (u32)mem2Lo;
    lbl_8065858C[2] = (u32)mem2Hi;
    lbl_8079483C = fn_8003F728((void*)0x90308000, 0x40000);
    lbl_80794A20 = MEMCreateExpHeapEx((void*)0x92B78000, 0x80000, 4);
}

/* Creates an expansion heap over a raw memory range with the default (blocking) allocation mode. */
extern "C" void* fn_8003F728(void* startAddress, u32 size)
{
    return MEMCreateExpHeapEx(startAddress, size, 0);
}

#pragma peephole off

/* Reads the console's language/TV state, picks the render mode for the TV standard, copies it into the
 * unit's own render-mode object and publishes it. */
extern "C" void fn_8003F730(s32 arg)
{
    lbl_80794780 = fn_804DCB40();
    lbl_80794781 = fn_804DC9E0();
    lbl_80794782 = SCGetLanguage();
    lbl_80794785 = fn_804E8DA0();
    fn_804E8D40();
    lbl_80794783 = VIGetTvFormat();
    lbl_80794784 = 0;

    if (arg != 0) {
        fn_8003F940((GXRenderModeObj*)_f_bss, (GXRenderModeObj*)arg);
        Rmode = (GXRenderModeObj*)_f_bss;
    } else if (lbl_80794780 == 1 && lbl_80794785 == 1) {
        lbl_80794784 = 0;
        Rmode = &lbl_8061A9FC;
        fn_804B6C00(&lbl_8061A9FC, _f_bss, 0, 16);
        if (lbl_80794783 != 0) {
            *(u32*)_f_bss = 22;
        }
    } else {
        switch (lbl_80794783) {
        case 0:
            lbl_80794784 = 0;
            Rmode = &lbl_8061A9C0;
            fn_804B6C00(&lbl_8061A9C0, _f_bss, 0, 16);
            break;
        case 5:
            lbl_80794784 = 0;
            Rmode = &lbl_8061A9C0;
            fn_804B6C00(&lbl_8061A9C0, _f_bss, 0, 16);
            *(u32*)_f_bss = 20;
            break;
        case 1:
            lbl_80794784 = 1;
            Rmode = &lbl_8061AA74;
            fn_804B6C00(&lbl_8061AA74, _f_bss, 0, 40);
            ((GXRenderModeObj*)_f_bss)->xfbHeight = lbl_8061AA74.xfbHeight;
            ((GXRenderModeObj*)_f_bss)->viYOrigin = lbl_8061AA74.viYOrigin;
            ((GXRenderModeObj*)_f_bss)->viHeight = lbl_8061AA74.viHeight;
            break;
        default:
            OSPanic(_f_data, 749, lbl_8057C82C);
        }
    }

    OSReport(lbl_8057C850, *(u32*)_f_bss);
    Rmode = (GXRenderModeObj*)_f_bss;
    lbl_80794791 = (lbl_80794781 == 1);
    lbl_80794792 = ((GXRenderModeObj*)_f_bss)->viXOrigin;
    lbl_80794794 = ((GXRenderModeObj*)_f_bss)->viYOrigin;
}

#pragma peephole reset

/* Copies one render-mode object over another. */
extern "C" void fn_8003F940(GXRenderModeObj* dst, GXRenderModeObj* src)
{
    *dst = *src;
}

#pragma peephole off

/* Re-applies a video mode: rebuilds the mode object for the console's TV standard, pushes the two
 * geometry fields into the active render mode and recomputes the screen geometry. */
extern "C" void fn_8003F9E4(u32 arg)
{
    GXRenderModeObj mode;

    if ((u8)arg == lbl_80794791) {
        return;
    }
    if (lbl_80794780 == 1) {
        fn_804B6C00(&lbl_8061A9FC, &mode, 0, 16);
        if (lbl_80794783 != 0) {
            *(u32*)&mode = 22;
        }
    } else {
        switch (lbl_80794783) {
        case 0:
            fn_804B6C00(&lbl_8061A9C0, &mode, 0, 16);
            break;
        case 5:
            fn_804B6C00(&lbl_8061A9C0, &mode, 0, 16);
            *(u32*)&mode = 20;
            break;
        case 1:
            fn_804B6C00(&lbl_8061AA74, &mode, 0, 40);
            mode.xfbHeight = lbl_8061AA74.xfbHeight;
            mode.viYOrigin = lbl_8061AA74.viYOrigin;
            mode.viHeight = lbl_8061AA74.viHeight;
            break;
        default:
            return;
        }
    }

#pragma peephole reset

    Rmode->viWidth = mode.viWidth;
    Rmode->viXOrigin = mode.viXOrigin;
    fn_804E7F60(Rmode);

    Screen_w.w0 = 640;
    Screen_w.w2 = 448;
    if ((u8)arg == 1) {
        f32 fb = lbl_80795AA0;
        Screen_w.f4 = fb;
        Screen_w.f8 = lbl_80795AA4;
        Screen_w.f12 = lbl_80795AA8;
        Screen_w.h60 = (s32)fb;
        Screen_w.h62 = Rmode->efbHeight;
        Screen_w.b25 = 1;
    } else {
        f32 fb = lbl_80795AAC;
        Screen_w.f4 = fb;
        Screen_w.f8 = lbl_80795AA4;
        Screen_w.f12 = lbl_80795AB0;
        Screen_w.b25 = 0;
        Screen_w.h60 = (s32)fb;
        Screen_w.h62 = Rmode->efbHeight;
    }
    Screen_w.f44 = lbl_80795AB8;
    Screen_w.f48 = lbl_80795ABC;
    Screen_w.f52 = Screen_w.f4 - Screen_w.f44;
    Screen_w.f56 = Screen_w.f8 - Screen_w.f48;
    lbl_80794791 = arg;
}

#pragma peephole reset

/* ---- 0x8003FBE8-0x8003FCC8: wide mode / brightness / TV-mode accessors ---- */

extern u8 set_widemode_flag;
extern char set_widemode_param;
extern f32 lbl_80790E24;
extern char lbl_8057C860[];
extern u32 lbl_80658598[];

extern "C" void OSReport(const char* fmt, ...);
extern "C" void GXSetColorUpdate(int enable);

/* Scoped peephole-off for this and the next function - evidence in the file header. */
#pragma peephole off

/* Requests a video-mode change; the flag is polled by the game loop and the parameter remembers the
 * requested mode. */
void change_widemode_req(unsigned char param)
{
    set_widemode_flag = 1;
    set_widemode_param = param;
}

/* Reports whether a video-mode change was requested. */
u8 check_change_widemode_flag(void)
{
    return set_widemode_flag;
}

/* Requests the default (ntsc/pal) video mode for the console's TV standard. */
u8 change_widemode_req_default(void)
{
    u8 mode = (lbl_80794781 == 1);
    change_widemode_req(mode);
    return mode;
}

#pragma peephole reset

/* Reads the screen brightness the game is currently using. */
extern "C" f32 fn_8003FC48(void)
{
    return lbl_80790E24;
}

/* Sets the screen brightness. */
void set_now_brightness(float brightness)
{
    lbl_80790E24 = brightness;
}

/* Empty; the retail object defines the symbol as a bare return. */
extern "C" void fn_8003FC58(void)
{
}

/* Reads the TV standard detected at boot. */
u8 get_tv_mode(void)
{
    return lbl_80794784;
}

/* Logs a warning and counts it per warning id. */
extern "C" void fn_8003FC64(u8 arg, u32 id, u32 value)
{
    OSReport(lbl_8057C860, arg, id, value);
    lbl_80658598[id]++;
}

/* Enables GX colour output. */
void hbm_InitGX(void)
{
    GXSetColorUpdate(1);
}

/* ---- 0x8003FCCC-0x8004035C: the per-frame GX/VI setup and the screen-geometry accessors ---- */

/* Brings GX up for the active render mode: initialises the FIFO, sets the projection and copy
 * configuration from the render-mode object, then binds both framebuffers. */
extern "C" void fn_8003FCCC(void)
{
    u32 copySize = 0;

    lbl_80794764 = GXInit((void*)lbl_80794760, 0x80000);

    fn_804BA7E0(lbl_80795AC8, lbl_80795AC8, (f32)Rmode->fbWidth, (f32)Rmode->efbHeight,
                lbl_80795AC8, lbl_80795ACC);
    fn_804BA830(0, 0, Rmode->fbWidth, Rmode->efbHeight);

    fn_804B7270(&copySize, 0xFFFFFF);

    fn_804B6D60(0, 0, Rmode->fbWidth, Rmode->efbHeight);
    fn_804B6DE0(Rmode->fbWidth, Rmode->xfbHeight);
    GXSetCopyFilter(Rmode->aa, Rmode->sample_pattern[0], 1, Rmode->vfilter);
    GXSetDispCopyGamma(0);
    fn_804B9FC0(0);
    if (Rmode->aa != 0) {
        fn_804B9FF0(2, 0);
    } else {
        fn_804B9FF0(0, 0);
    }
    fn_804B7500(lbl_80794770[0], 1);
    fn_804B7500(lbl_80794770[0], 0);
    fn_804B7500(lbl_80794770[1], 0);
    GXDrawDone();
}

/* Empty; the retail object defines the symbol as a bare return. */
extern "C" void fn_8003FE20(void)
{
}

/* Installs the empty callback above as the GX draw-done handler. */
extern "C" void fn_8003FE24(void)
{
    fn_804B64B0((void*)fn_8003FE20);
}

#pragma peephole off

/* Swaps to the other framebuffer: sets the display copy source for it, flushes, and waits for a
 * field if the mode is interlaced. */
extern "C" void fn_8003FE30(void)
{
    fn_804B6F70(Rmode->efbHeight, Rmode->xfbHeight);
    fn_804B71A0();
    fn_804E7F60(Rmode);
    fn_804E8AB0(lbl_80794770[lbl_8079477C]);
    lbl_8079477C ^= 1;
    lbl_80794778 = lbl_80794770[lbl_8079477C];
    VIFlush();
    fn_804E79D0();
    if ((Rmode->viTVMode & 1) != 0) {
        fn_804E79D0();
    }
}

#pragma peephole reset

/* Sets the copy/projection state that the frame's first draw needs, and clears the texture cache. */
extern "C" void fn_8003FEBC(void)
{
    GXRenderModeObj* mode = Rmode;

    if (mode->field_rendering != 0) {
        fn_804E8BB0();
        fn_804BA7A0(lbl_80795AC8, lbl_80795AC8, (f32)mode->fbWidth, (f32)mode->efbHeight,
                    lbl_80795AC8, lbl_80795ACC);
    } else {
        fn_804BA7E0(lbl_80795AC8, lbl_80795AC8, (f32)mode->fbWidth, (f32)mode->efbHeight,
                    lbl_80795AC8, lbl_80795ACC);
    }
    fn_804B5B20();
    GXInvalidateTexAll();
    GXSetAlphaCompare(7, 0, 1, 7, 0);
}

/* Per-frame GX setup: depth/alpha/colour state, then the copy filter rebuilt from the current
 * brightness scale and the render-mode V filter, and finally the other framebuffer. */
extern "C" void fn_8003FF98(void)
{
    GXSetZMode(1, 3, 1);
    GXSetAlphaUpdate(1);
    GXSetColorUpdate(1);

    f32 scale = (system_w.unk2159 == 2) ? lbl_80795ACC : lbl_80790E24;
    u8 filter[7];
    for (int i = 0; i < 7; i++) {
        filter[i] = (u8)(scale * (f32)Rmode->vfilter[i]);
    }
    GXSetCopyFilter(Rmode->aa, Rmode->sample_pattern[0], 1, filter);

    fn_804B7500(lbl_80794778, 1);
    GXSetCopyFilter(Rmode->aa, Rmode->sample_pattern[0], 1, Rmode->vfilter);
    GXDrawDone();
}

/* Waits for the pending VI requests, presents the frame, applies a queued video-mode change and then
 * waits out the second half of the frame before flipping the framebuffer. */
extern "C" void fn_80040144(void)
{
    while (lbl_807947A0 < fn_8003F524() - 1) {
        u32 ticks = (OS_BUS_CLOCK >> 2) / 1000;
        fn_804D4CA0(0, ticks);
    }
    fn_804E8AB0(lbl_80794778);
    if (lbl_80790E20 != 0) {
        VISetBlack(0);
        lbl_80790E20 = 0;
    }
    if (set_widemode_flag != 0) {
        fn_8003F9E4((u8)set_widemode_param);
        set_widemode_flag = 0;
        set_widemode_param = -1;
    }
    VIFlush();
    fn_804EAC70(lbl_807947A4);
    lbl_8079477C ^= 1;
    lbl_80794778 = lbl_80794770[lbl_8079477C];
    fn_80477150();
    while (lbl_807947A0 < fn_8003F524()) {
        u32 ticks = (OS_BUS_CLOCK >> 2) / 1000;
        fn_804D4CA0(0, ticks);
    }
    lbl_80794868++;
    lbl_807947A0 = 0;
}

/* The framebuffer the next frame will draw into. */
extern "C" void* fn_8004026C(void)
{
    return lbl_80794778;
}

/* The active framebuffer width. */
extern "C" u16 fn_80040274(void)
{
    return Rmode->fbWidth;
}

/* The active EFB height. */
extern "C" u16 fn_80040280(void)
{
    return Rmode->efbHeight;
}

/* The screen's visible origin pair. */
extern "C" u16* fn_8004028C(void)
{
    return &Screen_w.h60;
}

/* The screen height the current draw should use: a per-view offset when the calibration block has one,
 * the plain vertical size otherwise. */
extern "C" f32 fn_8004029C(void)
{
    if (Screen_w.unk26 != 0) {
        return Screen_w.fa72[(s8)fn_800CF384()];
    }
    return Screen_w.f12;
}

/* Writes the screen size the game is rendering at into `v`. */
void get_ScreenSize(_MH_VEC2* v)
{
    v->x = Screen_w.f4;
    v->y = Screen_w.f8;
}

/* Writes the screen size as floats, signed, for the display-list paths that need a float size. */
extern "C" void fn_8004030C(_MH_VEC2* v)
{
    v->x = (f32)(s16)Screen_w.w0;
    v->y = (f32)(s16)Screen_w.w2;
}

/* ---- 0x80040360-0x80040478: the screen-size accessors and the game's expansion-heap allocator ---- */

extern "C" void* fn_804C2200(void* heap, u32 size, u32 align);
extern "C" void fn_804C22B0(void* heap, void* block);
extern "C" void fn_800403AC(_MH_VEC2* dst, const _MH_VEC2* src);

/* Copies the two screen-size rectangles out of the calibration block. */
extern "C" void fn_80040360(_MH_VEC2* dst)
{
    fn_800403AC(&dst[0], (const _MH_VEC2*)&Screen_w.f44);
    fn_800403AC(&dst[1], (const _MH_VEC2*)&Screen_w.f52);
}

/* Copies one screen-size rectangle. */
extern "C" void fn_800403AC(_MH_VEC2* dst, const _MH_VEC2* src)
{
    dst->x = src->x;
    dst->y = src->y;
}

/* Reports whether the console is running in wide mode. */
int ck_WideMode(void)
{
    return Screen_w.b25 != 0;
}

/* Reports whether the calibration block's wide-mode flag is set. */
extern "C" u32 fn_800403DC(void)
{
    return Screen_w.b24 != 0;
}

/* Reports whether the calibration block's third flag is set. */
extern "C" u32 fn_800403F8(void)
{
    return Screen_w.unk26 != 0;
}

/* Returns 0; the retail object defines the symbol as a constant load. */
extern "C" u32 fn_80040414(void)
{
    return 0;
}

/* Empty; the retail object defines the symbol as a bare return. */
extern "C" void fn_8004041C(void)
{
}

/* Allocates `size` bytes, 8-aligned, from the expansion heap. */
extern "C" void* fn_80040420(u32 size)
{
    void* block = NULL;
    if (size != 0) {
        block = fn_804C2200(lbl_80794788, size, 8);
    }
    return block;
}

/* Returns a block to the expansion heap. */
extern "C" void fn_80040460(void* block)
{
    if (block != NULL) {
        fn_804C22B0(lbl_80794788, block);
    }
}
