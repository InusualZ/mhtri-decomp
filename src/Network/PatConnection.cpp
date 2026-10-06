/*
 * Network/PatConnection.cpp - the Pat server connection: the packet table, the `PatConnection` class (connect and
 *   close state machines, the send/receive/dispatch loop) and its free entry points (`setServerAddress`,
 *   `sendCommand`, the `readUInt*`/`writeUInt*` body codecs, `flushBuffer`/`encryptBuffer`).
 * RANGE. .text 0x803FAE9C-0x803FCC34 (37 functions, address order; the last is the TU's static initialiser, the
 *   `.ctors` word 0x8056F3BC); .ctors 0x8056F3BC-0x8056F3C0, .data 0x805FCE50-0x80600978 (the packet descriptions,
 *   `patPacketTable`, the log strings, `__vt__13PatConnection`), .sdata 0x80793960-0x80793968 ("" and "%s"),
 *   extab, extabindex.  Each cut is a function start where the `.data` run, the `.sdata2` pool and the
 *   extab/extabindex tables all change owner together; the right edge is the start of `Network/PatInterface.cpp`.
 * FLAGS. `-O3 -inline noauto` (configure.py; docs/network.md), file-scope `#pragma peephole off` (the unfused
 *   `extsh`/`extsb` of the destructor and `readUInt8_`) and `#pragma pool_data off` (each log string of
 *   `connectServer` gets its own `lis`/`addi`).
 * NAMES. `patPacketTable` (the map's `PacketTable_BaseOffset_ID1`), `disableCrypt` (the dump's `resetVar60d0`) and
 *   the field names are GUESSes from the bodies; `checkBufError` and the static initialiser are named by the log
 *   string and the compiler.
 * RESIDUALS. none in `.text` (37 of 37 rows at 100, and a trial `Matching` link keeps main.dol's SHA-1); `.data`
 *   and `.sdata` are byte-identical but 4 and 1 bytes shorter than the claims: the tails are the alignment padding
 *   before `Network/PatInterface.cpp`'s 8-aligned sections, which `flipcheck` counts as missing.  Relocations to the
 *   log strings name our `@NNN` literals where the map has `lbl_` rows.
 * SHAPES. The table's null handlers are `0` member pointers: MWCC leaves their words zero in `.data` and the static
 *   initialiser copies `__ptmf_null` into each, which is retail's `.ctors` function.  The descriptions are Shift-JIS;
 *   a character whose second byte is 0x5C ("予") is written as hex escapes, because the compiler reads that byte
 *   as a backslash.  The library singleton `getNetworkLogger` returns is an `sNetworkLibrary` (`networkLibrary`
 *   below), but the 16-bit byte-order slots go through the `NetworkLogger` view, whose `u16` returns keep retail's
 *   unextended `sth`; the resolver/socket release and the pool calls take the same object under the band's other
 *   spellings.  The logged socket error is read into a function-scope `code` before the log call, and
 *   `connectServer`'s close step keeps the resolver in a function-scope local (both are retail's allocation).
 */

#include "Network/PatConnection.h"
#include "Network/NetworkStreamSink.h"      /* getNetworkLogger */
#include "Network/sNetworkLibrary.h"        /* sNetworkLibrary, networkSocketPool_*, networkLog_destroyContext */
#include "Network/NetworkFileFetcher.h"     /* NetworkSocketBase */
#include "Network/NetworkSocketWii.h"       /* networkSocketHandle_getLastError */
#include "Network/network_pat_control.h"    /* PatCryptSetKey, PatCryptEncrypt, PatCryptDecrypt */
#include "unsplit/Network.h"                /* NetworkLogger - the 16-bit byte-order slots */
#include "MSL_C/alloc.h"                    /* memmove, snprintf, rand */
#include "MSL/strlen.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off
#pragma pool_data off

/* The network library the band's logger accessor returns. */
static inline sNetworkLibrary* networkLibrary()
{
    return (sNetworkLibrary*)getNetworkLogger();
}

/* ==== the packet table ======================================================================================== */

PatPacketEntry patPacketTable[298] = {
    { { 0x60, 0x01, 0x01 }, &PatConnection::recvReqLineCheck, "ラインチェック ", "" },
    { { 0x60, 0x01, 0x02 }, 0, "ラインチェック ", "" },
    { { 0x60, 0x02, 0x01 }, 0, "サーバ時刻要求 ", "" },
    { { 0x60, 0x02, 0x02 }, &PatConnection::recvAnsServerTime, "サーバ時刻返答 ", "" },
    { { 0x60, 0x10, 0x01 }, 0, "切断要求 ", "" },
    { { 0x60, 0x10, 0x02 }, &PatConnection::recvAnsShut, "切断返答 ", "" },
    { { 0x60, 0x10, 0x10 }, &PatConnection::recvNtcShut, "切断通知 ", "" },
    { { 0x60, 0x11, 0x10 }, &PatConnection::recvNtcRecconect, "再接続通知 ", "" },
    { { 0x60, 0x20, 0x01 }, &PatConnection::recvReqConnection, "ＰＡＴ接続環境要求 ", "" },
    { { 0x60, 0x20, 0x02 }, 0, "ＰＡＴ接続環境返答 ", "" },
    { { 0x60, 0x21, 0x10 }, &PatConnection::recvNtcLogin, "ログイン処理概要通知 ", "" },
    { { 0x60, 0x30, 0x01 }, 0, "ＰＡＴチケット要求 ", "" },
    { { 0x60, 0x30, 0x02 }, &PatConnection::recvAnsTicket, "ＰＡＴチケット返答 ", "" },
    { { 0x60, 0x31, 0x01 }, &PatConnection::recvReqTicket, "ＰＡＴチケット送信 ", "" },
    { { 0x60, 0x31, 0x02 }, 0, "ＰＡＴチケット受信 ", "" },
    { { 0x60, 0x40, 0x01 }, &PatConnection::recvReqWarning, "警告文送信 ", "" },
    { { 0x60, 0x40, 0x02 }, 0, "警告文受信確認 ", "" },
    { { 0x60, 0x50, 0x10 }, 0, "収集ログ通知 ", "" },
    { { 0x60, 0x70, 0x01 }, 0, "共通鍵要求 ", "" },
    { { 0x60, 0x70, 0x02 }, &PatConnection::recvAnsCommonKey, "共通鍵返答 ", "" },
    { { 0x60, 0x80, 0x10 }, 0, "チートチェックデータ送信 ", "" },
    { { 0x60, 0x81, 0x01 }, &PatConnection::recvReqMemoryCheck, "メモリ内容要求 ", "" },
    { { 0x60, 0x81, 0x02 }, 0, "メモリ内容送信 ", "" },
    { { 0x61, 0x01, 0x01 }, 0, "ログイン情報送信 ", "" },
    { { 0x61, 0x01, 0x02 }, &PatConnection::recvAnsLoginInfo, "ログイン情報返信 ", "" },
    { { 0x61, 0x02, 0x01 }, 0, "課金情報要求 ", "" },
    { { 0x61, 0x02, 0x02 }, &PatConnection::recvAnsChargeInfo, "課金情報返答 ", "" },
    { { 0x61, 0x10, 0x01 }, 0, "PAT ID候補数要求 ", "" },
    { { 0x61, 0x10, 0x02 }, &PatConnection::recvAnsUserListHead, "PAT ID候補数応答 ", "" },
    { { 0x61, 0x11, 0x01 }, 0, "PAT ID候補要求 ", "" },
    { { 0x61, 0x11, 0x02 }, &PatConnection::recvAnsUserListData, "PAT ID候補送信 ", "" },
    { { 0x61, 0x12, 0x01 }, 0, "PAT ID候補送信終了確認 ", "" },
    { { 0x61, 0x12, 0x02 }, &PatConnection::recvAnsUserListFoot, "PAT ID候補送信終了返答 ", "" },
    { { 0x61, 0x20, 0x01 }, 0, "ユーザオブジェクト送信 ", "" },
    { { 0x61, 0x20, 0x02 }, &PatConnection::recvAnsUserObject, "ユーザオブジェクト結果 ", "" },
    { { 0x61, 0x30, 0x01 }, 0, "FMPリストバージョン確認 ", "" },
    { { 0x61, 0x30, 0x02 }, &PatConnection::recvAnsFmpListVersion, "FMPリストバージョン確認応答 ", "" },
    { { 0x61, 0x31, 0x01 }, 0, "FMPリスト数送信 ", "" },
    { { 0x61, 0x31, 0x02 }, &PatConnection::recvAnsFmpListHead, "FMPリスト数応答 ", "" },
    { { 0x61, 0x32, 0x01 }, 0, "FMPリスト送信 ", "" },
    { { 0x61, 0x32, 0x02 }, &PatConnection::recvAnsFmpListData, "FMPリスト応答 ", "" },
    { { 0x61, 0x33, 0x01 }, 0, "FMPリスト送信終了 ", "" },
    { { 0x61, 0x33, 0x02 }, &PatConnection::recvAnsFmpListFoot, "FMPリスト送信終了 ", "" },
    { { 0x61, 0x34, 0x01 }, 0, "FMPデータ要求 ", "" },
    { { 0x61, 0x34, 0x02 }, &PatConnection::recvAnsFmpInfo, "FMPデータ返答 ", "" },
    { { 0x61, 0x40, 0x01 }, 0, "RFPの接続先要求 ", "" },
    { { 0x61, 0x40, 0x02 }, &PatConnection::recvAnsRfpConnect, "RFPの接続先応答 ", "" },
    { { 0x62, 0x01, 0x01 }, 0, "LMPの接続先要求 ", "" },
    { { 0x62, 0x01, 0x02 }, &PatConnection::recvAnsLmpConnect, "LMPの接続先応答 ", "" },
    { { 0x62, 0x10, 0x01 }, 0, "利用規約情報確認 ", "" },
    { { 0x62, 0x10, 0x02 }, &PatConnection::recvAnsTermsVersion, "利用規約情報応答 ", "" },
    { { 0x62, 0x11, 0x01 }, 0, "利用規約要求 ", "" },
    { { 0x62, 0x11, 0x02 }, &PatConnection::recvAnsTerms, "利用規約応答 ", "" },
    { { 0x62, 0x20, 0x01 }, 0, "メンテナンス情報要求 ", "" },
    { { 0x62, 0x20, 0x02 }, &PatConnection::recvAnsMaintenance, "メンテナンス情報通知 ", "" },
    { { 0x62, 0x30, 0x01 }, 0, "お知らせ要求 ", "" },
    { { 0x62, 0x30, 0x02 }, &PatConnection::recvAnsAnnounce, "お知らせ通知 ", "" },
    { { 0x62, 0x31, 0x01 }, 0, "未課金メッセージ要求 ", "" },
    { { 0x62, 0x31, 0x02 }, &PatConnection::recvAnsNoCharge, "未課金メッセージ通知 ", "" },
    { { 0x62, 0x41, 0x01 }, 0, "新メディアバージョン送信 ", "" },
    { { 0x62, 0x41, 0x02 }, &PatConnection::recvAnsMediaVersionInfo, "新メディアバージョン返答 ", "" },
    { { 0x62, 0x54, 0x01 }, 0, "真・名前用禁止文言要求 ", "" },
    { { 0x62, 0x54, 0x02 }, &PatConnection::recvAnsVulgarityInfoHigh, "真・名前用禁止文言応答 ", "" },
    { { 0x62, 0x55, 0x01 }, 0, "真・名前用禁止文言取得要求 ", "" },
    { { 0x62, 0x55, 0x02 }, &PatConnection::recvAnsVulgarityHigh, "真・名前用禁止文言取得応答 ", "" },
    { { 0x62, 0x56, 0x01 }, 0, "真・名前以外用禁止文言要求 ", "" },
    { { 0x62, 0x56, 0x02 }, &PatConnection::recvAnsVulgarityInfoLow, "真・名前以外用禁止文言応答 ", "" },
    { { 0x62, 0x57, 0x01 }, 0, "真・名前以外用禁止文言取得要求 ", "" },
    { { 0x62, 0x57, 0x02 }, &PatConnection::recvAnsVulgarityLow, "真・名前以外用禁止文言取得応答 ", "" },
    { { 0x62, 0x60, 0x01 }, 0, "認証トークン送信 ", "" },
    { { 0x62, 0x60, 0x02 }, &PatConnection::recvAnsAuthenticationToken, "認証トークン返答 ", "" },
    { { 0x63, 0x01, 0x01 }, 0, "バイナリバージョン確認 ", "" },
    { { 0x63, 0x01, 0x02 }, &PatConnection::recvAnsBinaryVersion, "バイナリバージョン確認応答 ", "" },
    { { 0x63, 0x02, 0x01 }, 0, "バイナリデータ開始要求 ", "" },
    { { 0x63, 0x02, 0x02 }, &PatConnection::recvAnsBinaryHead, "バイナリデータ開始応答 ", "" },
    { { 0x63, 0x03, 0x01 }, 0, "バイナリデータ要求 ", "" },
    { { 0x63, 0x03, 0x02 }, &PatConnection::recvAnsBinaryData, "バイナリデータ応答 ", "" },
    { { 0x63, 0x04, 0x01 }, 0, "バイナリデータ完了要求 ", "" },
    { { 0x63, 0x04, 0x02 }, &PatConnection::recvAnsBinarFoot, "バイナリデータ完了応答 ", "" },
    { { 0x63, 0x10, 0x01 }, 0, "FMPリストバージョン確認 ", "" },
    { { 0x63, 0x10, 0x02 }, &PatConnection::recvAnsFmpListVersion, "FMPリストバージョン確認応答 ", "" },
    { { 0x63, 0x11, 0x01 }, 0, "FMPリスト数要求 ", "" },
    { { 0x63, 0x11, 0x02 }, &PatConnection::recvAnsFmpListHead, "FMPリスト数応答 ", "" },
    { { 0x63, 0x12, 0x01 }, 0, "FMPリスト要求 ", "" },
    { { 0x63, 0x12, 0x02 }, &PatConnection::recvAnsFmpListData, "FMPリスト応答 ", "" },
    { { 0x63, 0x13, 0x01 }, 0, "FMPリスト終了送信 ", "" },
    { { 0x63, 0x13, 0x02 }, &PatConnection::recvAnsFmpListFoot, "FMPリスト終了返答 ", "" },
    { { 0x63, 0x14, 0x01 }, 0, "FMPデータ要求 ", "" },
    { { 0x63, 0x14, 0x02 }, &PatConnection::recvAnsFmpInfo, "FMPデータ返答 ", "" },
    { { 0x64, 0x01, 0x01 }, 0, "レイヤ開始要求 ", "" },
    { { 0x64, 0x01, 0x02 }, &PatConnection::recvAnsLayerStart, "レイヤ開始応答 ", "" },
    { { 0x64, 0x02, 0x01 }, 0, "レイヤ終了要求 ", "" },
    { { 0x64, 0x02, 0x02 }, &PatConnection::recvAnsLayerEnd, "レイヤ終了応答 ", "" },
    { { 0x64, 0x03, 0x10 }, &PatConnection::recvNtcLayerUserNum, "レイヤ人数通知 ", "" },
    { { 0x64, 0x10, 0x01 }, 0, "レイヤ移動要求（位置バイナリ） ", "" },
    { { 0x64, 0x10, 0x02 }, &PatConnection::recvAnsLayerJump, "レイヤ移動返答（位置バイナリ） ", "" },
    { { 0x64, 0x11, 0x01 }, 0, "レイヤ作成要求（番号指定） ", "" },
    { { 0x64, 0x11, 0x02 }, &PatConnection::recvAnsLayerCreateHead, "レイヤ作成返答 ", "" },
    { { 0x64, 0x12, 0x01 }, 0, "レイヤ作成設定要求（番号指定） ", "" },
    { { 0x64, 0x12, 0x02 }, &PatConnection::recvAnsLayerCreateSet, "レイヤ作成設定返答 ", "" },
    { { 0x64, 0x13, 0x01 }, 0, "レイヤ作成完了要求（番号指定） ", "" },
    { { 0x64, 0x13, 0x02 }, &PatConnection::recvAnsLayerCreateFoot, "レイヤ作成完了返答 ", "" },
    { { 0x64, 0x14, 0x01 }, 0, "レイヤダウン要求（番号指定） ", "" },
    { { 0x64, 0x14, 0x02 }, &PatConnection::recvAnsLayerDown, "レイヤダウン返答 ", "" },
    { { 0x64, 0x14, 0x10 }, &PatConnection::recvNtcLayerIn, "レイヤイン通知 ", "" },
    { { 0x64, 0x15, 0x01 }, 0, "レイヤアップ要求 ", "" },
    { { 0x64, 0x15, 0x02 }, &PatConnection::recvAnsLayerUp, "レイヤアップ返答 ", "" },
    { { 0x64, 0x15, 0x10 }, &PatConnection::recvNtcLayerOut, "レイヤアウト返答 ", "" },
    { { 0x64, 0x16, 0x01 }, 0, "レイヤ\x97\x5C約移動確認要求 ", "" },
    { { 0x64, 0x16, 0x02 }, &PatConnection::recvNtcLayerJumpReady, "レイヤ\x97\x5C約移動確認返答 ", "" },
    { { 0x64, 0x17, 0x01 }, 0, "レイヤ\x97\x5C約移動実行要求 ", "" },
    { { 0x64, 0x17, 0x02 }, &PatConnection::recvNtcLayerJumpGo, "レイヤ\x97\x5C約移動実行返答 ", "" },
    { { 0x64, 0x20, 0x01 }, 0, "レイヤ情報設定要求 ", "" },
    { { 0x64, 0x20, 0x02 }, &PatConnection::recvAnsLayerInfoSe, "レイヤ情報設定返答 ", "" },
    { { 0x64, 0x20, 0x10 }, &PatConnection::recvNtcLayerInfoSet, "レイヤ情報設定通知 ", "" },
    { { 0x64, 0x21, 0x01 }, 0, "レイヤ情報要求 ", "" },
    { { 0x64, 0x21, 0x02 }, &PatConnection::recvAnsLayerInfo, "レイヤ情報返答 ", "" },
    { { 0x64, 0x22, 0x01 }, 0, "親レイヤ情報要求 ", "" },
    { { 0x64, 0x22, 0x02 }, &PatConnection::recvAnsLayerParentInfo, "親レイヤ情報返答 ", "" },
    { { 0x64, 0x23, 0x01 }, 0, "子レイヤ情報要求 ", "" },
    { { 0x64, 0x23, 0x02 }, &PatConnection::recvAnsLayerChildInfo, "子レイヤ情報返答 ", "" },
    { { 0x64, 0x24, 0x01 }, 0, "子レイヤリスト数要求 ", "" },
    { { 0x64, 0x24, 0x02 }, &PatConnection::recvAnsLayerChildListHead, "子レイヤリスト数返答 ", "" },
    { { 0x64, 0x25, 0x01 }, 0, "子レイヤリスト要求 ", "" },
    { { 0x64, 0x25, 0x02 }, &PatConnection::recvAnsLayerChildListData, "子レイヤリスト返答 ", "" },
    { { 0x64, 0x26, 0x01 }, 0, "子レイヤリスト終了要求 ", "" },
    { { 0x64, 0x26, 0x02 }, &PatConnection::recvAnsLayerChildListFoot, "子レイヤリスト終了返答 ", "" },
    { { 0x64, 0x27, 0x01 }, 0, "兄弟レイヤリスト数要求 ", "" },
    { { 0x64, 0x27, 0x02 }, &PatConnection::recvAnsLayerSiblingListHead, "兄弟レイヤリスト数返答 ", "" },
    { { 0x64, 0x28, 0x01 }, 0, "兄弟レイヤリスト要求 ", "" },
    { { 0x64, 0x28, 0x02 }, &PatConnection::recvAnsLayerSiblingListData, "兄弟レイヤリスト返答 ", "" },
    { { 0x64, 0x29, 0x01 }, 0, "兄弟レイヤリスト終了要求 ", "" },
    { { 0x64, 0x29, 0x02 }, &PatConnection::recvAnsLayerSiblingListFoot, "兄弟レイヤリスト終了返答 ", "" },
    { { 0x64, 0x41, 0x01 }, 0, "レイヤのホスト者要求 ", "" },
    { { 0x64, 0x41, 0x02 }, &PatConnection::recvAnsLayerHost, "レイヤのホスト者返答 ", "" },
    { { 0x64, 0x41, 0x10 }, &PatConnection::recvNtcLayerHost, "レイヤのホスト通知 ", "" },
    { { 0x64, 0x60, 0x01 }, 0, "レイヤユーザデータ設定要求 ", "" },
    { { 0x64, 0x60, 0x02 }, &PatConnection::recvAnsLayerUserInfoSet, "レイヤユーザデータ設定返答 ", "" },
    { { 0x64, 0x60, 0x10 }, &PatConnection::recvNtcLayerUserInfoSet, "レイヤユーザデータ設定通知 ", "" },
    { { 0x64, 0x63, 0x01 }, 0, "レイヤ同期ユーザリスト要求 ", "" },
    { { 0x64, 0x63, 0x02 }, &PatConnection::recvAnsLayerUserList, "レイヤ同期ユーザリスト返答 ", "" },
    { { 0x64, 0x64, 0x01 }, 0, "レイヤユーザリスト数要求 ", "" },
    { { 0x64, 0x64, 0x02 }, &PatConnection::recvAnsLayerUserListHead, "レイヤユーザリスト数返答 ", "" },
    { { 0x64, 0x65, 0x01 }, 0, "レイヤユーザリスト要求 ", "" },
    { { 0x64, 0x65, 0x02 }, &PatConnection::recvAnsLayerUserListData, "レイヤユーザリスト返答 ", "" },
    { { 0x64, 0x66, 0x01 }, 0, "レイヤユーザリスト終了要求 ", "" },
    { { 0x64, 0x66, 0x02 }, &PatConnection::recvAnsLayerUserListFoot, "レイヤユーザリスト終了返答 ", "" },
    { { 0x64, 0x67, 0x01 }, 0, "レイヤユーザ検索リスト数要求 ", "" },
    { { 0x64, 0x67, 0x02 }, &PatConnection::recvAnsLayerUserSearchHead, "レイヤユーザ検索リスト数返答 ", "" },
    { { 0x64, 0x68, 0x01 }, 0, "レイヤユーザ検索リスト要求 ", "" },
    { { 0x64, 0x68, 0x02 }, &PatConnection::recvAnsLayerUserSearchData, "レイヤユーザ検索リスト返答 ", "" },
    { { 0x64, 0x69, 0x01 }, 0, "レイヤユーザ検索リスト終了要求 ", "" },
    { { 0x64, 0x69, 0x02 }, &PatConnection::recvAnsLayerUserSearchFoot, "レイヤユーザ検索リスト終了返答 ", "" },
    { { 0x64, 0x70, 0x10 }, &PatConnection::recvNtcLayerBinary, "レイヤユーザ用バイナリ通知 ", "" },
    { { 0x64, 0x70, 0x10 }, 0, "レイヤユーザ用バイナリ送信 ", "" },
    { { 0x64, 0x71, 0x10 }, &PatConnection::recvNtcLayerUserPosition, "レイヤゲームポジション受信通知 ", "" },
    { { 0x64, 0x71, 0x10 }, 0, "レイヤゲームポジション通知 ", "" },
    { { 0x64, 0x72, 0x10 }, &PatConnection::recvNtcLayerChat, "レイヤチャット通知 ", "" },
    { { 0x64, 0x72, 0x10 }, 0, "レイヤチャット送信 ", "" },
    { { 0x64, 0x73, 0x01 }, 0, "レイヤ相手指定チャット送信 ", "" },
    { { 0x64, 0x73, 0x02 }, &PatConnection::recvAnsLayerTell, "レイヤ相手指定チャット返信 ", "" },
    { { 0x64, 0x73, 0x10 }, &PatConnection::recvNtcLayerTell, "レイヤ相手指定チャット通知 ", "" },
    { { 0x64, 0x74, 0x10 }, &PatConnection::recvNtcLayerTellLow, "レイヤ相手指定チャット通知 (通知のみ) ", "" },
    { { 0x64, 0x74, 0x10 }, 0, "レイヤ相手指定チャット送信 (通知のみ) ", "" },
    { { 0x64, 0x75, 0x10 }, &PatConnection::recvNtcLayerBinary, "レイヤユーザ用バイナリ通知 (相手指定) ", "" },
    { { 0x64, 0x75, 0x10 }, 0, "レイヤユーザ用バイナリ送信 (相手指定) ", "" },
    { { 0x64, 0x80, 0x01 }, 0, "レイヤ調停データ確保要求 ", "" },
    { { 0x64, 0x80, 0x02 }, &PatConnection::recvAnsLayerMediationLock, "レイヤ調停データ確保返答 ", "" },
    { { 0x64, 0x80, 0x10 }, &PatConnection::recvNtcLayerMediationLock, "レイヤ調停データ確保通知 ", "" },
    { { 0x64, 0x81, 0x01 }, 0, "レイヤ調停データ開放要求 ", "" },
    { { 0x64, 0x81, 0x02 }, &PatConnection::recvAnsLayerMediationUnlock, "レイヤ調停データ開放返答 ", "" },
    { { 0x64, 0x81, 0x10 }, &PatConnection::recvNtcLayerMediationUnlock, "レイヤ調停データ開放通知 ", "" },
    { { 0x64, 0x82, 0x01 }, 0, "レイヤ調停データリスト取得要求 ", "" },
    { { 0x64, 0x82, 0x02 }, &PatConnection::recvAnsLayerMediationList, "レイヤ調停データリスト取得返答 ", "" },
    { { 0x64, 0x90, 0x01 }, 0, "レイヤ検索詳細数要求 ", "" },
    { { 0x64, 0x90, 0x02 }, &PatConnection::recvAnsLayerDetailSearchHead, "レイヤ検索詳細数返答 ", "" },
    { { 0x64, 0x91, 0x01 }, 0, "レイヤ検索詳細データ要求 ", "" },
    { { 0x64, 0x91, 0x02 }, &PatConnection::recvAnsLayerDetailSearchData, "レイヤ検索詳細データ返答 ", "" },
    { { 0x64, 0x92, 0x01 }, 0, "レイヤ検索詳細終了要求 ", "" },
    { { 0x64, 0x92, 0x02 }, &PatConnection::recvAnsLayerDetailSearchFoot, "レイヤ検索詳細終了返答 ", "" },
    { { 0x65, 0x01, 0x01 }, 0, "サークル作成要求 ", "" },
    { { 0x65, 0x01, 0x02 }, &PatConnection::recvAnsCircleCreate, "サークル作成返答 ", "" },
    { { 0x65, 0x02, 0x01 }, 0, "サークルデータ取得要求 ", "" },
    { { 0x65, 0x02, 0x02 }, &PatConnection::recvAnsCircleInfo, "サークルデータ取得返答 ", "" },
    { { 0x65, 0x03, 0x01 }, 0, "サークルイン要求 ", "" },
    { { 0x65, 0x03, 0x02 }, &PatConnection::recvAnsCircleJoin, "サークルイン返答 ", "" },
    { { 0x65, 0x03, 0x10 }, &PatConnection::recvNtcCircleJoin, "サークルイン通知 ", "" },
    { { 0x65, 0x04, 0x01 }, 0, "サークルアウト要求 ", "" },
    { { 0x65, 0x04, 0x02 }, &PatConnection::recvAnsCircleLeave, "サークルアウト返答 ", "" },
    { { 0x65, 0x04, 0x10 }, &PatConnection::recvNtcCircleLeave, "サークルアウト通知 ", "" },
    { { 0x65, 0x05, 0x01 }, 0, "サークル解散要求 ", "" },
    { { 0x65, 0x05, 0x02 }, &PatConnection::recvAnsCircleBreak, "サークル解散返答 ", "" },
    { { 0x65, 0x05, 0x10 }, &PatConnection::recvNtcCircleBreak, "サークル解散通知 ", "" },
    { { 0x65, 0x10, 0x01 }, 0, "マッチングオプション設定要求 ", "" },
    { { 0x65, 0x10, 0x02 }, &PatConnection::recvAnsCircleMatchOptionSet, "マッチングオプション設定返答 ", "" },
    { { 0x65, 0x10, 0x10 }, &PatConnection::recvNtcCircleMatchOptionSet, "マッチングオプション設定通知 ", "" },
    { { 0x65, 0x11, 0x01 }, 0, "マッチングオプション取得要求 ", "" },
    { { 0x65, 0x11, 0x02 }, &PatConnection::recvAnsCircleMatchOptionGet, "マッチングオプション取得返答 ", "" },
    { { 0x65, 0x12, 0x01 }, 0, "マッチング開始要求 ", "" },
    { { 0x65, 0x12, 0x02 }, &PatConnection::recvAnsCircleMatchStart, "マッチング開始返答 ", "" },
    { { 0x65, 0x12, 0x10 }, &PatConnection::recvNtcCircleMatchStart, "マッチング開始通知 ", "" },
    { { 0x65, 0x13, 0x01 }, 0, "マッチング終了要求 ", "" },
    { { 0x65, 0x13, 0x02 }, &PatConnection::recvAnsCircleMatchEnd, "マッチング終了返答 ", "" },
    { { 0x65, 0x20, 0x01 }, 0, "サークルデータ設定要求 ", "" },
    { { 0x65, 0x20, 0x02 }, &PatConnection::recvAnsCircleInfoSet, "サークルデータ設定返答 ", "" },
    { { 0x65, 0x20, 0x10 }, &PatConnection::recvNtcCircleInfoSet, "サークルデータ設定通知 ", "" },
    { { 0x65, 0x27, 0x01 }, 0, "サークル同期リスト要求 (レイヤ) ", "" },
    { { 0x65, 0x27, 0x02 }, &PatConnection::recvAnsCircleListLayer, "サークル同期リスト返答 (レイヤ) ", "" },
    { { 0x65, 0x28, 0x01 }, 0, "サークル検索数要求 ", "" },
    { { 0x65, 0x28, 0x02 }, &PatConnection::recvAnsCircleSearchHead, "サークル検索数返答 ", "" },
    { { 0x65, 0x29, 0x01 }, 0, "サークル検索要求 ", "" },
    { { 0x65, 0x29, 0x02 }, &PatConnection::recvAnsCircleSearchData, "サークル検索返答 ", "" },
    { { 0x65, 0x2A, 0x01 }, 0, "サークル検索終了要求 ", "" },
    { { 0x65, 0x2A, 0x02 }, &PatConnection::recvAnsCircleSearchFoot, "サークル検索終了返答 ", "" },
    { { 0x65, 0x35, 0x01 }, 0, "サークルからキック要求 ", "" },
    { { 0x65, 0x35, 0x02 }, &PatConnection::recvAnsCircleKick, "サークルからキック返答 ", "" },
    { { 0x65, 0x35, 0x10 }, &PatConnection::recvNtcCircleKick, "サークルからキック通知 ", "" },
    { { 0x65, 0x36, 0x01 }, 0, "キックリストから削除要求 ", "" },
    { { 0x65, 0x36, 0x02 }, &PatConnection::recvAnsCircleDeleteKickList, "キックリストから削除返答 ", "" },
    { { 0x65, 0x40, 0x01 }, 0, "サークルのホスト移譲要求 ", "" },
    { { 0x65, 0x40, 0x02 }, &PatConnection::recvAnsCircleHostHandover, "サークルのホスト移譲返答 ", "" },
    { { 0x65, 0x40, 0x10 }, &PatConnection::recvNtcCircleHostHandover, "サークルのホスト移譲通知 ", "" },
    { { 0x65, 0x41, 0x01 }, 0, "サークルのホスト者要求 ", "" },
    { { 0x65, 0x41, 0x02 }, &PatConnection::recvAnsCircleHost, "サークルのホスト者返答 ", "" },
    { { 0x65, 0x41, 0x10 }, &PatConnection::recvNtcCircleHost, "サークルのホスト通知 ", "" },
    { { 0x65, 0x60, 0x01 }, 0, "サークル同期ユーザリスト要求 ", "" },
    { { 0x65, 0x60, 0x02 }, &PatConnection::recvAnsCircleUserList, "サークル同期ユーザリスト返答 ", "" },
    { { 0x65, 0x70, 0x10 }, &PatConnection::recvNtcCircleBinary, "サークルバイナリ通知 ", "" },
    { { 0x65, 0x70, 0x10 }, 0, "サークルバイナリ送信 ", "" },
    { { 0x65, 0x71, 0x10 }, &PatConnection::recvNtcCircleBinary, "サークルバイナリ通知 (相手指定) ", "" },
    { { 0x65, 0x71, 0x10 }, 0, "サークルバイナリ送信 (相手指定) ", "" },
    { { 0x65, 0x72, 0x10 }, &PatConnection::recvNtcChat, "サークルチャット通知 ", "" },
    { { 0x65, 0x72, 0x10 }, 0, "サークルチャット送信 ", "" },
    { { 0x65, 0x73, 0x01 }, 0, "サークル相手指定チャット送信 ", "" },
    { { 0x65, 0x73, 0x02 }, &PatConnection::recvAnsCircleTell, "サークル相手指定チャット返信 ", "" },
    { { 0x65, 0x73, 0x10 }, &PatConnection::recvNtcCircleTell, "サークル相手指定チャット通知 ", "" },
    { { 0x65, 0x80, 0x01 }, 0, "サークル通知定義登録要求 ", "" },
    { { 0x65, 0x80, 0x02 }, &PatConnection::recvAnsCircleInfoNoticeSet, "サークル通知定義登録返答 ", "" },
    { { 0x65, 0x81, 0x10 }, &PatConnection::recvNtcCircleListLayerCreate, "サークル追加通知 (レイヤ) ", "" },
    { { 0x65, 0x82, 0x10 }, &PatConnection::recvNtcCircleListLayerChange, "サークル変更通知 (レイヤ) ", "" },
    { { 0x65, 0x83, 0x10 }, &PatConnection::recvNtcCircleListLayerDelete, "サークル削除通知 (レイヤ) ", "" },
    { { 0x65, 0x90, 0x01 }, 0, "MCS作成要求 ", "" },
    { { 0x65, 0x90, 0x02 }, &PatConnection::recvAnsMcsCreate, "MCS作成返答 ", "" },
    { { 0x65, 0x90, 0x10 }, &PatConnection::recvNtcMcsCreate, "MCS作成通知 ", "" },
    { { 0x65, 0x91, 0x10 }, &PatConnection::recvNtcMcsStart, "MCS移行通知 ", "" },
    { { 0x66, 0x11, 0x01 }, 0, "相手指定チャット送信 ", "" },
    { { 0x66, 0x11, 0x02 }, &PatConnection::recvAnsTell, "相手指定チャット返信 ", "" },
    { { 0x66, 0x11, 0x10 }, &PatConnection::recvNtcTell, "相手指定チャット通知 ", "" },
    { { 0x66, 0x12, 0x01 }, 0, "相手指定バイナリ要求 ", "" },
    { { 0x66, 0x12, 0x02 }, &PatConnection::recvAnsBinaryUser, "相手指定バイナリ返答 ", "" },
    { { 0x66, 0x12, 0x10 }, &PatConnection::recvNtcBinaryUser, "相手指定バイナリ通知 ", "" },
    { { 0x66, 0x13, 0x10 }, &PatConnection::recvNtcBinaryServer, "サーババイナリ通知 ", "" },
    { { 0x66, 0x30, 0x01 }, 0, "ユーザ検索設定要求 ", "" },
    { { 0x66, 0x30, 0x02 }, &PatConnection::recvAnsUserSearchSet, "ユーザ検索設定返答 ", "" },
    { { 0x66, 0x31, 0x01 }, 0, "ユーザ\x95\x5C示用バイナリ設定要求 ", "" },
    { { 0x66, 0x31, 0x02 }, &PatConnection::recvAnsUserBinarySet, "ユーザ\x95\x5C示用バイナリ設定返答 ", "" },
    { { 0x66, 0x32, 0x01 }, 0, "ユーザ\x95\x5C示用バイナリ通知要求 ", "" },
    { { 0x66, 0x32, 0x02 }, &PatConnection::recvAnsUserBinaryNotice, "ユーザ\x95\x5C示用バイナリ通知返答 ", "" },
    { { 0x66, 0x32, 0x10 }, &PatConnection::recvNtcUserBinaryNotice, "ユーザ\x95\x5C示用バイナリ通知通知 ", "" },
    { { 0x66, 0x33, 0x01 }, 0, "ユーザ検索数要求 ", "" },
    { { 0x66, 0x33, 0x02 }, &PatConnection::recvAnsUserSearchHead, "ユーザ検索数返答 ", "" },
    { { 0x66, 0x34, 0x01 }, 0, "ユーザ検索要求 ", "" },
    { { 0x66, 0x34, 0x02 }, &PatConnection::recvAnsUserSearchData, "ユーザ検索返答 ", "" },
    { { 0x66, 0x35, 0x01 }, 0, "ユーザ検索終了要求 ", "" },
    { { 0x66, 0x35, 0x02 }, &PatConnection::recvAnsUserSearchFoot, "ユーザ検索終了返答 ", "" },
    { { 0x66, 0x36, 0x01 }, 0, "ユーザ検索データ要求 ", "" },
    { { 0x66, 0x36, 0x02 }, &PatConnection::recvAnsUserSearchInfo, "ユーザ検索データ返答 ", "" },
    { { 0x66, 0x37, 0x01 }, 0, "ユーザ検索データ要求(自分) ", "" },
    { { 0x66, 0x37, 0x02 }, &PatConnection::recvAnsUserSearchInfoMine, "ユーザ検索データ返答(自分) ", "" },
    { { 0x66, 0x40, 0x01 }, 0, "ユーザステータス設定要求 ", "" },
    { { 0x66, 0x40, 0x02 }, &PatConnection::recvAnsUserStatusSet, "ユーザステータス設定返答 ", "" },
    { { 0x66, 0x41, 0x01 }, 0, "ユーザステータス要求 ", "" },
    { { 0x66, 0x41, 0x02 }, &PatConnection::recvAnsUserStatus, "ユーザステータス返答 ", "" },
    { { 0x66, 0x50, 0x01 }, 0, "フレンド登録要求 ", "" },
    { { 0x66, 0x50, 0x02 }, &PatConnection::recvAnsFriendAdd, "フレンド登録返答 ", "" },
    { { 0x66, 0x50, 0x10 }, &PatConnection::recvNtcFriendAdd, "フレンド登録完了通知 ", "" },
    { { 0x66, 0x51, 0x01 }, 0, "フレンド登録依頼返答要求 ", "" },
    { { 0x66, 0x51, 0x02 }, &PatConnection::recvAnsFriendAccept, "フレンド登録依頼返答返答 ", "" },
    { { 0x66, 0x51, 0x10 }, &PatConnection::recvNtcFriendAccept, "フレンド登録依頼通知 ", "" },
    { { 0x66, 0x53, 0x01 }, 0, "フレンドデータ削除要求 ", "" },
    { { 0x66, 0x53, 0x02 }, &PatConnection::recvAnsFriendDelete, "フレンドデータ削除返答 ", "" },
    { { 0x66, 0x54, 0x01 }, 0, "フレンドリスト要求 ", "" },
    { { 0x66, 0x54, 0x02 }, &PatConnection::recvAnsFriendList, "フレンドリスト返答 ", "" },
    { { 0x66, 0x60, 0x01 }, 0, "ブラックデータ登録要求 ", "" },
    { { 0x66, 0x60, 0x02 }, &PatConnection::recvAnsBlackAdd, "ブラックデータ登録返答 ", "" },
    { { 0x66, 0x61, 0x01 }, 0, "ブラックデータ削除要求 ", "" },
    { { 0x66, 0x61, 0x02 }, &PatConnection::recvAnsBlackDelete, "ブラックデータ削除返答 ", "" },
    { { 0x66, 0x62, 0x01 }, 0, "ブラックリスト要求 ", "" },
    { { 0x66, 0x62, 0x02 }, &PatConnection::recvAnsBlackList, "ブラックリスト返答 ", "" },
    { { 0x69, 0x01, 0x01 }, 0, "自動同意ページ数要求 ", "" },
    { { 0x69, 0x01, 0x02 }, &PatConnection::recvAnsAgreementPageNum, "自動同意ページ数返答 ", "" },
    { { 0x69, 0x02, 0x01 }, 0, "自動同意ページ情報要求 ", "" },
    { { 0x69, 0x02, 0x02 }, &PatConnection::recvAnsAgreementPageInfo, "自動同意ページ情報返答 ", "" },
    { { 0x69, 0x03, 0x01 }, 0, "自動同意ページデータ要求 ", "" },
    { { 0x69, 0x03, 0x02 }, &PatConnection::recvAnsAgreementPage, "自動同意ページデータ返答 ", "" },
    { { 0x69, 0x10, 0x01 }, 0, "同意送信 ", "" },
    { { 0x69, 0x10, 0x02 }, &PatConnection::recvAnsAgreement, "同意返答 ", "" },
    { { 0x00, 0x00, 0x00 }, 0, NULL, NULL },
};

/* ==== the connection ========================================================================================== */

/* Clears the server records, the resolver and socket slots and the secure-server settings, then restores the
 * buffers through `resetDefaults`. */
PatConnection::PatConnection()
{
    memset(&server_0004, 0, sizeof(server_0004));
    memset(&serverInfo_008C, 0, sizeof(serverInfo_008C));
    resolver_0090 = NULL;
    memset(&socket_0094, 0, sizeof(socket_0094));
    secureHost_60C4 = NULL;
    rootCA_60C8 = NULL;
    rootCASize_60CC = 0;
    resetDefaults();
}

PatConnection::~PatConnection()
{
}

/* Empties both buffers, seeds the request id, rewinds the cursors and clears the crypt and state bytes. */
void PatConnection::resetDefaults()
{
    connected_0098 = 0;
    sendSize_009C = 0;
    memset(sendBuffer_00A0, 0, sizeof(sendBuffer_00A0));
    recvSize_20A0 = 0;
    memset(recvBuffer_20A8, 0, sizeof(recvBuffer_20A8));
    sequence_60A8 = rand();
    memset(&header_60AA, 0, sizeof(header_60AA));
    readCursor_60B4 = &recvBuffer_20A8[8];
    packetStart_60BC = sendBuffer_00A0;
    writeCursor_60C0 = &sendBuffer_00A0[8];
    cryptEnabled_60D0 = 0;
    opening_state = 0;
    connectionState_60D2 = 0;
}

s32 PatConnection::connectServer(NetworkErrorInfo* error)
{
    NetworkResolverBase* resolver;
    s32 code;

    switch (opening_state) {
    case 0:
        opening_state = 10;
        break;
    case 10:
        if (socket_0094 != NULL) {
            opening_state = 0;
            return 2;
        }
        if (serverInfo_008C == NULL) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80000003;
                error->param1_04 = 0;
                error->param2_08 = 0;
            }
            opening_state = 90;
        } else if (serverInfo_008C->address_80.ip_00[0] != 0) {
            opening_state = 30;
        } else {
            opening_state = 20;
        }
        break;
    case 20:
        if (resolver_0090 == NULL) {
            resolver_0090 = networkLibrary()->acquireResolver();
        }
        resolver_0090->setName(serverInfo_008C->host_00);
        opening_state += 5;
        break;
    case 25: {
        s32 result = resolver_0090->check();
        if (result < 0) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80020001;
                error->param1_04 = 0;
                error->param2_08 = result;
            }
            networkLibrary()->logError("PatConnection::connectServer resolver fail[%d]\n", result);
            opening_state = 90;
        } else if (result > 0) {
            resolver_0090->recordGet(0, (u32*)serverInfo_008C->address_80.ip_00);
            networkLog_destroyContext((NetworkSessionManagerLogger*)getNetworkLogger(), resolver_0090);
            resolver_0090 = NULL;
            opening_state = 30;
        }
        break;
    }
    case 30: {
        s32 result;
        if (socket_0094 == NULL) {
            socket_0094 = (NetworkSocketBase*)networkSocketPool_acquire(getNetworkLogger());
        }
        if (secureHost_60C4 != NULL && secureHost_60C4[0] != '\0') {
            result = socket_0094->openSecure(secureHost_60C4, rootCA_60C8, rootCASize_60CC);
        } else {
            result = socket_0094->open(1);
        }
        if (result < 0) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80000008;
                error->param1_04 = 0x60;
                error->param2_08 = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
            }
            code = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
            networkLibrary()->logError("PatConnection::connectServer init fail[%d]\n", code);
            opening_state = 90;
        } else if (socket_0094->connect(&serverInfo_008C->address_80) < 0) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80000008;
                error->param1_04 = 0x61;
                error->param2_08 = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
            }
            code = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
            networkLibrary()->logError("PatConnection::connectServer open fail[%d]\n", code);
            opening_state = 90;
        } else {
            opening_state += 5;
        }
        break;
    }
    case 35: {
        s32 result = socket_0094->pollConnect();
        if (result < 0) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80000008;
                error->param1_04 = 0x62;
                error->param2_08 = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
            }
            code = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
            networkLibrary()->logError("PatConnection::connectServer connect fail[%d]\n", code);
            opening_state = 90;
        } else if (result > 0) {
            connected_0098 = 1;
            opening_state = 0;
            return 1;
        }
        break;
    }
    case 90: {
        resolver = resolver_0090;
        if (resolver != NULL) {
            networkLog_destroyContext((NetworkSessionManagerLogger*)getNetworkLogger(), resolver);
            resolver_0090 = NULL;
        }
        if (socket_0094 != NULL) {
            socket_0094->close();
            networkSocketPool_release(getNetworkLogger(), (NetworkSocketHandle*)socket_0094);
            socket_0094 = NULL;
        }
        opening_state = 0;
        return -1;
    }
    }
    return 0;
}

s32 PatConnection::disconnect()
{
    switch (connectionState_60D2) {
    case 0:
        connectionState_60D2 = 10;
        break;
    case 10: {
        NetworkResolverBase* resolver = resolver_0090;
        if (resolver != NULL) {
            networkLog_destroyContext((NetworkSessionManagerLogger*)getNetworkLogger(), resolver);
            resolver_0090 = NULL;
        }
        if (socket_0094 != NULL) {
            socket_0094->close();
            networkSocketPool_release(getNetworkLogger(), (NetworkSocketHandle*)socket_0094);
            socket_0094 = NULL;
        }
        connected_0098 = 0;
        connectionState_60D2 = 0;
        opening_state = 0;
        return 1;
    }
    }
    return 0;
}

void setServerAddress(PatConnection* self, const char* host, const u8* address, const u16* port)
{
    snprintf(self->server_0004.host_00, sizeof(self->server_0004.host_00), "%s", host);
    memcpy(self->server_0004.address_80.ip_00, address, sizeof(self->server_0004.address_80.ip_00));
    self->server_0004.address_80.port_04 = *port;
    self->serverInfo_008C = &self->server_0004;
}

void getServerAddress(PatConnection* self, u8* out)
{
    memcpy(out, self->server_0004.address_80.ip_00, sizeof(self->server_0004.address_80.ip_00));
}

void setSecureServer(PatConnection* self, const char* host, const u8* rootCA, s32 rootCASize)
{
    self->secureHost_60C4 = host;
    self->rootCA_60C8 = rootCA;
    self->rootCASize_60CC = rootCASize;
}

void enableCrypt(PatConnection* self, const u8* key)
{
    PatCryptSetKey(key);
    self->cryptEnabled_60D0 = 1;
}

void disableCrypt(PatConnection* self)
{
    self->cryptEnabled_60D0 = 0;
}

s32 PatConnection::receiveCommand(NetworkErrorInfo* error)
{
    s32 code;
    s32 result;

    if (socket_0094 == NULL) {
        if (error->code_00 == 0) {
            error->code_00 = 0x80000008;
            error->param1_04 = 0;
            error->param2_08 = 0;
        }
        networkLibrary()->logError("PatConnection::receiveCommand fail no socket.\n");
        connected_0098 = 0;
        return -1;
    }
    result = socket_0094->receive(&recvBuffer_20A8[recvSize_20A0], sizeof(recvBuffer_20A8) - recvSize_20A0, NULL);
    if (result < 0) {
        if (error->code_00 == 0) {
            error->code_00 = 0x80000008;
            error->param1_04 = 0x63;
            error->param2_08 = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
        }
        code = networkSocketHandle_getLastError((NetworkSocketHandle*)socket_0094);
        networkLibrary()->logError("PatConnection::receiveCommand fail[%d]\n", code);
        connected_0098 = 0;
        return result;
    }
    recvSize_20A0 += result;
    return result;
}

s32 PatConnection::dispatchCommand(NetworkErrorInfo* error)
{
    u8* body = &recvBuffer_20A8[8];

    while ((u32)recvSize_20A0 >= 8) {
        u16 size;
        u16 length;
        u16 bodySize;
        u32 packetSize;
        s32 result;

        memcpy(&header_60AA, recvBuffer_20A8, sizeof(header_60AA));
        memcpy(&size, header_60AA.size_00, sizeof(size));
        bodySize = getNetworkLogger()->flag_48(size);
        packetSize = bodySize + 8;
        if ((u32)recvSize_20A0 < packetSize) {
            return 0;
        }
        readCursor_60B4 = body;
        if (cryptEnabled_60D0 != 0 && bodySize != 0) {
            length = bodySize;
            result = PatCryptDecrypt(body, &length);
            if (result < 0) {
                networkLibrary()->logError("PatCryptDecrypt fail[%02Xx%02X]\n", header_60AA.opcode_04[0],
                                           header_60AA.opcode_04[1]);
                if (error->code_00 == 0) {
                    error->code_00 = 0x80000000;
                    error->param1_04 = 0x66;
                    error->param2_08 = result;
                }
                return -1;
            }
            size = getNetworkLogger()->encode_4C(length);
            memcpy(header_60AA.size_00, &size, sizeof(size));
        }
        result = recvCommand(&header_60AA);
        recvSize_20A0 -= packetSize;
        if (recvSize_20A0 != 0) {
            memmove(recvBuffer_20A8, &recvBuffer_20A8[packetSize], recvSize_20A0);
        }
        if (result == -1) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80000000;
                error->param1_04 = 0x67;
                error->param2_08 = 0;
            }
            return -1;
        }
        if (result == -2) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80000004;
                error->param1_04 = 0;
                error->param2_08 = 0;
            }
            return -1;
        }
        if (checkBufError() < 0) {
            if (error->code_00 == 0) {
                error->code_00 = 0x80000000;
                error->param1_04 = 0x69;
                error->param2_08 = 0;
            }
            return -1;
        }
    }
    return 0;
}

s32 sendCommand(PatConnection* self, NetworkErrorInfo* error)
{
    s32 code;
    s32 result;

    if (self->sendSize_009C == 0) {
        return 0;
    }
    if (self->socket_0094 == NULL) {
        if (error->code_00 == 0) {
            error->code_00 = 0x80000008;
            error->param1_04 = 0;
            error->param2_08 = 0;
        }
        networkLibrary()->logError("PatConnection::sendCommand fail no socket.\n");
        self->connected_0098 = 0;
        return -1;
    }
    result = self->socket_0094->send(self->sendBuffer_00A0, self->sendSize_009C, NULL);
    if (result < 0) {
        if (error->code_00 == 0) {
            error->code_00 = 0x80000008;
            error->param1_04 = 0x64;
            error->param2_08 = networkSocketHandle_getLastError((NetworkSocketHandle*)self->socket_0094);
        }
        code = networkSocketHandle_getLastError((NetworkSocketHandle*)self->socket_0094);
        networkLibrary()->logError("PatConnection::sendCommand fail[%d]\n", code);
        self->connected_0098 = 0;
        return result;
    }
    self->sendSize_009C = 0;
    self->packetStart_60BC = self->sendBuffer_00A0;
    return 0;
}

/* ==== the packet body readers ================================================================================= */

void readUInt8(PatConnection* self, u8* out)
{
    *out = *self->readCursor_60B4;
    self->readCursor_60B4++;
}

void readUInt16(PatConnection* self, u16* out)
{
    u16 value;

    memcpy(&value, self->readCursor_60B4, sizeof(value));
    *out = getNetworkLogger()->flag_48(value);
    self->readCursor_60B4 += sizeof(value);
}

void readUInt32_(PatConnection* self, u32* out)
{
    u32 value;

    memcpy(&value, self->readCursor_60B4, sizeof(value));
    *out = networkLibrary()->hostToNet32(value);
    self->readCursor_60B4 += sizeof(value);
}

void readUInt64(PatConnection* self, u64* out)
{
    u64 value;

    memcpy(&value, self->readCursor_60B4, sizeof(value));
    *out = networkLibrary()->hostToNet64(value);
    self->readCursor_60B4 += sizeof(value);
}

void readUInt8_(PatConnection* self, s8* out)
{
    u8 value;

    readUInt8(self, &value);
    *out = (s8)value;
}

void readInt16(PatConnection* self, s16* out)
{
    s16 value;

    readUInt16(self, (u16*)&value);
    *out = value;
}

void readInt32(PatConnection* self, s32* out)
{
    s32 value;

    readUInt32_(self, (u32*)&value);
    *out = value;
}

void readString(PatConnection* self, u32* length, char* buffer, u16 size)
{
    u16 fieldSize;

    readUInt16(self, &fieldSize);
    if (size <= fieldSize) {
        if (size != 0) {
            *length = size - 1;
        } else {
            *length = 0;
        }
    } else {
        *length = fieldSize;
    }
    if ((s32)*length > 0) {
        memset(buffer, 0, size);
        memcpy(buffer, self->readCursor_60B4, *length);
    }
    self->readCursor_60B4 += fieldSize;
}

void readUInt8Array(PatConnection* self, u32* length, u8* buffer, u16 size)
{
    u16 fieldSize;

    readUInt16(self, &fieldSize);
    if (size <= fieldSize) {
        *length = size;
    } else {
        *length = fieldSize;
    }
    if ((s32)*length > 0) {
        memset(buffer, 0, size);
        memcpy(buffer, self->readCursor_60B4, *length);
    }
    self->readCursor_60B4 += fieldSize;
}

void beginReadBlock(PatConnection* self, u16* size)
{
    readUInt16(self, size);
    self->blockSize_20A4 = *size;
    self->blockStart_60B8 = self->readCursor_60B4;
}

void endReadBlock(PatConnection* self)
{
    self->readCursor_60B4 += self->blockSize_20A4 - (self->readCursor_60B4 - self->blockStart_60B8);
}

/* ==== the request writers ===================================================================================== */

u32 flushBuffer(PatConnection* self, s32 index, u8 flags)
{
    u8* packet;
    const PatPacketEntry* entry;

    if (self->connected_0098 == 0) {
        return 0;
    }
    if (0x2000 - self->sendSize_009C < 0x800) {
        NetworkErrorInfo error;
        if (sendCommand(self, &error) < 0) {
            self->postError(error);
            self->sendSize_009C = 0;
            self->packetStart_60BC = self->sendBuffer_00A0;
            return 0;
        }
    }
    packet = self->packetStart_60BC;
    memset(packet, 0, 2);
    if (patPacketTable[index].opcode_00[2] == 2) {
        memcpy(packet + 2, self->header_60AA.requestId_02, 2);
    } else {
        u16 requestId = getNetworkLogger()->encode_4C(++self->sequence_60A8);
        memcpy(packet + 2, &requestId, sizeof(requestId));
    }
    memcpy(packet + 7, &flags, 1);
    entry = &patPacketTable[index];
    memcpy(packet + 4, &entry->opcode_00[0], 1);
    memcpy(packet + 5, &entry->opcode_00[1], 1);
    memcpy(packet + 6, &entry->opcode_00[2], 1);
    self->writeCursor_60C0 = self->packetStart_60BC + 8;
    return self->sequence_60A8;
}

void encryptBuffer(PatConnection* self)
{
    u8* packet = self->packetStart_60BC;
    u16 size;
    u16 length;
    u16 wireSize;

    if (self->connected_0098 == 0) {
        NetworkErrorInfo error;
        error.code_00 = 0x80000008;
        error.param1_04 = 0;
        error.param2_08 = 0;
        self->postError(error);
        return;
    }
    size = self->writeCursor_60C0 - (packet + 8);
    length = size;
    if (self->cryptEnabled_60D0 != 0 && size != 0) {
        s32 result = PatCryptEncrypt(packet + 8, &length);
        if (result < 0) {
            u8* failed = self->packetStart_60BC;
            networkLibrary()->logError("PatCryptEncrypt fail[%02Xx%02X]\n", failed[4], failed[5]);
            NetworkErrorInfo error;
            error.code_00 = 0x80000000;
            error.param1_04 = 0x65;
            error.param2_08 = result;
            self->postError(error);
            return;
        }
    }
    self->sendSize_009C += length + 8;
    self->writeCursor_60C0 += length - size;
    wireSize = getNetworkLogger()->encode_4C(length);
    memcpy(packet, &wireSize, sizeof(wireSize));
    self->packetStart_60BC = self->writeCursor_60C0;
}

u8* writeUInt8(PatConnection* self, u8 value)
{
    u8* position;

    if (self->connected_0098 == 0) {
        return NULL;
    }
    position = self->writeCursor_60C0;
    *position = value;
    self->writeCursor_60C0++;
    return position;
}

u8* writeUInt16(PatConnection* self, u16 value)
{
    u8* position;
    u16 wire;

    if (self->connected_0098 == 0) {
        return NULL;
    }
    position = self->writeCursor_60C0;
    wire = getNetworkLogger()->encode_4C(value);
    memcpy(position, &wire, sizeof(wire));
    self->writeCursor_60C0 += sizeof(wire);
    return position;
}

u8* writeUInt32(PatConnection* self, u32 value)
{
    u8* position;
    u32 wire;

    if (self->connected_0098 == 0) {
        return NULL;
    }
    position = self->writeCursor_60C0;
    wire = networkLibrary()->netToHost32(value);
    memcpy(position, &wire, sizeof(wire));
    self->writeCursor_60C0 += sizeof(wire);
    return position;
}

u8* writeUInt64(PatConnection* self, u64 value)
{
    u8* position;
    u64 wire;

    if (self->connected_0098 == 0) {
        return NULL;
    }
    position = self->writeCursor_60C0;
    wire = networkLibrary()->netToHost64(value);
    memcpy(position, &wire, sizeof(wire));
    self->writeCursor_60C0 += sizeof(wire);
    return position;
}

u8* writeBool(PatConnection* self, s8 value)
{
    return writeUInt8(self, value);
}

u8* writeInt16(PatConnection* self, s16 value)
{
    return writeUInt16(self, value);
}

u8* writeUInt32Shared(PatConnection* self, u32 value)
{
    return writeUInt32(self, value);
}

u8* writeString(PatConnection* self, const char* text)
{
    u8* position;
    u16 length;

    if (self->connected_0098 == 0) {
        return NULL;
    }
    position = self->writeCursor_60C0;
    length = strlen(text);
    writeUInt16(self, length);
    if (length != 0) {
        memcpy(self->writeCursor_60C0, text, length);
    }
    self->writeCursor_60C0 += length;
    return position;
}

u8* writeUInt8Array(PatConnection* self, const u8* data, u16 count)
{
    u8* position;

    if (self->connected_0098 == 0) {
        return NULL;
    }
    position = self->writeCursor_60C0;
    writeUInt16(self, count);
    if (count != 0) {
        memcpy(self->writeCursor_60C0, data, count);
    }
    self->writeCursor_60C0 += count;
    return position;
}

s32 PatConnection::checkBufError()
{
    if ((u16)((header_60AA.size_00[0] << 8) | header_60AA.size_00[1]) < readCursor_60B4 - &recvBuffer_20A8[8]) {
        networkLibrary()->logError("PatConnection::checkBufError: data buffer over reading\n");
        return -1;
    }
    return 0;
}
