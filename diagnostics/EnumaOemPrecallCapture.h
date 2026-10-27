// SPDX-License-Identifier: BSD-2-Clause-Patent
// Development-only, one selected pre-forward snapshot. No board sink/profile.
// The sink must be independently observable, bounded and non-reentrant; TRUE
// means accepted bytes, NOT durable storage, camera capture or visible pixels.
#ifndef ENUMA_OEM_PRECALL_CAPTURE_H
#define ENUMA_OEM_PRECALL_CAPTURE_H
#include "EnumaOemServiceTraceView.h"
enum {
  EnumaCaptureUnarmed,EnumaCaptureArmed,EnumaCaptureEmitting,
  EnumaCaptureExported,EnumaCaptureSinkFailed,EnumaCaptureReentered,
  EnumaCaptureBadTpl,EnumaCaptureLost,EnumaCaptureExhausted,
  EnumaCaptureStopped,EnumaCaptureInterrupted
};
typedef struct {
  UINT32 State,LinesAccepted,Attempts;
  UINT64 Session,SelectedCall,StreamHash;
  ENUMA_SERVICE_TRACE *Owner;
  ENUMA_BOOT_LOG_SINK Sink;
  VOID *SinkContext;
  // Persistent storage: no 10KB snapshot on the OEM call stack.
  ENUMA_SERVICE_TRACE_LOG Snapshot;
} ENUMA_PRECALL_CAPTURE;
STATIC ENUMA_PRECALL_CAPTURE *mEnumaPrecallCapture;

STATIC BOOLEAN EnumaCaptureWrite(VOID *Opaque,CONST CHAR8 *Line,UINTN Size) {
  ENUMA_PRECALL_CAPTURE *C=Opaque;UINTN I;
  if(C->State!=EnumaCaptureEmitting){return FALSE;}
  if(!C->Sink(C->SinkContext,Line,Size)){
    if(C->State==EnumaCaptureEmitting){C->State=EnumaCaptureSinkFailed;}
    return FALSE;
  }
  // A callback may reenter or explicitly stop capture before returning TRUE.
  if(C->State!=EnumaCaptureEmitting){return FALSE;}
  for(I=0;I<Size;I++){C->StreamHash^=(UINT8)Line[I];C->StreamHash*=1099511628211ULL;}
  C->LinesAccepted++;return TRUE;
}
STATIC BOOLEAN EnumaCaptureMarker(ENUMA_PRECALL_CAPTURE *C,BOOLEAN End) {
  CHAR8 Line[128];UINT32 N=0;
  EnumaBootViewText(Line,&N,End?"OEM capture end session=":"OEM capture begin session=");
  EnumaBootViewHex(Line,&N,C->Session,16);
  EnumaBootViewText(Line,&N," selected=");EnumaBootViewHex(Line,&N,C->SelectedCall,16);
  if(End){EnumaBootViewText(Line,&N," hash=");EnumaBootViewHex(Line,&N,C->StreamHash,16);}
  Line[N++]='\n';Line[N]=0;
  return EnumaCaptureWrite(C,Line,N);
}
STATIC VOID EnumaCaptureBeforeForward(ENUMA_SERVICE_TRACE *Trace,UINT64 Call,EFI_TPL Tpl) {
  ENUMA_PRECALL_CAPTURE *C=mEnumaPrecallCapture;UINT32 I,Cursor=0;BOOLEAN Found=FALSE;
  if(!C||C->Owner!=Trace){return;}
  if(C->State==EnumaCaptureEmitting){C->State=EnumaCaptureReentered;return;}
  if(C->State!=EnumaCaptureArmed){return;}
  if(Call==MAX_UINT64||Trace->Log.NextSequence==MAX_UINT64){C->State=EnumaCaptureExhausted;return;}
  if(Call<C->SelectedCall){return;}
  if(Call!=C->SelectedCall){C->State=EnumaCaptureLost;return;}
  // No formatting/output at raised TPL. Never defer and silently relabel a
  // later call as the selected entry.
  if(Tpl!=TPL_APPLICATION){C->State=EnumaCaptureBadTpl;return;}
  C->State=EnumaCaptureEmitting;C->Attempts++;C->StreamHash=14695981039346656037ULL;
  if(!EnumaServiceTraceSnapshot(&C->Snapshot)||!EnumaServiceLogValid(&C->Snapshot)){
    C->State=EnumaCaptureLost;return;
  }
  for(I=0;I<C->Snapshot.Count;I++){
    CONST ENUMA_SERVICE_EVENT *E=&C->Snapshot.Events[(C->Snapshot.Head+I)%128];
    if(E->Call==Call){
      if(E->Returned||Found){C->State=EnumaCaptureLost;return;}
      Found=TRUE;
    }
  }
  if(!Found){C->State=EnumaCaptureLost;return;}
  if(!EnumaCaptureMarker(C,FALSE)){return;}
  while(Cursor<=C->Snapshot.Count){
    if(!EnumaServiceReplayNext(&C->Snapshot,&Cursor,EnumaCaptureWrite,C)){
      if(C->State==EnumaCaptureEmitting){C->State=EnumaCaptureLost;}
      return;
    }
  }
  if(EnumaCaptureMarker(C,TRUE)){C->State=EnumaCaptureExported;}
}
STATIC __attribute__((unused)) EFI_STATUS EnumaCaptureArm(ENUMA_PRECALL_CAPTURE *C,ENUMA_SERVICE_TRACE *Trace,
  UINT64 Session,UINT64 SelectedCall,ENUMA_BOOT_LOG_SINK Sink,VOID *Opaque) {
  if(!C||!Trace||!Sink||!Session||SelectedCall==MAX_UINT64){return EFI_INVALID_PARAMETER;}
  // Arm once, before any target calls, on the single DXE CPU at application
  // TPL. No reset/rearm API: stale records cannot silently become a new run.
  if(mEnumaPrecallCapture||Trace->BeforeForward){return EFI_ALREADY_STARTED;}
  if(Trace!=mEnumaServiceTrace||!Trace->Published||Trace->Log.NextCall||Trace->Log.NextSequence){return EFI_NOT_READY;}
  ZeroMem(C,sizeof(*C));C->State=EnumaCaptureArmed;C->Owner=Trace;
  C->Session=Session;C->SelectedCall=SelectedCall;C->Sink=Sink;C->SinkContext=Opaque;
  mEnumaPrecallCapture=C;Trace->BeforeForward=EnumaCaptureBeforeForward;return EFI_SUCCESS;
}
STATIC VOID EnumaCaptureStop(ENUMA_PRECALL_CAPTURE *C) {
  if(!C||C!=mEnumaPrecallCapture){return;}
  if(C->State==EnumaCaptureEmitting){C->State=EnumaCaptureInterrupted;}
  else if(C->State==EnumaCaptureArmed){C->State=EnumaCaptureStopped;}
}
#endif
