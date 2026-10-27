// SPDX-License-Identifier: BSD-2-Clause-Patent
// Offline-qualified attachment helper, NOT wired into Dispatcher or Image.c.
#ifndef ENUMA_OEM_TRACE_TARGET_H
#define ENUMA_OEM_TRACE_TARGET_H
#include <Protocol/LoadedImage.h>
#include "EnumaOemServiceTrace.h"
STATIC EFI_STATUS EnumaAttachDisplayTrace(ENUMA_SERVICE_TRACE *Context,
  CONST EFI_GUID *File,EFI_LOADED_IMAGE_PROTOCOL *Loaded,EFI_SYSTEM_TABLE *System) {
  STATIC CONST EFI_GUID Display={0x4138022f,0x06c7,0x4f79,{0x9c,0x94,0x7e,0x33,0xb5,0x11,0xa4,0xe7}};
  UINTN I;EFI_STATUS Status;
  if(!File||!Loaded||!System||!Context){return EFI_INVALID_PARAMETER;}
  for(I=0;I<sizeof(Display);I++){
    if(((CONST UINT8 *)File)[I]!=((CONST UINT8 *)&Display)[I]){return EFI_NOT_FOUND;}
  }
  // The future Core hook supplies File and Loaded from the SAME scheduled
  // image. An INF GUID alone does not authenticate arbitrary supplied bytes.
  if(Loaded->SystemTable!=System||Loaded->ImageCodeType!=EfiBootServicesCode||
     !Loaded->ImageBase||!Loaded->ImageSize||Loaded->ImageSize>MAX_UINTN){return EFI_UNSUPPORTED;}
  Status=EnumaServiceTracePrepare(Context,System,(UINTN)Loaded->ImageBase,(UINTN)Loaded->ImageSize);
  if(Status!=EFI_SUCCESS){return Status;}
  Loaded->SystemTable=&Context->System;
  // Context survives all driver returns, protocol callbacks and events.
  // Never restore/free it solely because StartImage returned EFI_SUCCESS.
  return EFI_SUCCESS;
}
#endif
