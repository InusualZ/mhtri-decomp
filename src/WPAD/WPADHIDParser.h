/* WPAD/WPADHIDParser.h - the report parser `WPAD/WPADHIDParser.cpp` owns: the extension and calibration handshakes, the report dispatch and the
 *   report decoders (docs/plan.md 6.5 rule 2). */
#ifndef WPAD_WPADHIDPARSER_H
#define WPAD_WPADHIDPARSER_H

#include "types.h"
#include "WPAD/wpad.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804F2FF0..0x804F72B0, in address order. */
void wpadExtInitCallback(s32 chan, s32 result);
void wpadAccCalibrationReadDone(s32 chan, s32 result);
void wpadExtCalibrationReadDone(s32 chan, s32 result);
void wpadExtIdReadDone(s32 chan, s32 result);
void wpadDpdCalibrationReadDone(s32 chan, s32 error, s32 which);
s32 wpadDispatchReport(s32 chan, u8* report);
void wpadReportStatus(u8 chan, u8* report, WPADSample* sample);
void wpadReportReadData(u8 chan, u8* report, WPADSample* sample);
void wpadReportAck(u8 chan, u8* report, WPADSample* sample);
void wpadDecodeDpd(s32 chan, WPADSample** sample, u32 format, u8* data, s32 length);
void wpadDecodeDpdInterleaved(s32 chan, WPADSample** sample, u8 slot, u8* data, s32 reserved);
void wpadDecodeExtDualStick(s32 chan, WPADSample** sample, u8 variant, u8* data, u32 length);
void wpadDecodeExtBoard(s32 chan, WPADSample** sample, u8 format, u8* data, u32 length);
void wpadReportButtons(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsAccel(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsExt8(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsAccelDpd12(u8 chan, u8* report, WPADSample* sample);
void wpadReportExt19(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsAccelExt16(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsDpd10Ext9(u8 chan, u8* report, WPADSample* sample);
void wpadReportButtonsAccelDpd10Ext6(u8 chan, u8* report, WPADSample* sample);
void wpadReportExt21(u8 chan, u8* report, WPADSample* sample);
void wpadReportInterleavedA(u8 chan, u8* report, WPADSample* sample);
void wpadReportInterleavedB(u8 chan, u8* report, WPADSample* sample);
void wpadReportIgnored(u8 chan, u8* report, WPADSample* sample);

#ifdef __cplusplus
}
#endif

#endif /* WPAD_WPADHIDPARSER_H */
