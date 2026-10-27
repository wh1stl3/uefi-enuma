// SPDX-License-Identifier: BSD-2-Clause-Patent
// Included only by Dispatcher.c. The same Image.c private Info pointer that is
// about to enter OEM must match the protocol on the scheduled image handle.
#ifndef ENUMA_OEM_CORE_BINDING_H
#define ENUMA_OEM_CORE_BINDING_H
#include "EnumaOemTraceTarget.h"
#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DevicePath.h>
#include "EnumaOemLoadedIdentity.h"
#include "EnumaOemBoardCapture.h"
STATIC ENUMA_SERVICE_TRACE mOemCoreTrace;
STATIC ENUMA_PRECALL_CAPTURE mOemCoreCapture;
STATIC BOOLEAN mOemCoreAttempted;
STATIC EFI_HANDLE EnumaOemStartingFv;
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
  // Compile/link scope: stage10 blocks all dispatch. These explicit selectors
  // are placeholders bound by build metadata, never a physical trial nonce.
  Status=EnumaCaptureArm(&mOemCoreCapture,&mOemCoreTrace,1,0,OemBoardSink,&mOemCoreCapture);
  return Status;
}
#endif
