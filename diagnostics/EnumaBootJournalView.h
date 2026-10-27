// SPDX-License-Identifier: BSD-2-Clause-Patent
// Attributed text view of an immutable DXE snapshot; no hardware or allocation.
// Does not change the v1 snapshot ABI or the frozen three-row pixel page.
#ifndef ENUMA_BOOT_JOURNAL_VIEW_H
#define ENUMA_BOOT_JOURNAL_VIEW_H
#include "EnumaBootJournal.h"

#define ENUMA_BOOT_VIEW_LINE_BYTES 192U

STATIC inline BOOLEAN EnumaBootLogViewValid(CONST ENUMA_BOOT_LOG *Log) {
  UINT32 I;
  if (!Log || Log->Head >= 128 || Log->Count > 128 || Log->NextSequence < Log->Count) { return FALSE; }
  if (Log->Count != (Log->NextSequence<128?(UINT32)Log->NextSequence:128U) ||
      Log->Dropped != Log->NextSequence-Log->Count) { return FALSE; }
  for (I=0; I<Log->Count; I++) {
    CONST ENUMA_BOOT_EVENT *E=&Log->Events[(Log->Head+I)%128];
    if (E->Sequence != Log->NextSequence-Log->Count+I || E->HasGuid > 1) { return FALSE; }
  }
  return TRUE;
}

STATIC inline CONST CHAR8 *EnumaBootEventPhase(UINT32 Point) {
  switch (Point) {
    case 13: return "start-return";
    case 14: return "load-return";
    case 15: return "before-start";
    case 20: return "before-entry";
    case 21: return "entry-return";
    case 22: return "after-exit";
    default: return "checkpoint";
  }
}

STATIC inline VOID EnumaBootViewText(CHAR8 *Line, UINT32 *N, CONST CHAR8 *Text) {
  while (*Text) { Line[(*N)++]=*Text++; }
}

STATIC inline VOID EnumaBootViewHex(CHAR8 *Line, UINT32 *N, UINT64 Value, UINT32 Digits) {
  EnumaBootLogHex(Line+*N,Value,Digits); *N+=Digits;
}

// EFI_GUID's first three fields are LE in memory; print canonical UUID order.
STATIC inline VOID EnumaBootViewGuid(CHAR8 *Line, UINT32 *N, CONST UINT8 *GuidLe) {
  STATIC CONST UINT8 Order[16]={3,2,1,0,5,4,7,6,8,9,10,11,12,13,14,15};
  UINT32 I;
  for (I=0; I<16; I++) {
    if (I==4 || I==6 || I==8 || I==10) { Line[(*N)++]='-'; }
    EnumaBootViewHex(Line,N,GuidLe[Order[I]],2);
  }
}

// One line per call, cursor advances only on acceptance. No automatic retry:
// an unsuccessful sink may already have emitted part/all of the line.
// Sequence/ordinal are not a boot identity or a clock. GUID names a file, not
// proof that a prior return belongs to the currently executing provider.
STATIC inline BOOLEAN EnumaBootLogReplayAttributedNext(
  CONST ENUMA_BOOT_LOG *Log, UINT32 *Cursor, ENUMA_BOOT_LOG_SINK Sink, VOID *Context) {
  CHAR8 Line[ENUMA_BOOT_VIEW_LINE_BYTES]; UINT32 N=0;
  CONST ENUMA_BOOT_EVENT *E;
  if (!EnumaBootLogViewValid(Log) || !Cursor || !Sink || *Cursor>Log->Count) { return FALSE; }
  if (*Cursor==0) {
    EnumaBootViewText(Line,&N,"DXE cached dropped=");
    EnumaBootViewHex(Line,&N,Log->Dropped,16);
    EnumaBootViewText(Line,&N," next="); EnumaBootViewHex(Line,&N,Log->NextSequence,16);
  } else {
    E=&Log->Events[(Log->Head+*Cursor-1)%128];
    EnumaBootViewText(Line,&N,"DXE seq="); EnumaBootViewHex(Line,&N,E->Sequence,16);
    EnumaBootViewText(Line,&N," p="); EnumaBootViewHex(Line,&N,E->Point,8);
    EnumaBootViewText(Line,&N," phase="); EnumaBootViewText(Line,&N,EnumaBootEventPhase(E->Point));
    EnumaBootViewText(Line,&N," o="); EnumaBootViewHex(Line,&N,E->Ordinal,8);
    EnumaBootViewText(Line,&N," s=");
    if (E->Point==15 || E->Point==20) { EnumaBootViewText(Line,&N,"not-returned"); }
    else if (E->Point==13 || E->Point==14 || E->Point==21 || E->Point==22) { EnumaBootViewHex(Line,&N,E->Status,16); }
    else { EnumaBootViewText(Line,&N,"not-a-return"); }
    EnumaBootViewText(Line,&N," guid=");
    if (E->HasGuid) { EnumaBootViewGuid(Line,&N,E->GuidLe); }
    else { EnumaBootViewText(Line,&N,"-"); }
  }
  Line[N++]='\n'; Line[N]=0;
  if (!Sink(Context,Line,N)) { return FALSE; }
  (*Cursor)++; return TRUE;
}
#endif
