// SPDX-License-Identifier: BSD-2-Clause-Patent
#include "InheritedGopPublish.h"
#include <Protocol/DevicePath.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct {
  VENDOR_DEVICE_PATH Vendor;
  EFI_DEVICE_PATH_PROTOCOL End;
} ENUMA_DISPLAY_PATH;
// Exact identity used by Configuration/BootDevices.h and BDS ConOut/ErrOut.
STATIC CONST ENUMA_DISPLAY_PATH DisplayPath={
  {{HARDWARE_DEVICE_PATH,HW_VENDOR_DP,{sizeof(VENDOR_DEVICE_PATH),0}},EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID},
  {END_DEVICE_PATH_TYPE,END_ENTIRE_DEVICE_PATH_SUBTYPE,{sizeof(EFI_DEVICE_PATH_PROTOCOL),0}}
};
struct ENUMA_GOP_PUBLICATION {
  EFI_HANDLE Handle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  ENUMA_DISPLAY_PATH Path;
};
EFI_HANDLE EnumaGopPublicationHandle(CONST ENUMA_GOP_PUBLICATION *P) { return P?P->Handle:NULL; }

STATIC EFI_STATUS Publish(CONST ENUMA_INHERITED_FRAME *Frame,ENUMA_GOP_VALIDATE Validate,VOID *Context,CONST ENUMA_GOP_ACCESS *Access,ENUMA_GOP_PUBLICATION **Out) {
  EFI_STATUS Status; EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop=NULL; ENUMA_GOP_PUBLICATION *P;
  if (!Out) { return EFI_INVALID_PARAMETER; }
  if (*Out) { return EFI_ALREADY_STARTED; }
  if (!gBS || !gBS->InstallMultipleProtocolInterfaces || !gBS->UninstallMultipleProtocolInterfaces) { return EFI_NOT_READY; }
  Status=Access?EnumaGopCreateWtOpaque(Frame,Validate,Context,Access,&Gop):EnumaGopCreate(Frame,Validate,Context,&Gop);
  if (Status!=EFI_SUCCESS) { return Status; }
  P=AllocateZeroPool(sizeof(*P));
  if (!P) { EnumaGopDestroy(Gop); return EFI_OUT_OF_RESOURCES; }
  P->Gop=Gop; CopyMem(&P->Path,&DisplayPath,sizeof(DisplayPath));
  // InstallMultiple rolls back the entire pair on error. Until success the
  // owned GOP/path are never handed to a controller connection routine.
  Status=gBS->InstallMultipleProtocolInterfaces(&P->Handle,
    &gEfiGraphicsOutputProtocolGuid,P->Gop,&gEfiDevicePathProtocolGuid,&P->Path,NULL);
  if (EFI_ERROR(Status)) { EnumaGopDestroy(Gop); FreePool(P); return Status; }
  *Out=P; return Status;
}
EFI_STATUS EnumaGopPublish(CONST ENUMA_INHERITED_FRAME *F,ENUMA_GOP_VALIDATE V,VOID *C,ENUMA_GOP_PUBLICATION **Out) {return Publish(F,V,C,NULL,Out);}
EFI_STATUS EnumaGopPublishWtOpaque(CONST ENUMA_INHERITED_FRAME *F,ENUMA_GOP_VALIDATE V,VOID *C,CONST ENUMA_GOP_ACCESS *A,ENUMA_GOP_PUBLICATION **Out) {
  if(!A){return EFI_INVALID_PARAMETER;}return Publish(F,V,C,A,Out);
}

EFI_STATUS EnumaGopUnpublish(ENUMA_GOP_PUBLICATION **Publication) {
  ENUMA_GOP_PUBLICATION *P; EFI_STATUS Status;
  if (!Publication || !*Publication) { return EFI_INVALID_PARAMETER; }
  if (!gBS || !gBS->UninstallMultipleProtocolInterfaces) { return EFI_NOT_READY; }
  P=*Publication;
  Status=gBS->UninstallMultipleProtocolInterfaces(P->Handle,
    &gEfiGraphicsOutputProtocolGuid,P->Gop,&gEfiDevicePathProtocolGuid,&P->Path,NULL);
  if (Status!=EFI_SUCCESS) { return Status; }
  EnumaGopDestroy(P->Gop); FreePool(P); *Publication=NULL;
  return EFI_SUCCESS;
}
