// SPDX-License-Identifier: BSD-2-Clause-Patent
// Development-only private Boot Services table. Not activated by any profile.
// Caller MUST keep Context and its private tables alive for the entire driver
// lifetime, including callbacks after StartImage returns. No detach/free API.
// Single CPU DXE: TPL serializes ring updates, never wraps the forwarded call.
#ifndef ENUMA_OEM_SERVICE_TRACE_H
#define ENUMA_OEM_SERVICE_TRACE_H
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#define ENUMA_SERVICE_TRACE_CAPACITY 128U
#define ENUMA_SERVICE_TRACE_UNKNOWN_RVA MAX_UINT64
enum { EnumaServiceLocateProtocol=1,EnumaServiceHandleProtocol,EnumaServiceAllocatePool };
typedef struct {
  UINT64 Sequence,Call,CallerRva,Value,Status;
  UINT32 Service;
  BOOLEAN Returned,HasGuid;
  EFI_GUID Guid;
} ENUMA_SERVICE_EVENT;
typedef struct {
  UINT64 NextSequence,NextCall,Dropped;
  UINT32 Head,Count;
  ENUMA_SERVICE_EVENT Events[ENUMA_SERVICE_TRACE_CAPACITY];
} ENUMA_SERVICE_TRACE_LOG;
typedef struct {
  EFI_SYSTEM_TABLE System;
  EFI_BOOT_SERVICES Boot;
  EFI_BOOT_SERVICES *Original;
  UINTN ImageBase,ImageSize;
  BOOLEAN Published;
  ENUMA_SERVICE_TRACE_LOG Log;
} ENUMA_SERVICE_TRACE;
STATIC ENUMA_SERVICE_TRACE *mEnumaServiceTrace;

// Called with the original RaiseTPL(EFI_TPL_HIGH_LEVEL) held. No allocations,
// formatting, protocols, waits or device access inside this critical section.
STATIC VOID EnumaServiceRecord(ENUMA_SERVICE_TRACE *C,UINT64 Call,UINT32 Service,
  UINTN Caller,UINT64 Value,CONST EFI_GUID *Guid,BOOLEAN Returned,EFI_STATUS Status) {
  ENUMA_SERVICE_TRACE_LOG *L=&C->Log;ENUMA_SERVICE_EVENT *E;UINT32 Index;
  if(L->NextSequence==MAX_UINT64){return;}
  Index=(L->Head+L->Count)%ENUMA_SERVICE_TRACE_CAPACITY;
  if(L->Count==ENUMA_SERVICE_TRACE_CAPACITY){L->Head=(L->Head+1)%ENUMA_SERVICE_TRACE_CAPACITY;L->Dropped++;}
  else{L->Count++;}
  E=&L->Events[Index];ZeroMem(E,sizeof(*E));
  E->Sequence=L->NextSequence++;E->Call=Call;E->Service=Service;E->Returned=Returned;
  E->Status=Returned?(UINT64)Status:0;E->Value=Value;
  E->CallerRva=Caller>=C->ImageBase&&Caller-C->ImageBase<C->ImageSize?Caller-C->ImageBase:ENUMA_SERVICE_TRACE_UNKNOWN_RVA;
  E->HasGuid=Guid!=NULL;if(Guid){CopyMem(&E->Guid,Guid,sizeof(*Guid));}
}
STATIC UINT64 EnumaServiceBegin(UINT32 Service,UINTN Caller,UINT64 Value,CONST EFI_GUID *Guid) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;EFI_TPL Previous;UINT64 Call;
  Previous=C->Original->RaiseTPL(TPL_HIGH_LEVEL);Call=C->Log.NextCall;
  if(Call!=MAX_UINT64){C->Log.NextCall++;EnumaServiceRecord(C,Call,Service,Caller,Value,Guid,FALSE,0);}
  C->Original->RestoreTPL(Previous);return Call;
}
STATIC VOID EnumaServiceEnd(UINT64 Call,UINT32 Service,UINTN Caller,UINT64 Value,CONST EFI_GUID *Guid,EFI_STATUS Status) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;EFI_TPL Previous;
  Previous=C->Original->RaiseTPL(TPL_HIGH_LEVEL);
  if(Call!=MAX_UINT64){EnumaServiceRecord(C,Call,Service,Caller,Value,Guid,TRUE,Status);}
  C->Original->RestoreTPL(Previous);
}
// Noinline is required: LR must name the original PE call site, not the tracer.
STATIC __attribute__((noinline)) EFI_STATUS EFIAPI EnumaTraceLocateProtocol(EFI_GUID *Guid,VOID *Registration,VOID **Interface) {
  UINTN Caller=(UINTN)__builtin_return_address(0);UINT64 Call;EFI_STATUS Status;EFI_GUID Saved;
  if(Guid){CopyMem(&Saved,Guid,sizeof(Saved));}
  Call=EnumaServiceBegin(EnumaServiceLocateProtocol,Caller,0,Guid?&Saved:NULL);
  Status=mEnumaServiceTrace->Original->LocateProtocol(Guid,Registration,Interface);
  EnumaServiceEnd(Call,EnumaServiceLocateProtocol,Caller,0,Guid?&Saved:NULL,Status);return Status;
}
STATIC __attribute__((noinline)) EFI_STATUS EFIAPI EnumaTraceHandleProtocol(EFI_HANDLE Handle,EFI_GUID *Guid,VOID **Interface) {
  UINTN Caller=(UINTN)__builtin_return_address(0);UINT64 Call;EFI_STATUS Status;EFI_GUID Saved;
  if(Guid){CopyMem(&Saved,Guid,sizeof(Saved));}
  Call=EnumaServiceBegin(EnumaServiceHandleProtocol,Caller,0,Guid?&Saved:NULL);
  Status=mEnumaServiceTrace->Original->HandleProtocol(Handle,Guid,Interface);
  EnumaServiceEnd(Call,EnumaServiceHandleProtocol,Caller,0,Guid?&Saved:NULL,Status);return Status;
}
STATIC __attribute__((noinline)) EFI_STATUS EFIAPI EnumaTraceAllocatePool(EFI_MEMORY_TYPE Type,UINTN Size,VOID **Buffer) {
  UINTN Caller=(UINTN)__builtin_return_address(0);UINT64 Call;EFI_STATUS Status;
  Call=EnumaServiceBegin(EnumaServiceAllocatePool,Caller,Size,NULL);
  Status=mEnumaServiceTrace->Original->AllocatePool(Type,Size,Buffer);
  EnumaServiceEnd(Call,EnumaServiceAllocatePool,Caller,Size,NULL,Status);return Status;
}
STATIC EFI_STATUS EnumaServiceTracePrepare(ENUMA_SERVICE_TRACE *Context,EFI_SYSTEM_TABLE *System,UINTN ImageBase,UINTN ImageSize) {
  EFI_BOOT_SERVICES *Boot;EFI_STATUS Status;
  if(!Context||!System||!ImageSize||ImageBase>MAX_UINTN-ImageSize){return EFI_INVALID_PARAMETER;}
  if(mEnumaServiceTrace){return EFI_ALREADY_STARTED;}
  Boot=System->BootServices;
  if(System->Hdr.HeaderSize!=sizeof(*System)||!Boot||Boot->Hdr.HeaderSize!=sizeof(*Boot)||
     !Boot->RaiseTPL||!Boot->RestoreTPL||!Boot->CalculateCrc32||!Boot->LocateProtocol||!Boot->HandleProtocol||!Boot->AllocatePool){return EFI_UNSUPPORTED;}
  ZeroMem(Context,sizeof(*Context));Context->Original=Boot;Context->ImageBase=ImageBase;Context->ImageSize=ImageSize;
  CopyMem(&Context->Boot,Boot,sizeof(*Boot));CopyMem(&Context->System,System,sizeof(*System));
  Context->Boot.LocateProtocol=EnumaTraceLocateProtocol;Context->Boot.HandleProtocol=EnumaTraceHandleProtocol;
  Context->Boot.AllocatePool=EnumaTraceAllocatePool;Context->Boot.Hdr.CRC32=0;
  Status=Boot->CalculateCrc32(&Context->Boot,sizeof(Context->Boot),&Context->Boot.Hdr.CRC32);
  if(EFI_ERROR(Status)){return Status;}
  Context->System.BootServices=&Context->Boot;Context->System.Hdr.CRC32=0;
  Status=Boot->CalculateCrc32(&Context->System,sizeof(Context->System),&Context->System.Hdr.CRC32);
  if(EFI_ERROR(Status)){return Status;}
  Context->Published=TRUE;mEnumaServiceTrace=Context;return EFI_SUCCESS;
}
STATIC BOOLEAN EnumaServiceTraceSnapshot(ENUMA_SERVICE_TRACE_LOG *Output) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;EFI_TPL Previous;
  if(!C||!Output||Output==&C->Log){return FALSE;}
  Previous=C->Original->RaiseTPL(TPL_HIGH_LEVEL);CopyMem(Output,&C->Log,sizeof(*Output));C->Original->RestoreTPL(Previous);return TRUE;
}
#endif
