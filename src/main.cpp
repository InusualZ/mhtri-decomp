/*
 * The game's root translation unit: main(), the pre-main latches, and the init/video/mode helpers.
 *
 * .text 0x8003F200-0x80040360 - 38 functions, 4448 B, one file. `__start` calls `main` (0x8003F218, the DOL
 * entry point); the first 24 B are the two pre-main latches `fn_8003F200`/`fn_8003F20C` that set the bytes
 * main polls (0x807947A5/A6); then the GQR/VI/TV setup, the memory-heap creation, and the wide-mode /
 * brightness / screen-size accessors.
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
 * Identified but **not claimed**: the unit's own data fragments. A split has to be section-aligned
 * (`.sbss`/`.data` are 16/32) and the retail fragment ends are not visible from the symbol map, so they are
 * claimed later, with a measurement (playbook 23) - the run-time globals `set_widemode_flag` (.sbss
 * 0x80794790), `set_widemode_param` (.sdata 0x80790E21), `system_w` (0x806585E0) and `Screen_w`
 * (0x8065903C), the `.data` pool at 0x8057C820, `.sdata2` 0x80795AA0-0x80795AD8, and the unwind pair
 * `@etb_800066E0` / `@eti_8001E480`. Until the `.sdata2` range is claimed, `main`'s pooled unsigned->float
 * magic is a *local* `@66` in our object while the target references the map's `lbl_80795AC0`, so that one
 * `lfd` can never pair by name (playbook 23; 1 ARG row).
 *
 * Flags - measured this batch against `build/RMHE08/obj/main.o`, all of it per-library for `main` (never
 * `cflags_base`):
 *   * `-O4,p` -> `-O3`. Decisive: `-O4,p` gives `main` 71.12 %, `fn_8003FC64` 65.62 %,
 *     `change_widemode_req__FUc` 58.00 %, `fn_8003F58C` 79.78 %; `-O3` gives 96.70 / 99.58 / 100 / 90.81.
 *   * **no `-func_align` flag is needed**: `-O3` already packs functions on 4 B, so the target's starts
 *     (`fn_8003F20C` at .text+0xC, `main` at +0x18, `SetSystemVcnt__Fl` at +0x30C) come out right. The
 *     `-O4,p` alternative would have needed `-func_align 4`, since `-O4,p` implies `-func_align 16`
 *     (+0x10/+0x20 there) - one more reason `-O3` is the retail setting.
 *   * `-inline off`. With `-inline auto` (cflags_base's) the retail call to `fn_8003F554` inside
 *     `fn_8003F52C`/`fn_8003F564` is inlined (74.00 % on both, 48 B vs 40 B) and
 *     `change_widemode_req__FUc` is inlined into `change_widemode_req_default__Fv` (21.18 %); `-inline off`
 *     puts both at 100 %. Nothing else moves.
 *   * `-use_lmw_stmw` stays **off**: the target's 4448 B `.text` has no `lmw`/`stmw` at all (objdump), and
 *     `-use_lmw_stmw on` measures byte-identical to off.
 *
 * Load-bearing source shapes:
 *   * `.text` is the plain default section - no pragma here, unlike the `.init` units.
 *   * the unnamed `fn_*` entries are `extern "C"`: their map names are unmangled while the file is C++
 *     (`SetSystemVcnt__Fl`, `change_widemode_req__FUc`, `get_tv_mode__Fv`, `hbm_InitGX__Fv` are the mangled
 *     ones and are written by their *source* names so the compiler emits the map's spelling), so a plain C++
 *     definition of `fn_8003F200` would emit `fn_8003F200__Fv` and objdiff would pair nothing.
 *   * `#pragma peephole off` around `change_widemode_req`/`change_widemode_req_default` only: the peephole
 *     pass deletes the dead sign-extend of the byte parameter and fuses that function's `srwi`+`clrlwi` into
 *     one `rlwinm`, neither of which retail has - while the rest of the unit needs the peephole *on* (it is
 *     what removes `main`'s redundant `clrlwi`s in front of the two `sth r0,0x3e(r4)`; with the whole-unit
 *     `-opt nopeephole` those two appear and `main` drops to 94.49 %). `#pragma peephole off` is the only
 *     spelling this compiler honours (`opt_peephole`/`peep` parse but do nothing).
 *   * `main`'s loop shape - four per-frame calls, three `system_w` byte polls, the two latch polls, the three
 *     function-pointer calls through `system_w`, and the two trailing `if`s - is what puts every `continue`
 *     on the loop head and keeps `&system_w` and the constant `1` in r31/r30.
 *   * `lbl_8079479C` is a **signed** 32-bit global: retail converts it with the `xoris`/`0x4330000080000000`
 *     (signed) idiom, not the `0x4330000000000000` (unsigned) one - MWCC emits them the opposite way round
 *     from the obvious guess, so `u32` there costs 2 instructions and `lfd`s the wrong constant.
 *   * `OSSetPeriodicAlarm` takes 7 register arguments in this SDK (the dump's own body reads `alarm[6]`,
 *     `alarm[7]` and a 64-bit time from the middle pair), so it is declared here as
 *     `(void* alarm, u32 unk, u64 time, u32 arg5, u32 arg6, void* handler)`; the retail call is
 *     `(&alarm, 0xF0000, OSGetTime(), 0, 0xF7314, fn_8003F52C)`.
 *
 * Work in progress: the range is claimed once, so filling the remaining `fn_*` needs no re-split - each batch
 * adds functions to this file, and the unit's score rises as they land.
 *   written (address order, 20 of 38): 0x8003F200, F20C, F218(main), F4D8, F50C, F524, F52C, F554, F564,
 *     F58C, F728, FBE8, FBFC, FC04, FC48, FC50, FC58, FC5C, FC64, FCC4.
 *   next in address order: fn_8003F620 (create_memory_heaps), then fn_8003F730 / F940 / F9E4 / FCCC.
 * Residual:
 *   * `main` 96.70 % - 704 B target vs 712 B ours. First divergence is instruction 19: retail emits
 *     `lis r3, Screen_w@ha; addi r4, r3, Screen_w@l; li r0,640; sth r0,Screen_w@l(r3); li r0,448;
 *     sth r0,2(r4)`, we emit `li r0,640` one slot earlier and the `addi r4` one slot later (same six
 *     instructions, scheduler tie-break). The other three are allocator tie-breaks of the same kind: the
 *     `&Screen_w` base is re-materialized into r3 at the `if/else` merge (+2 instructions, `main`'s whole
 *     8 B overshoot) where retail keeps r4, `lis r31, system_w@ha` vs retail's `lis r3` + `addi r31,r3`,
 *     and the `lfd` above. Source variants tried for the re-materialization: a local `s32 vcnt` temp, the
 *     conversion reading `Screen_w.w16` back, explicit `(s32)`/`(char)` casts - no change.
 *   * `fn_8003F58C` 90.81 % - 148 B both. Two argument-setup order swaps (`mr r5,r3` before `mr r6,r4`;
 *     `addi r8,r4,0x7314` before `li r7,0`) and one coalesced `lis r9` where retail uses a scratch `r4`.
 *   * `fn_8003FC64` 99.58 % - one `lis r4` vs `lis r3` scratch-register choice on the warning-counter base.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef long s32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

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

/* The active render mode (`GXRenderModeObj`): +4 width, +6 height. */
typedef struct {
    u32 tvMode;
    u16 fbWidth;
    u16 efbHeight;
} GXRenderModeObj;

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
    u8 pad26[18];
    f32 f44;
    f32 f48;
    f32 f52;
    f32 f56;
    u16 h60;
    u16 h62;
    u8 pad64[20];
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
    u8 pad2159[2];
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

extern "C" void OSInit(void);
extern "C" void DVDInit(void);
extern "C" void NANDInit(void);
extern "C" void OSRestart(u32 resetCode);
extern "C" void* memset(void* dst, int val, u32 size);

extern "C" void fn_8003F4D8(void);
extern "C" void fn_8003F58C(u8 arg);
extern "C" void fn_8003F620(void);
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
extern u32 lbl_807947A0;

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
extern "C" u32 fn_8003F524(void)
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

/* Creates an expansion heap over a raw memory range with the default (blocking) allocation mode. */
extern "C" void* fn_8003F728(void* startAddress, u32 size)
{
    return MEMCreateExpHeapEx(startAddress, size, 0);
}

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
