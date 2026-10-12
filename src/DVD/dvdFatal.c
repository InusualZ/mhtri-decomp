/*
 * DVD/dvdFatal.c - the SDK DVD fatal-error screen (`__DVDShowFatalMessage`, auto-fatal setter).
 *
 * RANGE. `.text` 0x804ABDD0-0x804ABF40; `.rodata` 0x805739A8-0x805739E0; `.data` 0x80618C40-0x80619408; `.sdata`
 *   0x80793DF8-0x80793E00; `.sbss` 0x80795148-0x80795150; `.sdata2` 0x8079D0D8-0x8079D0E0 (4 functions / 0x154
 *   B).
 *   - `.data` is the twelve message literals (Shift-JIS Japanese, Latin-1 European text), pooled by `-str reuse`;
 *     `.rodata` holds the two seven-language tables, `.sdata` the two-entry China/Korea table, `.sdata2` the
 *     white text colour, `.sbss` `FatalFunc`.
 * FLAGS. `cflags_base` (`Wii/1.3`, -O4,p) in the BTE-tail SDK block of `configure.py`.
 * NAMES. the unit name is a GUESS (named for what the messages do); `__DVDGetAutoFatalMessaging`,
 *   `__DVDPrintFatalMessage`, `__DVDErrorMessageDefault`, `__DVDErrorMessageEurope` and `FatalFunc` are SDK names;
 *   the `.sdata` table `__DVDErrorMessageChinaKorea` (game regions 4/5) is a GUESS. The map rows of the twelve
 *   `.data` literals and of `lbl_8079D0D8` (the `.sdata2` white text colour) keep their generated `lbl_` names
 *   (the source spells them as a literal and a local initialiser).
 * RESIDUALS. none (code and every claimed section byte-identical; the only differing bytes are alignment fill).
 * SHAPES. the region `switch` lists `default` first, then case 2, then cases 4/5 - that order lays the blocks out
 *   as the target does (an if-chain or another case order puts China/Korea first). The Japanese literals are
 *   UTF-8 in the source and become Shift-JIS through sjiswrap; the Latin-1 bytes are octal escapes.
 */
#include "DVD/dvdFatal.h"
#include "OS/OSFatal.h"
#include "OS/OSFont.h"
#include "OS/OSInterrupt.h"
#include "SC/SCApi.h"
#include "SC/SCProductInfo.h"
#include "gx.h"

static void (*FatalFunc)(void) = NULL;

/* The console languages (SCGetLanguage), in table order: Japanese, English, German, French, Spanish, Italian, Dutch. */
const char* const __DVDErrorMessageDefault[] = {
    "\n\n\nエラーが発生しました。\n"
    "\nイジェクトボタンを押してディスクを取り出してか\n"
    "ら、本体の電源をOFFにして、本体の取扱説明書の\n"
    "指示に従ってください。",
    "\n\n\nAn error has occurred.\n"
    "Press the Eject Button, remove the\n"
    "Game Disc, and turn off the power to the\n"
    "console. Please read the Wii Operations\n"
    "Manual for further instructions.",
    "\n\n\nEin Fehler ist aufgetreten.\n"
    "Dr\374cke den Ausgabeknopf, entnimm die\n"
    "Disc und schalte die Wii-Konsole aus.\n"
    "Bitte lies die Wii-Bedienungsanleitung,\n"
    "um weitere Informationen zu erhalten.",
    "\n\n\nUne erreur est survenue.\n"
    "Appuyez sur le bouton EJECT, retirez\n"
    "le disque et \351teignez la console.\n"
    "Veuillez vous r\351f\351rer au Mode d'emploi\n"
    "de la Wii pour plus de d\351tails.",
    "\n\n\nOcurri\363 un Error.\n"
    "Oprime el Bot\363n EJECT, saca el disco\n"
    "y apaga la consola. Consulta el manual\n"
    "de operaciones de la consola Wii para\n"
    "obtener m\341s informaci\363n.",
    "\n\n\nSi \350 verificato un errore.\n"
    "Premi il pulsante EJECT, estrai il disco\n"
    "e spegni la console. Per maggiori\n"
    "informazioni, consulta il manuale di\n"
    "istruzioni della console Wii.",
    "\n\n\nEr is een fout opgetreden.\n"
    "Druk op de EJECT-knop, verwijder de\n"
    "disk en zet het Wii-systeem uit. Lees\n"
    "de Wii-handleiding voor meer informatie.",
};

const char* const __DVDErrorMessageEurope[] = {
    "\n\n\nエラーが発生しました。\n"
    "\nイジェクトボタンを押してディスクを取り出してか\n"
    "ら、本体の電源をOFFにして、本体の取扱説明書の\n"
    "指示に従ってください。",
    "\n\n\nAn error has occurred.\n"
    "Press the EJECT Button, remove the Disc,\n"
    "and turn off the power to the console.\n"
    "Please refer to the Wii Operations Manual\n"
    "for details.",
    "\n\n\nEin Fehler ist aufgetreten.\n"
    "Dr\374cke den Ausgabeknopf, entnimm die\n"
    "Disc und schalte die Wii-Konsole aus.\n"
    "Bitte lies die Wii-Bedienungsanleitung,\n"
    "um weitere Informationen zu erhalten.",
    "\n\n\nUne erreur est survenue.\n"
    "Appuyez sur le bouton EJECT, retirez\n"
    "le disque et \351teignez la console.\n"
    "Veuillez vous r\351f\351rer au mode d'emploi\n"
    "Wii pour plus de d\351tails.",
    "\n\n\nSe ha producido un error.\n"
    "Pulsa el Bot\363n EJECT, extrae el disco y\n"
    "apaga la consola. Consulta el manual de\n"
    "instrucciones de la consola Wii para\n"
    "obtener m\341s informaci\363n.",
    "\n\n\nSi \350 verificato un errore.\n"
    "Premi il pulsante EJECT, estrai il disco\n"
    "e spegni la console. Per maggiori\n"
    "informazioni, consulta il manuale di\n"
    "istruzioni della console Wii.",
    "\n\n\nEr is een fout opgetreden.\n"
    "Druk op de EJECT-knop, verwijder de\n"
    "disk en zet het Wii-systeem uit. Lees\n"
    "de Wii-handleiding voor meer informatie.",
};

/* Korean and Chinese game regions: the Japanese-script and the English "error #104" messages. */
const char* __DVDErrorMessageChinaKorea[] = {
    "\n\nエラーコード１０４。\n"
    "エラーが発生しました。\n"
    "\nイジェクトボタンを押してディスクを取り出してか\n"
    "ら、本体の電源をOFFにして、本体の取扱説明書の\n"
    "指示に従ってください。",
    "\n\nError #104,\n"
    "An error has occurred.\n"
    "Press the EJECT Button, remove the\n"
    "Game Disc, and turn the power off.\n"
    "Please read the Wii Operations Manual\n"
    "for more information.",
};

/* 0x804ABDD0 (0xCC): picks the message for the game region and console language and halts through OSFatal. */
void __DVDShowFatalMessage(void) {
    const char* message;
    const char* const* messageList;
    GXColor bg = {0, 0, 0, 0};
    GXColor fg = {255, 255, 255, 0};

    if (SCGetLanguage() == 0) {
        OSSetFontEncode(1);
    } else {
        OSSetFontEncode(0);
    }

    switch (SCGetProductGameRegion()) {
    default:
        messageList = __DVDErrorMessageDefault;
        break;
    case 2:
        messageList = __DVDErrorMessageEurope;
        break;
    case 4:
    case 5:
        messageList = __DVDErrorMessageChinaKorea;
        break;
    }

    if (SCGetLanguage() > 6) {
        message = messageList[1];
    } else {
        message = messageList[SCGetLanguage()];
    }

    OSFatal(fg, bg, message);
}

/* 0x804ABEA0 (0x5C): installs (enable) or removes the fatal-message handler; returns whether one was installed. */
BOOL DVDSetAutoFatalMessaging(BOOL enable) {
    BOOL level;
    BOOL wasEnabled;

    level = OSDisableInterrupts();
    wasEnabled = FatalFunc ? TRUE : FALSE;
    FatalFunc = enable ? __DVDShowFatalMessage : NULL;
    OSRestoreInterrupts(level);
    return wasEnabled;
}

/* 0x804ABF00 (0x14): returns whether the fatal-message handler is installed. */
BOOL __DVDGetAutoFatalMessaging(void) {
    return FatalFunc ? TRUE : FALSE;
}

/* 0x804ABF20 (0x18): calls the installed fatal-message handler, if any. */
void __DVDPrintFatalMessage(void) {
    if (FatalFunc) {
        FatalFunc();
    }
}
