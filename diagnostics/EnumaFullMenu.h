// SPDX-License-Identifier: BSD-2-Clause-Patent
// Full pinned application from FV; application host avoids implicit ConnectAll.
#ifndef ENUMA_FULL_MENU_H
#define ENUMA_FULL_MENU_H
#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>
#include "EnumaMenuApplication.h"
#include "EnumaMenuConfigFs.h"
STATIC ENUMA_MENU_APPLICATION_CONTEXT mMenuApp;
STATIC ENUMA_MENU_CONFIG_FS mMenuFs;
STATIC EFI_HANDLE mMenuVolume;
STATIC VOID *mMenuPe;
STATIC UINTN mMenuPeSize;
STATIC EFI_GUID mMenuFsGuid=EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

STATIC EFI_STATUS FullMenuPrepare(VOID) {
  EFI_GUID FvGuid=EFI_FIRMWARE_VOLUME2_PROTOCOL_GUID;
  EFI_GUID AppGuid={0x9b665204,0xa133,0x47a2,{0x93,0x96,0x9e,0x2b,0x37,0x0a,0x4e,0x25}};
  EFI_HANDLE *Handles=NULL;UINTN Count=0,I;EFI_STATUS Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;UINT32 Auth=0;
  if(mMenuPe||mMenuVolume||mMenuApp.Active){return EFI_ALREADY_STARTED;}
  Status=gBS->LocateHandleBuffer(ByProtocol,&FvGuid,NULL,&Count,&Handles);
  if(EFI_ERROR(Status)){return Status;}
  Status=EFI_NOT_FOUND;
  for(I=0;I<Count;I++) {
    if(EFI_ERROR(gBS->HandleProtocol(Handles[I],&FvGuid,(VOID **)&Fv))){continue;}
    Status=Fv->ReadSection(Fv,&AppGuid,EFI_SECTION_PE32,0,&mMenuPe,&mMenuPeSize,&Auth);
    if(!EFI_ERROR(Status)){break;}
    if(mMenuPe){FreePool(mMenuPe);mMenuPe=NULL;}mMenuPeSize=0;
  }
  FreePool(Handles);
  if(EFI_ERROR(Status)){return Status;}
  if(!mMenuPe||!mMenuPeSize||mMenuPeSize>8*1024*1024||(Auth&EFI_AUTH_STATUS_TEST_FAILED)){
    if(mMenuPe){FreePool(mMenuPe);mMenuPe=NULL;}return EFI_SECURITY_VIOLATION;
  }
  MenuConfigInit(&mMenuFs);
  return gBS->InstallMultipleProtocolInterfaces(&mMenuVolume,&mMenuFsGuid,&mMenuFs.Protocol,NULL);
}
STATIC EFI_STATUS FullMenuRun(EFI_HANDLE Display) {
  EFI_STATUS Status;
  if(!mMenuPe||!mMenuVolume){return EFI_NOT_READY;}
  Status=EnumaMenuApplicationStartSource(&mMenuApp,gImageHandle,NULL,mMenuPe,mMenuPeSize,mMenuVolume,Display,FALSE);
  if(EFI_ERROR(Status)){return Status;}
  // Do not silently accept built-in fallback if the private config was not read.
  if(mMenuFs.ReadBytes!=sizeof(mMenuConfig)-1||mMenuFs.OpenCount||mMenuApp.Active){return EFI_DEVICE_ERROR;}
  return EFI_SUCCESS;
}
STATIC BOOLEAN FullMenuRelease(VOID) {
  EFI_STATUS Status;
  if(mMenuApp.Active||mMenuFs.OpenCount){return FALSE;}
  if(mMenuVolume){
    Status=gBS->UninstallMultipleProtocolInterfaces(mMenuVolume,&mMenuFsGuid,&mMenuFs.Protocol,NULL);
    if(EFI_ERROR(Status)){return FALSE;}mMenuVolume=NULL;
  }
  if(mMenuPe){FreePool(mMenuPe);mMenuPe=NULL;}mMenuPeSize=0;
  return TRUE;
}
#endif
