// SPDX-License-Identifier: BSD-2-Clause-Patent
// Five bounded, independent observations. No status is fabricated for entry.
#ifndef ENUMA_OEM_PROGRESS_H
#define ENUMA_OEM_PROGRESS_H
#include "EnumaOemServiceTraceView.h"
#ifndef ENUMA_OEM_EARLY
#define ENUMA_OEM_EARLY 0
#endif
#ifndef ENUMA_OEM_POST_KERNEL
#define ENUMA_OEM_POST_KERNEL 0
#endif
#define ENUMA_PROGRESS_CALL (ENUMA_OEM_REGISTRATION ? 5ULL : ENUMA_OEM_POST_KERNEL ? 4ULL : (ENUMA_OEM_EARLY ? 1ULL : 0ULL))
typedef struct {
  ENUMA_SERVICE_TRACE *Owner;
  UINT64 Session,SidCall;
  UINT32 Seen;
  BOOLEAN Active,Stopped;
  BOOLEAN (*Present)(UINT32,CONST ENUMA_SERVICE_EVENT *,UINT64);
  VOID (*Stop)(VOID *);
  ENUMA_SERVICE_TRACE_LOG Snapshot;
} ENUMA_OEM_PROGRESS_CONTEXT;
STATIC ENUMA_OEM_PROGRESS_CONTEXT mOemProgress;
STATIC __attribute__((noinline)) VOID OemProgressFail(VOID){
  mOemProgress.Stopped=TRUE;mOemProgress.Stop(NULL);
  for(;;){} // A returning terminal callback cannot resume OEM.
}
STATIC __attribute__((noinline)) VOID OemProgressEmit(UINT32 Phase,CONST ENUMA_SERVICE_EVENT *E,EFI_TPL Tpl){
  ENUMA_OEM_PROGRESS_CONTEXT *P=&mOemProgress;UINT32 Bit=1U<<Phase;
  if(P->Stopped||P->Active||Phase<1||Phase>5||(P->Seen&Bit)||Tpl!=TPL_APPLICATION){OemProgressFail();}
  P->Seen|=Bit;P->Active=TRUE;
  if(!P->Present(Phase,E,P->Session)){OemProgressFail();}
  P->Active=FALSE;
}
STATIC __attribute__((noinline)) VOID OemProgressObserve(ENUMA_SERVICE_TRACE *Trace,UINT64 Call,EFI_TPL Tpl,BOOLEAN Returned){
#if ENUMA_OEM_EARLY
  STATIC CONST EFI_GUID Sid={0xb5062be7,0x170b,0x4a32,{0xbe,0x21,0x68,0x92,0x62,0xff,0x43,0x99}};
#else
  STATIC CONST EFI_GUID Sid={0x54b6d3b4,0x5d33,0x4f91,{0x86,0x00,0x6c,0x41,0xd5,0xde,0xb1,0x9a}};
#endif
  ENUMA_OEM_PROGRESS_CONTEXT *P=&mOemProgress;CONST ENUMA_SERVICE_EVENT *E;BOOLEAN IsSid;
  if(Trace!=P->Owner||P->Stopped){return;}
  if(P->Active){OemProgressFail();}
  if(Call==MAX_UINT64||!EnumaServiceTraceSnapshot(&P->Snapshot)||!EnumaServiceLogValid(&P->Snapshot)||!P->Snapshot.Count){OemProgressFail();}
  E=&P->Snapshot.Events[(P->Snapshot.Head+P->Snapshot.Count-1)%128];
  if(E->Call!=Call||E->Returned!=Returned){OemProgressFail();}
  IsSid=E->Service==EnumaServiceLocateProtocol&&E->HasGuid&&CompareMem(&E->Guid,&Sid,sizeof(Sid))==0;
  if(!Returned&&IsSid&&P->SidCall==MAX_UINT64){P->SidCall=Call;}
  if(Call==ENUMA_PROGRESS_CALL){
    if(ENUMA_OEM_REGISTRATION&&(E->Service!=EnumaServiceInstallConfigurationTable||E->CallerRva!=0x5cc8)){OemProgressFail();}
    if(E->CallerRva==MAX_UINT64||(Returned&&!(P->Seen&(1U<<1)))){OemProgressFail();}
    OemProgressEmit(Returned?2:1,E,Tpl);
  }
  if(IsSid&&Call==P->SidCall){
    if(E->CallerRva==MAX_UINT64||(Returned&&!(P->Seen&(1U<<3)))){OemProgressFail();}
    OemProgressEmit(Returned?4:3,E,Tpl);
  }
}
STATIC __attribute__((noinline)) VOID OemProgressBefore(ENUMA_SERVICE_TRACE *T,UINT64 C,EFI_TPL L){OemProgressObserve(T,C,L,FALSE);}
STATIC __attribute__((noinline)) VOID OemProgressAfter(ENUMA_SERVICE_TRACE *T,UINT64 C,EFI_TPL L){OemProgressObserve(T,C,L,TRUE);}
STATIC __attribute__((noinline)) VOID OemProgressReturned(EFI_STATUS Status){
  ENUMA_SERVICE_EVENT E;EFI_TPL Tpl;ENUMA_OEM_PROGRESS_CONTEXT *P=&mOemProgress;
  if(!P->Owner||P->Stopped){return;}
  ZeroMem(&E,sizeof(E));E.Returned=TRUE;E.Status=Status;E.Call=MAX_UINT64;E.CallerRva=MAX_UINT64;
  E.Sequence=P->Owner->Log.NextSequence;
  Tpl=P->Owner->Original->RaiseTPL(TPL_HIGH_LEVEL);P->Owner->Original->RestoreTPL(Tpl);
  OemProgressEmit(5,&E,Tpl);
}
STATIC EFI_STATUS OemProgressArm(ENUMA_SERVICE_TRACE *T,UINT64 Session,
  BOOLEAN (*Present)(UINT32,CONST ENUMA_SERVICE_EVENT *,UINT64),VOID (*Stop)(VOID *)){
  ENUMA_OEM_PROGRESS_CONTEXT *P=&mOemProgress;
  if(!T||!Session||!Present||!Stop){return EFI_INVALID_PARAMETER;}
  if(P->Owner||T->BeforeForward||T->AfterForward){return EFI_ALREADY_STARTED;}
  if(T!=mEnumaServiceTrace||!T->Published||T->Log.NextCall||T->Log.NextSequence){return EFI_NOT_READY;}
  ZeroMem(P,sizeof(*P));P->Owner=T;P->Session=Session;P->SidCall=MAX_UINT64;P->Present=Present;P->Stop=Stop;
  T->BeforeForward=OemProgressBefore;T->AfterForward=OemProgressAfter;return EFI_SUCCESS;
}
#endif
