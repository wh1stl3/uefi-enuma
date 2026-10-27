// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_OEM_PROGRESS_PAGE_H
#define ENUMA_OEM_PROGRESS_PAGE_H
STATIC BOOLEAN OemProgressPage(ENUMA_LOG_PAGE *Page,UINT32 Phase,CONST ENUMA_SERVICE_EVENT *E,UINT64 Session){
  UINT32 I,J;BOOLEAN Returned=Phase==2||Phase==4||Phase==5;
  if(!Page||!E||!Session||Phase<1||Phase>5||E->Returned!=Returned||
     (Phase!=5&&(E->CallerRva==MAX_UINT64||E->Call==MAX_UINT64))){return FALSE;}
  ZeroMem(Page,sizeof(*Page));
  for(I=0;I<5;I++){for(J=0;J<20;J++){Page->Text[I][J]=' ';}Page->Text[I][20]=0;}
  // Use only the existing reviewed glyph set. E/S prefix denotes entry/return;
  // G is the session token (not a GUID). Full SID GUID is fixed by the selector.
  Page->Text[0][0]=Returned?'S':'E';EnumaBootLogHex(Page->Text[0]+1,Phase,2);
#if ENUMA_OEM_FAR_CHAIN
  if(Phase==5){Page->Text[0][3]='C';EnumaBootLogHex(Page->Text[0]+4,E->Value,16);}
#endif
  if(Phase!=5){Page->Text[0][3]='C';EnumaBootLogHex(Page->Text[0]+4,E->Call,16);}
  Page->Text[1][0]='Q';EnumaBootLogHex(Page->Text[1]+1,E->Sequence,16);
  if(Phase!=5){Page->Text[2][0]='O';EnumaBootLogHex(Page->Text[2]+1,E->CallerRva,16);}
#if ENUMA_OEM_FAR_CHAIN
  if(Phase==5){Page->Text[2][0]='O';EnumaBootLogHex(Page->Text[2]+1,E->CallerRva,16);}
#endif
  Page->Text[3][0]=Returned?'S':'E';
  if(Returned){EnumaBootLogHex(Page->Text[3]+1,E->Status,16);}
  Page->Text[4][0]='G';EnumaBootLogHex(Page->Text[4]+1,Session,16);
  Page->Sequence=E->Sequence;Page->Ready=TRUE;return TRUE;
}
#endif
