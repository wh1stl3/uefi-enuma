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
#ifndef ENUMA_OEM_REGISTRATION
#define ENUMA_OEM_REGISTRATION 0
#endif
#ifndef ENUMA_OEM_FAR_CHAIN
#define ENUMA_OEM_FAR_CHAIN 0
#endif
#ifndef ENUMA_OEM_RAM_EVENT
#define ENUMA_OEM_RAM_EVENT 0
#endif
#if ENUMA_OEM_RAM_EVENT && !ENUMA_OEM_FAR_CHAIN
#error RAM event capture requires the far-chain scope
#endif
#ifndef ENUMA_OEM_MAIN_ENTRY
#define ENUMA_OEM_MAIN_ENTRY 0
#endif
enum { EnumaServiceLocateProtocol=1,EnumaServiceHandleProtocol,EnumaServiceAllocatePool,EnumaServiceInstallConfigurationTable };
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
typedef struct ENUMA_SERVICE_TRACE_CONTEXT {
  EFI_SYSTEM_TABLE System;
  EFI_BOOT_SERVICES Boot;
  EFI_BOOT_SERVICES *Original;
  EFI_SYSTEM_TABLE *SourceSystem;
  UINTN ImageBase,ImageSize;
  BOOLEAN Published;
  ENUMA_SERVICE_TRACE_LOG Log;
  // Optional development observer. Runs after restoring the caller's TPL and
  // BEFORE forwarding. NULL preserves the original no-output path. It must not
  // depend on the forwarded service returning. Context and observer are static.
  VOID (*BeforeForward)(struct ENUMA_SERVICE_TRACE_CONTEXT *,UINT64,EFI_TPL);
  VOID (*AfterForward)(struct ENUMA_SERVICE_TRACE_CONTEXT *,UINT64,EFI_TPL);
#if ENUMA_OEM_RAM_EVENT
  // Called under original TPL_HIGH_LEVEL; bounded RAM stores only, never I/O.
  VOID (*Recorded)(struct ENUMA_SERVICE_TRACE_CONTEXT *,CONST ENUMA_SERVICE_EVENT *);
#endif
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
#if ENUMA_OEM_RAM_EVENT
  if(C->Recorded){C->Recorded(C,E);}
#endif
}
STATIC UINT64 EnumaServiceBegin(UINT32 Service,UINTN Caller,UINT64 Value,CONST EFI_GUID *Guid) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;EFI_TPL Previous;UINT64 Call;
  Previous=C->Original->RaiseTPL(TPL_HIGH_LEVEL);Call=C->Log.NextCall;
  if(Call!=MAX_UINT64){C->Log.NextCall++;EnumaServiceRecord(C,Call,Service,Caller,Value,Guid,FALSE,0);}
  C->Original->RestoreTPL(Previous);
  if(C->BeforeForward){C->BeforeForward(C,Call,Previous);}
  return Call;
}
STATIC VOID EnumaServiceEnd(UINT64 Call,UINT32 Service,UINTN Caller,UINT64 Value,CONST EFI_GUID *Guid,EFI_STATUS Status) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;EFI_TPL Previous;
  Previous=C->Original->RaiseTPL(TPL_HIGH_LEVEL);
  if(Call!=MAX_UINT64){EnumaServiceRecord(C,Call,Service,Caller,Value,Guid,TRUE,Status);}
  C->Original->RestoreTPL(Previous);
  if(C->AfterForward){C->AfterForward(C,Call,Previous);}
}
// Noinline is required: LR must name the original PE call site, not the tracer.
STATIC EFI_STATUS EnumaTraceLocateProtocolAt(UINTN Caller,EFI_GUID *Guid,VOID *Registration,VOID **Interface) {
  UINT64 Call;EFI_STATUS Status;EFI_GUID Saved;
  if(Guid){CopyMem(&Saved,Guid,sizeof(Saved));}
  Call=EnumaServiceBegin(EnumaServiceLocateProtocol,Caller,0,Guid?&Saved:NULL);
  Status=mEnumaServiceTrace->Original->LocateProtocol(Guid,Registration,Interface);
  EnumaServiceEnd(Call,EnumaServiceLocateProtocol,Caller,0,Guid?&Saved:NULL,Status);return Status;
}
STATIC __attribute__((noinline)) EFI_STATUS EFIAPI EnumaTraceLocateProtocol(EFI_GUID *Guid,VOID *Registration,VOID **Interface) {
  return EnumaTraceLocateProtocolAt((UINTN)__builtin_return_address(0),Guid,Registration,Interface);
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
#if ENUMA_OEM_REGISTRATION
// Exact original OEM call at 0x5cc4, LR 0x5cc8. Other callers are transparent
// and unrecorded so existing call ordinals retain their meaning.
STATIC EFI_STATUS EnumaTraceInstallConfigurationAt(UINTN Caller,EFI_GUID *Guid,VOID *Table) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;
  UINT64 Call;EFI_STATUS Status;EFI_GUID Saved;
  if(Caller<C->ImageBase||Caller-C->ImageBase!=0x5cc8||C->ImageSize<=0x5cc8){
    return C->Original->InstallConfigurationTable(Guid,Table);
  }
  if(Guid){CopyMem(&Saved,Guid,sizeof(Saved));}
  Call=EnumaServiceBegin(EnumaServiceInstallConfigurationTable,Caller,0,Guid?&Saved:NULL);
  Status=C->Original->InstallConfigurationTable(Guid,Table);
  EnumaServiceEnd(Call,EnumaServiceInstallConfigurationTable,Caller,0,Guid?&Saved:NULL,Status);return Status;
}
STATIC __attribute__((noinline)) EFI_STATUS EFIAPI EnumaTraceInstallConfigurationTable(EFI_GUID *Guid,VOID *Table) {
  return EnumaTraceInstallConfigurationAt((UINTN)__builtin_return_address(0),Guid,Table);
}
#endif
STATIC EFI_STATUS EnumaServiceTracePrepare(ENUMA_SERVICE_TRACE *Context,EFI_SYSTEM_TABLE *System,UINTN ImageBase,UINTN ImageSize) {
  EFI_BOOT_SERVICES *Boot;EFI_STATUS Status;
  if(!Context||!System||!ImageSize||ImageBase>MAX_UINTN-ImageSize){return EFI_INVALID_PARAMETER;}
  if(mEnumaServiceTrace){return EFI_ALREADY_STARTED;}
  Boot=System->BootServices;
  if(System->Hdr.HeaderSize!=sizeof(*System)||!Boot||Boot->Hdr.HeaderSize!=sizeof(*Boot)||
     !Boot->RaiseTPL||!Boot->RestoreTPL||!Boot->CalculateCrc32||!Boot->LocateProtocol||!Boot->HandleProtocol||!Boot->AllocatePool){return EFI_UNSUPPORTED;}
#if ENUMA_OEM_REGISTRATION
  if(!Boot->InstallConfigurationTable){return EFI_UNSUPPORTED;}
#endif
  ZeroMem(Context,sizeof(*Context));Context->Original=Boot;Context->SourceSystem=System;Context->ImageBase=ImageBase;Context->ImageSize=ImageSize;
  CopyMem(&Context->Boot,Boot,sizeof(*Boot));CopyMem(&Context->System,System,sizeof(*System));
  Context->Boot.LocateProtocol=EnumaTraceLocateProtocol;Context->Boot.HandleProtocol=EnumaTraceHandleProtocol;
  Context->Boot.AllocatePool=EnumaTraceAllocatePool;Context->Boot.Hdr.CRC32=0;
#if ENUMA_OEM_REGISTRATION
  Context->Boot.InstallConfigurationTable=EnumaTraceInstallConfigurationTable;
#endif
  Status=Boot->CalculateCrc32(&Context->Boot,sizeof(Context->Boot),&Context->Boot.Hdr.CRC32);
  if(EFI_ERROR(Status)){return Status;}
  Context->System.BootServices=&Context->Boot;Context->System.Hdr.CRC32=0;
  Status=Boot->CalculateCrc32(&Context->System,sizeof(Context->System),&Context->System.Hdr.CRC32);
  if(EFI_ERROR(Status)){return Status;}
  Context->Published=TRUE;mEnumaServiceTrace=Context;return EFI_SUCCESS;
}
// Core invokes this before old configuration storage is freed and before
// notifications. It performs no allocations or protocol calls. Inactive traces
// are harmless; unrelated SystemTables cannot update this private view.
STATIC EFI_STATUS EnumaServiceTraceSyncSystem(EFI_SYSTEM_TABLE *System) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;
  if(!C||!C->Published){return EFI_SUCCESS;}
  if(System!=C->SourceSystem){return EFI_INVALID_PARAMETER;}
  C->System.ConfigurationTable=System->ConfigurationTable;
  C->System.NumberOfTableEntries=System->NumberOfTableEntries;
  C->System.Hdr.CRC32=0;
  return C->Original->CalculateCrc32(&C->System,sizeof(C->System),&C->System.Hdr.CRC32);
}
#if !ENUMA_OEM_EXIT_CLOCK
STATIC BOOLEAN EnumaServiceTraceSnapshot(ENUMA_SERVICE_TRACE_LOG *Output) {
  ENUMA_SERVICE_TRACE *C=mEnumaServiceTrace;EFI_TPL Previous;
  if(!C||!Output||Output==&C->Log){return FALSE;}
  Previous=C->Original->RaiseTPL(TPL_HIGH_LEVEL);CopyMem(Output,&C->Log,sizeof(*Output));C->Original->RestoreTPL(Previous);return TRUE;
}
#endif
#endif
