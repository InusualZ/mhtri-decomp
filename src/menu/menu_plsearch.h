/* Declarations owned by `src/menu/menu_plsearch.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_MENU_MENU_PLSEARCH_H
#define MHTRI_MENU_MENU_PLSEARCH_H

#include "types.h"

/* Declarations moved here from `unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct PatTerms;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80449968 / 0x8044996C / 0x80449918 / 0x8044991C - the boot and account loading steps. */
void startBootLoad(void);

s32 pollBootLoad(void);

void startAccountLoad(void);

s32 pollAccountLoad(s16 frame);

/* 0x8044F520 - the lobby mailbox inside the lobby state block (+0x1FC4); `Network/net_session_close.cpp`
 * appends received mail to it (GUESS name: the body returns that address).  `NetMailBox` is that consumer's. */
struct NetMailBox* getLobbyMailBox(void);

/* 0x80450F8C / 0x80451024 / 0x80451040 / 0x8045106C / 0x804513A8 - the terms object's (`struct PatTerms`,
 * `Network/network_pat_control.h`) open, check request, update request, update cancel and progress read, which
 * the mediator's terms wrappers forward to (GUESS names from the bodies: the open stores the buffer pair and sets
 * the ready byte, the check request clears it and raises +0x0E, the update request raises +0x0F while ready, the
 * cancel drops +0x0F and raises +0x10, the read returns the +0xE0 count). */
/* .sbss 0x80794D58 - the live terms object (its constructor publishes it, the destructor clears it; GUESS name). */
extern struct PatTerms* sPatTerms;

/* untyped: byte range - the MEM2 buffer handed to the terms object */
void initPatTerms(struct PatTerms* terms, void* buffer, u32 size);
void requestPatTermsCheck(struct PatTerms* terms);
void requestPatTermsUpdate(struct PatTerms* terms);
void cancelPatTermsUpdate(struct PatTerms* terms);
u16 getPatTermsProgress(struct PatTerms* terms);
/* 0x80450580 - the terms object's state step (a 21-case switch on its +0x0C state byte); nonzero once it rests
 * (GUESS name: the mediator steps it every frame and spins on it in its destructor). */
s32 updatePatTerms(struct PatTerms* terms);

/* 0x804512E8 - runs the echo suppressor over `size` bytes of microphone input into `out` while the terms object is
 * ready (GUESS name: the failure log reads "fail to suppress echo P-Mic"); returns the suppressor's result. */
s32 suppressPatTermsEcho(struct PatTerms* terms, const u8* in, u8* out, s32 size);

/* 0x8045108C / 0x804510FC - read `size` bytes of voice samples into `out` / write `size` bytes from `in` (the
 * second reads `in` as 16-bit samples and consults the terms object's +0xE2 flag) through the voice band 0x80526180 / 0x80526270, once
 * the terms update finished; the byte count, 0 before.  NAMES (GUESSES): the mediator's `readVoice` forwards to the
 * first, its unnamed 0x80417080 to the second. */
s32 readPatTermsVoice(struct PatTerms* terms, u8* out, s32 size);
s32 writePatTermsVoice(struct PatTerms* terms, const s16* in, s32 size);

/* 0x80449860 / 0x804498C8 / 0x804498CC - start the data file, start the system file and wait for the system file
 * (the last stores the save-busy flag) (GUESS names). */
s32 game_data_file_create_start(void);
s32 game_system_file_create_start(void);
s32 game_system_file_create_wait(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x80449878 - waits for the game save (C++ scope: `game_save_wait__Fv`). */
s32 game_save_wait(void);
/* 0x80449970 - starts creating the save file of player `player` in slot `slot` (C++ scope, `__Flll`); 1 when started. */
u32 createDataFile_init(long player, long slot, long mode);
/* 0x8044ACB8 - starts reading player `player`'s save file (`__Fll`). */
void readDataFile_init(long player, long mode);
/* 0x8044AFDC - steps the save-file read; nonzero once it has finished. */
s32 readDataFile(void);
/* 0x8044B268 - copies save slot `slot`'s player data into the user data. */
void setPlayerSave2Userdata(u8 slot);
/* 0x8044BD44 - the message id for the last NAND error. */
s32 chg_nand_err2msgcode(void);
#endif

#endif /* MHTRI_MENU_MENU_PLSEARCH_H */
