// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_DEFERRED_MENU_PROVIDER_H
#define ENUMA_DEFERRED_MENU_PROVIDER_H
#include "EnumaDeferredMenu.h"
STATIC ENUMA_INHERITED_FRAME mDeferredFrame;
STATIC BOOLEAN mDeferredConsumed;
STATIC ENUMA_DEFERRED_MENU_PROTOCOL mDeferredMenu;
STATIC UINT32 EFIAPI DeferredMenuRun(ENUMA_DEFERRED_MENU_PROTOCOL *This,CONST ENUMA_BOOT_LOG *Snapshot) {
  if(This!=&mDeferredMenu||mDeferredConsumed){return 1;}
  mDeferredConsumed=TRUE;
  // Re-check CPU policy after returning from driver entry. Native route is
  // revalidated again by MenuValidate immediately before the first pixel store.
  if(!EnumaDeferredReturnValid(Snapshot)||!DeferredPolicyValid(&mDeferredFrame)){return 1;}
  return RunMenuWindow(&mDeferredFrame);
}
STATIC EFI_STATUS PublishDeferredMenu(EFI_HANDLE Image,const ENUMA_INHERITED_FRAME *Frame) {
  EFI_GUID Guid=ENUMA_DEFERRED_MENU_GUID;
  if(!Image||!Frame||mDeferredMenu.Owner){return EFI_ALREADY_STARTED;}
  mDeferredFrame=*Frame;mDeferredMenu.Version=1;mDeferredMenu.Bytes=sizeof(mDeferredMenu);
  mDeferredMenu.Owner=Image;mDeferredMenu.Run=DeferredMenuRun;
  // Install on the actual loaded driver handle. No stack addresses survive
  // RegisterFrameBuffer; the callback allocates its own synchronous context.
  return gBS->InstallProtocolInterface(&Image,&Guid,EFI_NATIVE_INTERFACE,&mDeferredMenu);
}
#endif
