// SPDX-License-Identifier: BSD-2-Clause-Patent
// Borrow an existing GOP; never install an alias onto the console splitter.
#ifndef ENUMA_CONSOLE_GOP_H
#define ENUMA_CONSOLE_GOP_H
#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
STATIC inline EFI_STATUS EnumaLookupConsoleGop(EFI_BOOT_SERVICES *Services,EFI_HANDLE ConsoleHandle,EFI_GRAPHICS_OUTPUT_PROTOCOL **Out) {
  EFI_STATUS Status=EFI_NOT_FOUND;EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop=NULL;
  if(!Out){return EFI_INVALID_PARAMETER;}*Out=NULL;
  if(!Services||!Services->HandleProtocol||!Services->LocateProtocol){return EFI_NOT_READY;}
  if(ConsoleHandle){Status=Services->HandleProtocol(ConsoleHandle,&gEfiGraphicsOutputProtocolGuid,(VOID **)&Gop);}
  if(Status!=EFI_SUCCESS||!Gop){
    Gop=NULL;Status=Services->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,NULL,(VOID **)&Gop);
  }
  if(Status!=EFI_SUCCESS){return Status;}
  if(!Gop||!Gop->QueryMode||!Gop->SetMode||!Gop->Blt||!Gop->Mode||!Gop->Mode->Info||
     Gop->Mode->SizeOfInfo<sizeof(*Gop->Mode->Info)||!Gop->Mode->MaxMode||Gop->Mode->Mode>=Gop->Mode->MaxMode||
     !Gop->Mode->Info->HorizontalResolution||!Gop->Mode->Info->VerticalResolution){return EFI_NOT_READY;}
  *Out=Gop;return EFI_SUCCESS;
}
#endif
