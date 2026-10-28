// SPDX-License-Identifier: BSD-2-Clause-Patent
// Offline pixel backend. Not installed in a profile. One complete window BEFORE
// forwarding; no restore is deferred until the original OEM service returns.
// This backend requires an independently verified frame/CPU policy and exclusive
// window ownership. A callback exception/nonlocal exit cannot guarantee restore.
#ifndef ENUMA_OEM_CAPTURE_PIXEL_H
#define ENUMA_OEM_CAPTURE_PIXEL_H
#if ENUMA_BOOT_LOG_REPLAY != 3 || (ENUMA_BOOT_LOG_HOLD_US != 8000000 && !(ENUMA_OEM_CONTEXT && ENUMA_BOOT_LOG_HOLD_US == 4000000))
#error OEM capture requires the reviewed 320x160 eight-second geometry
#endif
#include "PixelTransaction.h"
#if !defined(ENUMA_CAPTURE_PREPARED_ONLY) || !ENUMA_CAPTURE_PREPARED_ONLY
#include "EnumaOemPrecallCapture.h"
#endif
typedef struct {
#if defined(ENUMA_CAPTURE_PREPARED_ONLY) && ENUMA_CAPTURE_PREPARED_ONLY
  VOID *Capture; // not consumed by the prepared-page transaction
#else
  ENUMA_PRECALL_CAPTURE *Capture;
#endif
  ENUMA_INHERITED_FRAME Frame;
  ENUMA_U32 Policy;ENUMA_U64 El;
  ENUMA_PIXEL_READ Read;ENUMA_PIXEL_WRITE Write;ENUMA_PIXEL_REVALIDATE Validate;
  ENUMA_PIXEL_BARRIER Barrier;ENUMA_PIXEL_HOLD Hold;VOID *Io;
  // Static/caller-owned storage, never on the OEM entry stack.
  ENUMA_U32 Saved[ENUMA_PIXEL_WORDS];
  ENUMA_LOG_PAGE Page;
  ENUMA_U32 Result;
  BOOLEAN Attempted,Active;
  BOOLEAN PagePrepared;
  // Required by the sink adapter. Must terminate/recover outside OEM control
  // and never return; it is NOT a fabricated EFI_STATUS from the OEM service.
  VOID (*Stop)(VOID *);
  VOID *StopContext;
} ENUMA_CAPTURE_PIXEL;

STATIC BOOLEAN EnumaCapturePixelPage(ENUMA_CAPTURE_PIXEL *P) {
#if defined(ENUMA_CAPTURE_PREPARED_ONLY) && ENUMA_CAPTURE_PREPARED_ONLY
  return P->PagePrepared&&P->Page.Ready;
#else
  ENUMA_PRECALL_CAPTURE *C=P->Capture;CONST ENUMA_SERVICE_EVENT *E=NULL;
  UINT32 I,J,N=0,K=0;CHAR8 Guid[36];
  if(P->PagePrepared){return P->Page.Ready;}
  P->Page.Ready=FALSE;
  if(!C||C->State!=EnumaCaptureEmitting||!EnumaServiceLogValid(&C->Snapshot)){return FALSE;}
  for(I=0;I<C->Snapshot.Count;I++){
    CONST ENUMA_SERVICE_EVENT *Candidate=&C->Snapshot.Events[(C->Snapshot.Head+I)%128];
    if(Candidate->Call==C->SelectedCall){if(E||Candidate->Returned){return FALSE;}E=Candidate;}
  }
  if(!E||E->CallerRva==MAX_UINT64){return FALSE;}
  for(I=0;I<5;I++){for(J=0;J<20;J++){P->Page.Text[I][J]=' ';}P->Page.Text[I][20]=0;}
  // E + service ID + full call ID. No status is displayed for an entry.
  P->Page.Text[0][0]='E';EnumaBootLogHex(P->Page.Text[0]+1,E->Service,2);
  P->Page.Text[0][3]='C';EnumaBootLogHex(P->Page.Text[0]+4,E->Call,16);
  P->Page.Text[1][0]='Q';EnumaBootLogHex(P->Page.Text[1]+1,E->Sequence,16);
  if(C->Snapshot.Dropped){P->Page.Text[1][19]='+';}
  P->Page.Text[2][0]='O';EnumaBootLogHex(P->Page.Text[2]+1,E->CallerRva,16);
  if(E->HasGuid){
    EnumaBootViewGuid(Guid,&N,(CONST UINT8 *)&E->Guid);
    P->Page.Text[3][0]=P->Page.Text[4][0]='G';P->Page.Text[3][1]='0';P->Page.Text[4][1]='1';
    for(I=0;I<N;I++){if(Guid[I]!='-'){P->Page.Text[3+K/16][2+K%16]=Guid[I];K++;}}
  }else{
    // AllocatePool request size (hex), not a returned status.
    P->Page.Text[3][0]='C';EnumaBootLogHex(P->Page.Text[3]+1,E->Value,16);
  }
  P->Page.Sequence=E->Sequence;P->Page.Ready=TRUE;return TRUE;
#endif
}
STATIC ENUMA_U32 EnumaCapturePixelAt(ENUMA_CAPTURE_PIXEL *P,UINT32 N) {
  UINT32 X=N%320,Y=N/320,Row=Y/32,Col=X/16,Gx=(X%16)/4,Gy=(Y%32)/4;
  if(Row>=5||Col>=20||Gx>=3||Gy>=5){return P->Frame.ReservedMask;}
  return (EnumaLogGlyph(P->Page.Text[Row][Col],Gy)&(1u<<(2-Gx)))?0xffffffffu:P->Frame.ReservedMask;
}
// Result codes match the existing transaction: 1 no writes, 2 pattern failure
// restored, 3 hold failure restored, 4 restore unproven, 5 CPU restore verified.
STATIC ENUMA_U32 EnumaCapturePixelRun(ENUMA_CAPTURE_PIXEL *P) {
  UINT32 I,V;BOOLEAN Pattern=TRUE,Held,Restored=TRUE;
  if(P->Attempted){return P->Result;}P->Attempted=TRUE;P->Result=1;
  if(P->Policy!=3||P->El!=4||!EnumaPixelFrameValid(&P->Frame)||!P->Read||!P->Write||
     !P->Validate||!P->Barrier||!P->Hold||!EnumaCapturePixelPage(P)){return P->Result;}
  if(P->Barrier(P->Io)!=1){return P->Result;}
  for(I=0;I<ENUMA_PIXEL_WORDS;I++){
    if(P->Read(P->Io,EnumaPixelAddress(&P->Frame,I),&P->Saved[I])!=1){return P->Result;}
  }
  if(P->Validate(P->Io,&P->Frame)!=1){return P->Result;}
  P->Active=TRUE;P->Result=4; // conservative latch survives nonlocal exit
  for(I=0;I<ENUMA_PIXEL_WORDS;I++){
    if(P->Write(P->Io,EnumaPixelAddress(&P->Frame,I),EnumaCapturePixelAt(P,I))!=1){Pattern=FALSE;break;}
  }
  if(P->Barrier(P->Io)!=1){Pattern=FALSE;}
  if(Pattern){for(I=0;I<ENUMA_PIXEL_WORDS;I++){
    if(P->Read(P->Io,EnumaPixelAddress(&P->Frame,I),&V)!=1||V!=EnumaCapturePixelAt(P,I)){Pattern=FALSE;break;}
  }}
  Held=P->Hold(P->Io,ENUMA_BOOT_LOG_HOLD_US)==1;
  for(I=0;I<ENUMA_PIXEL_WORDS;I++){
    if(P->Write(P->Io,EnumaPixelAddress(&P->Frame,I),P->Saved[I])!=1){Restored=FALSE;}
  }
  if(P->Barrier(P->Io)!=1){Restored=FALSE;}
  for(I=0;I<ENUMA_PIXEL_WORDS;I++){
    if(P->Read(P->Io,EnumaPixelAddress(&P->Frame,I),&V)!=1||V!=P->Saved[I]){Restored=FALSE;}
  }
  P->Active=FALSE;P->Result=!Restored?4:(!Pattern?2:(!Held?3:5));return P->Result;
}
#if !defined(ENUMA_CAPTURE_PREPARED_ONLY) || !ENUMA_CAPTURE_PREPARED_ONLY
STATIC BOOLEAN EnumaCapturePixelSink(VOID *Opaque,CONST CHAR8 *Line,UINTN Size) {
  ENUMA_CAPTURE_PIXEL *P=Opaque;UINT32 I,Result;
  STATIC CONST CHAR8 End[]="OEM capture end ";
  if(!P||!P->Capture||!P->Stop||P->Capture->State!=EnumaCaptureEmitting){return FALSE;}
  if(Size<sizeof(End)-1){return FALSE;}
  for(I=0;I<sizeof(End)-1;I++){if(Line[I]!=End[I]){return TRUE;}}
  Result=EnumaCapturePixelRun(P);
  if(Result==4){
    // Record stop before calling the platform terminal path. If that callback
    // violates its contract by returning, fail closed here; never forward OEM.
    EnumaCaptureStop(P->Capture);P->Stop(P->StopContext);
    for(;;){} // No Boot Services, writes, retry or invented driver result.
  }
  return Result==5;
}
#endif
// No board sink/profile is installed. Exception/nonlocal exit needs the same
// platform stop policy; there is no general C recovery from an MMIO fault.
#endif
