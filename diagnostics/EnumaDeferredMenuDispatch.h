// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_DEFERRED_MENU_DISPATCH_H
#define ENUMA_DEFERRED_MENU_DISPATCH_H
#if ENUMA_DEFERRED_MENU
#if !ENUMA_FULL_MENU || !ENUMA_BOUNDED_MENU || !ENUMA_BOOT_JOURNAL || !ENUMA_VIDEO_TERMINAL || ENUMA_DIAG_STAGE != 13 || ENUMA_DIAG_DETAIL_POINT != 16 || ENUMA_DIAG_DISPATCH_FALLBACK
#error Deferred menu requires the exact bounded post-return profile
#endif
#include "EnumaDeferredMenu.h"
STATIC BOOLEAN EnumaDeferredAttempted;
STATIC ENUMA_BOOT_LOG EnumaDeferredSnapshot;
STATIC VOID EnumaDeferredMenuDispatch(EFI_HANDLE Handle,CONST EFI_GUID *File,EFI_STATUS Status) {
  EFI_GUID Guid=ENUMA_DEFERRED_MENU_GUID;ENUMA_DEFERRED_MENU_PROTOCOL *Service=NULL;UINT32 Result=1;
  if(!File||ReadUnaligned64((CONST UINT64 *)File)!=0x4ffc788ddcfd1e6dULL||
     ReadUnaligned64((CONST UINT64 *)((CONST UINT8 *)File+8))!=0x921a65752fca1b8eULL){return;}
  // The original point-13 record already contains the real driver status.
  // A missing service, failure, or second call must not dispatch later drivers.
  if(!EnumaDeferredAttempted){
    EnumaDeferredAttempted=TRUE;
    if(Status==EFI_SUCCESS&&Handle&&
       CoreHandleProtocol(Handle,&Guid,(VOID **)&Service)==EFI_SUCCESS&&Service&&
       Service->Version==1&&Service->Bytes==sizeof(*Service)&&Service->Owner==Handle&&Service->Run&&
       EnumaDxeBootLogSnapshot(&EnumaDeferredSnapshot)&&EnumaDeferredReturnValid(&EnumaDeferredSnapshot)){
      Result=Service->Run(Service,&EnumaDeferredSnapshot);
      if(Result<1||Result>5){Result=2;}
    }
  }
  // Point 28 is a deferred-menu result, NOT another driver's StartImage return.
  (VOID)EnumaBootLogAppend(&EnumaDxeBootLog,28,(UINT32)EnumaDispatchOrdinal,File,Result);
  MicroSecondDelay(2000000+5000000*((UINTN)Result-1));ResetCold();CpuDeadLoop();
}
#else
#define EnumaDeferredMenuDispatch(Handle,File,Status) do {} while (0)
#endif
#endif
