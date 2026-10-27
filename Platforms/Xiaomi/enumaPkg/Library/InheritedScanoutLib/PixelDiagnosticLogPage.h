// SPDX-License-Identifier: BSD-2-Clause-Patent
// Opt-in replay=3: one immutable completed return, never current-provider status.
#ifndef ENUMA_PIXEL_DIAGNOSTIC_LOG_PAGE_H
#define ENUMA_PIXEL_DIAGNOSTIC_LOG_PAGE_H
#include "EnumaBootJournalView.h"
typedef struct { CHAR8 Text[5][21]; UINT64 Sequence; BOOLEAN Ready; } ENUMA_LOG_PAGE;
STATIC ENUMA_LOG_PAGE mEnumaLogPage;
STATIC inline int EnumaLogPageFromSnapshot(CONST ENUMA_BOOT_LOG *Log,ENUMA_LOG_PAGE *Page) {
  UINT32 I,J,N=0,K=0;CHAR8 Guid[36];CONST ENUMA_BOOT_EVENT *Event=NULL;
  if(!Page) { return 0; } Page->Ready=FALSE;
  if(!EnumaBootLogViewValid(Log)) { return 0; }
  for(I=Log->Count;I>0;I--) {
    CONST ENUMA_BOOT_EVENT *E=&Log->Events[(Log->Head+I-1)%128];
    if(E->Point==13) {
      UINT32 Rank=(E->Status>>63)?2U:(E->Status?1U:0U);
      UINT32 Previous=Event?((Event->Status>>63)?2U:(Event->Status?1U:0U)):0U;
      // Reverse walk: newest at highest severity; equal severity keeps newest.
      if(!Event||Rank>Previous) { Event=E; }
    }
  }
  // Never hide a selected failure without GUID by substituting another event.
  if(!Event||!Event->HasGuid) { return 0; }
  for(I=0;I<5;I++) { for(J=0;J<20;J++) { Page->Text[I][J]=' '; } Page->Text[I][20]=0; }
  Page->Text[0][0]=(Event->Status>>63)?'E':(Event->Status?'W':'C');EnumaBootLogHex(Page->Text[0]+1,Event->Point,2);
  Page->Text[0][4]='O';EnumaBootLogHex(Page->Text[0]+5,Event->Ordinal,8);
  if(Log->Dropped) { Page->Text[0][13]='+'; }
  Page->Text[1][0]='S';EnumaBootLogHex(Page->Text[1]+1,Event->Status,16);
  Page->Text[2][0]='Q';EnumaBootLogHex(Page->Text[2]+1,Event->Sequence,16);
  // Canonical UUID order, hyphens omitted; G0/G1 label the first/last 16 digits.
  EnumaBootViewGuid(Guid,&N,Event->GuidLe);
  Page->Text[3][0]='G';Page->Text[3][1]='0';
  Page->Text[4][0]='G';Page->Text[4][1]='1';
  for(I=0;I<N;I++) { if(Guid[I]!='-') { Page->Text[3+K/16][2+K%16]=Guid[I];K++; } }
  Page->Sequence=Event->Sequence;Page->Ready=TRUE;return 1;
}
STATIC inline ENUMA_U32 EnumaLogGlyph(CHAR8 Ch,ENUMA_U32 Row) {
  STATIC CONST UINT8 Glyph[25][5]={
    {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},
    {5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},
    {7,5,7,5,7},{7,5,7,1,7},{2,5,7,5,5},{6,5,6,5,6},
    {7,4,4,4,7},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},
    {2,5,5,5,2},{3,4,2,1,6},{0,2,7,2,0},{0,0,0,0,0},
    {2,5,5,7,3},{7,4,5,5,7},
    {3,4,6,4,3},{5,5,5,7,5},{3,4,4,4,3}};
  UINT32 Index=19;
  if(Ch>='0'&&Ch<='9') { Index=(UINT32)(Ch-'0'); }
  else if(Ch>='a'&&Ch<='f') { Index=(UINT32)(Ch-'a'+10); }
  else if(Ch=='C') { Index=24; } else if(Ch=='O') { Index=16; }
  else if(Ch=='S') { Index=17; } else if(Ch=='+') { Index=18; }
  else if(Ch=='E') { Index=22; } else if(Ch=='W') { Index=23; }
  else if(Ch=='Q') { Index=20; } else if(Ch=='G') { Index=21; }
  return Row<5?Glyph[Index][Row]:0;
}
STATIC inline ENUMA_U32 EnumaPixelLogAt(const ENUMA_INHERITED_FRAME *Frame,ENUMA_U32 N) {
  UINT32 X=N%320,Y=N/320,Row=Y/32,Column=X/16,Gx=(X%16)/4,Gy=(Y%32)/4;
  if(!mEnumaLogPage.Ready||Row>=5||Column>=20||Gx>=3||Gy>=5) { return Frame->ReservedMask; }
  return (EnumaLogGlyph(mEnumaLogPage.Text[Row][Column],Gy)&(1u<<(2-Gx)))?0xffffffffu:Frame->ReservedMask;
}
#endif
