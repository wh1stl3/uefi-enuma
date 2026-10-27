// SPDX-License-Identifier: BSD-2-Clause-Patent
// Included only by Dispatcher.c. The same Image.c private Info pointer that is
// about to enter OEM must match the protocol on the scheduled image handle.
#ifndef ENUMA_OEM_CORE_BINDING_H
#define ENUMA_OEM_CORE_BINDING_H
#include "EnumaOemBuildIdentity.h"
#include "EnumaOemTraceTarget.h"
#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DevicePath.h>
#include "EnumaOemLoadedIdentity.h"
#include "EnumaOemBoardCapture.h"
#include "EnumaOemProgress.h"
STATIC ENUMA_SERVICE_TRACE mOemCoreTrace;
STATIC ENUMA_PRECALL_CAPTURE mOemCoreCapture;
STATIC BOOLEAN mOemCoreAttempted;
STATIC EFI_HANDLE EnumaOemStartingFv;
STATIC EFI_HANDLE mOemProgressHandle;
VOID EnumaOemCoreSyncSystemTable(EFI_SYSTEM_TABLE *System) {
  if(EFI_ERROR(EnumaServiceTraceSyncSystem(System))){OemCaptureTerminal(NULL);}
}
VOID EnumaOemCoreAfterStart(EFI_HANDLE Handle,EFI_STATUS Status){
#if ENUMA_OEM_PROGRESS
  if(Handle==mOemProgressHandle&&Handle==EnumaStartingImage&&mOemCoreAttempted){OemProgressReturned(Status);}
#else
  (VOID)Handle;(VOID)Status;
#endif
}
STATIC __attribute__((unused)) VOID OemRuntimeBeforeForward(ENUMA_SERVICE_TRACE *Trace,UINT64 Call,EFI_TPL Tpl){
  EnumaCaptureBeforeForward(Trace,Call,Tpl);
  // Runtime first-call experiment cannot continue without a completed CPU
  // capture/restore. This is a terminal diagnostic refusal, not OEM status.
  if(ENUMA_OEM_RUNTIME&&Call==ENUMA_OEM_SELECTED_CALL&&mOemCoreCapture.State!=EnumaCaptureExported){
    EnumaCaptureStop(&mOemCoreCapture);OemCaptureTerminal(NULL);
  }
}
EFI_STATUS EnumaOemCoreBeforeEntry(EFI_HANDLE Handle,EFI_LOADED_IMAGE_PROTOCOL *Loaded,UINTN Entry){
  STATIC CONST EFI_GUID Display={0x4138022f,0x06c7,0x4f79,{0x9c,0x94,0x7e,0x33,0xb5,0x11,0xa4,0xe7}};
  EFI_LOADED_IMAGE_PROTOCOL *Actual=NULL;VOID *Fv=NULL;EFI_STATUS Status;
  CONST UINT8 *Path;
  if(!EnumaStartingFile||CompareMem(EnumaStartingFile,&Display,sizeof(Display))!=0){return EFI_SUCCESS;}
  if(!Handle||Handle!=EnumaStartingImage){return EFI_SUCCESS;} // nested other image
  if(!Loaded||
     CoreHandleProtocol(Handle,&gEfiLoadedImageProtocolGuid,(VOID **)&Actual)!=EFI_SUCCESS||Actual!=Loaded){return EFI_SECURITY_VIOLATION;}
  if(mOemCoreAttempted){return EFI_ALREADY_STARTED;}mOemCoreAttempted=TRUE;
  Path=(CONST UINT8 *)Loaded->FilePath;
  if(!EnumaOemStartingFv||Loaded->DeviceHandle!=EnumaOemStartingFv||!Path||
     Path[0]!=MEDIA_DEVICE_PATH||Path[1]!=MEDIA_PIWG_FW_FILE_DP||Path[2]!=20||Path[3]!=0||
     CompareMem(Path+4,&Display,sizeof(Display))!=0||Path[20]!=END_DEVICE_PATH_TYPE||
     Path[21]!=END_ENTIRE_DEVICE_PATH_SUBTYPE||Path[22]!=4||Path[23]!=0||
     CoreHandleProtocol(Loaded->DeviceHandle,&gEfiFirmwareVolume2ProtocolGuid,&Fv)!=EFI_SUCCESS||!Fv){return EFI_SECURITY_VIOLATION;}
  if(Loaded->SystemTable!=gDxeCoreST||!EnumaOemLoadedIdentity(Loaded,Entry)){return EFI_SECURITY_VIOLATION;}
  Status=EnumaAttachDisplayTrace(&mOemCoreTrace,EnumaStartingFile,Loaded,gDxeCoreST);
  if(EFI_ERROR(Status)){return Status;}
  mOemOriginalBoot=mOemCoreTrace.Original;
#if ENUMA_OEM_PROGRESS
  Status=OemProgressArm(&mOemCoreTrace,ENUMA_OEM_SESSION,OemProgressPresent,OemCaptureTerminal);
  if(!EFI_ERROR(Status)){mOemProgressHandle=Handle;}
#else
  Status=EnumaCaptureArm(&mOemCoreCapture,&mOemCoreTrace,ENUMA_OEM_SESSION,ENUMA_OEM_SELECTED_CALL,OemBoardSink,&mOemCoreCapture);
  if(Status==EFI_SUCCESS&&ENUMA_OEM_RUNTIME){mOemCoreTrace.BeforeForward=OemRuntimeBeforeForward;}
#endif
  return Status;
}
#endif
