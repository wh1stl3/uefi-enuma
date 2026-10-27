// SPDX-License-Identifier: BSD-2-Clause-Patent
// Explicit child-image console binding; caller owns provider lifetime.
#ifndef ENUMA_MENU_APPLICATION_H
#define ENUMA_MENU_APPLICATION_H
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/GraphicsOutput.h>

typedef struct {
  EFI_SYSTEM_TABLE Table;
  EFI_HANDLE Child;
  BOOLEAN Active;
} ENUMA_MENU_APPLICATION_CONTEXT;

// Caller keeps Context and Display alive until Active is FALSE. No physical
// admission is performed here: the display provider owns all access checks.
// This launches the pinned UniXNU application, not arbitrary resident drivers.
STATIC EFI_STATUS EnumaMenuApplicationStartSource(
  ENUMA_MENU_APPLICATION_CONTEXT *Context, EFI_HANDLE Parent,
  EFI_DEVICE_PATH_PROTOCOL *Path, VOID *Source, UINTN SourceSize, EFI_HANDLE ConfigVolume,
  EFI_HANDLE Display, BOOLEAN FirmwareHost) {
  EFI_STATUS Status, Cleanup; EFI_LOADED_IMAGE_PROTOCOL *Loaded;
  BOOLEAN Started=FALSE;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  STATIC CHAR16 Marker[]=L"unixnu:firmware";
  if(!Context||Context->Active||!Parent||(!Path&&(!Source||!SourceSize))||!Display){return EFI_INVALID_PARAMETER;}
  Status=gBS->HandleProtocol(Display,&gEfiGraphicsOutputProtocolGuid,(VOID **)&Gop);
  if(EFI_ERROR(Status)||!Gop||!Gop->Blt||!Gop->Mode||!Gop->Mode->Info){return EFI_NOT_READY;}
  Context->Child=NULL;
  Status=gBS->LoadImage(FALSE,Parent,Path,Source,SourceSize,&Context->Child);
  if(EFI_ERROR(Status)&&!Context->Child){return Status;}
  Context->Active=TRUE;
  // A security violation can return a loaded (but unstarted) handle.
  if(EFI_ERROR(Status)){goto Cleanup;}
  Status=gBS->HandleProtocol(Context->Child,&gEfiLoadedImageProtocolGuid,(VOID **)&Loaded);
  if(EFI_ERROR(Status)){goto Cleanup;}
  if(Loaded->ImageCodeType!=EfiLoaderCode){Status=EFI_UNSUPPORTED;goto Cleanup;}
  CopyMem(&Context->Table,gST,sizeof(Context->Table));
  Context->Table.ConsoleOutHandle=Display;Context->Table.ConOut=NULL;
  Context->Table.ConsoleInHandle=NULL;Context->Table.ConIn=NULL;
  Context->Table.Hdr.CRC32=0;
  Status=gBS->CalculateCrc32(&Context->Table,Context->Table.Hdr.HeaderSize,&Context->Table.Hdr.CRC32);
  if(EFI_ERROR(Status)){goto Cleanup;}
  // CoreStartImage passes LoadedImage.SystemTable to the child entry; changing
  // only the caller's gST does not cross the independently linked image boundary.
  Loaded->SystemTable=&Context->Table;
  if(ConfigVolume){Loaded->DeviceHandle=ConfigVolume;}
  Loaded->LoadOptions=FirmwareHost?Marker:NULL;
  Loaded->LoadOptionsSize=FirmwareHost?sizeof(Marker):0;
  Started=TRUE;Status=gBS->StartImage(Context->Child,NULL,NULL);
Cleanup:
  // Applications normally unload on return. If still present, unload before
  // allowing the caller to withdraw the GOP or release the private table.
  if(!Started||gBS->HandleProtocol(Context->Child,&gEfiLoadedImageProtocolGuid,(VOID **)&Loaded)==EFI_SUCCESS){
    Cleanup=gBS->UnloadImage(Context->Child);
    if(EFI_ERROR(Cleanup)){return Cleanup;}
  }
  Context->Active=FALSE;Context->Child=NULL;
  return Status;
}
STATIC inline EFI_STATUS EnumaMenuApplicationStart(ENUMA_MENU_APPLICATION_CONTEXT *C,EFI_HANDLE Parent,
  EFI_DEVICE_PATH_PROTOCOL *Path,EFI_HANDLE Display,BOOLEAN FirmwareHost) {
  return EnumaMenuApplicationStartSource(C,Parent,Path,NULL,0,NULL,Display,FirmwareHost);
}
#endif
