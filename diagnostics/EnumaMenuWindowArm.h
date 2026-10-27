// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_MENU_WINDOW_ARM_H
#define ENUMA_MENU_WINDOW_ARM_H
#include "InheritedVideo/MenuWindow.h"
typedef struct {const ENUMA_INHERITED_FRAME *Frame;UINT32 Reads,Writes,Barriers,Validations;} ENUMA_MENU_CONTEXT;
STATIC int MenuAddressOk(ENUMA_MENU_CONTEXT *C,ENUMA_U64 A) {
  ENUMA_U64 Offset;
  if(!C||!EnumaPixelFrameValid(C->Frame)||(A&3)||A<C->Frame->Base) {return 0;}
  Offset=A-C->Frame->Base;
  return Offset/C->Frame->Stride>=ENUMA_MENU_Y&&Offset/C->Frame->Stride<ENUMA_MENU_Y+ENUMA_MENU_HEIGHT&&
    Offset%C->Frame->Stride>=ENUMA_MENU_X*4&&Offset%C->Frame->Stride<(ENUMA_MENU_X+ENUMA_MENU_WIDTH)*4;
}
STATIC int MenuRead(void *Opaque,ENUMA_U64 A,ENUMA_U32 *V) {
  ENUMA_MENU_CONTEXT *C=Opaque;
  if(!V||!MenuAddressOk(C,A)||C->Reads>=14u*ENUMA_MENU_PIXELS) {return 0;}
  C->Reads++;*V=*(volatile UINT32 *)(UINTN)A;return 1;
}
STATIC int MenuWrite(void *Opaque,ENUMA_U64 A,ENUMA_U32 V) {
  ENUMA_MENU_CONTEXT *C=Opaque;
  if(!MenuAddressOk(C,A)||C->Writes>=13u*ENUMA_MENU_PIXELS) {return 0;}
  C->Writes++;*(volatile UINT32 *)(UINTN)A=V;return 1;
}
STATIC int MenuBarrier(void *Opaque) {
  ENUMA_MENU_CONTEXT *C=Opaque;
  if(!C||C->Barriers>=ENUMA_MENU_MAX_CALLS+2) {return 0;}
  C->Barriers++;__asm__ volatile("dsb sy" ::: "memory");return 1;
}
STATIC int MenuValidate(void *Opaque,const ENUMA_INHERITED_FRAME *Frame) {
  ENUMA_MENU_CONTEXT *C=Opaque;ENUMA_U32 Status,Reason=0;
  if(!C||C->Validations||!EnumaPixelBytesEqual(Frame,&mEvidence.Frame,sizeof(*Frame))) {return 0;}
  C->Validations++;
  Status=EnumaSampleNativeAlphaViewDetailed(Read,Wait,NULL,&mPixelLimits,&mPixelVerify,&Reason);
  return Status==EnumaInheritedOk&&Reason==EnumaVideoReasonComplete&&EnumaPixelEvidenceSame(&mEvidence,&mPixelVerify);
}
UINT32 EnumaBoundedMenu(const ENUMA_INHERITED_FRAME *,ENUMA_PIXEL_READ,ENUMA_PIXEL_WRITE,ENUMA_PIXEL_BARRIER,ENUMA_PIXEL_REVALIDATE,VOID *);
STATIC UINT32 RunMenuWindow(const ENUMA_INHERITED_FRAME *F) {
  ENUMA_INHERITED_FRAME Stable=*F;ENUMA_MENU_CONTEXT C={&Stable,0,0,0,0};
  return EnumaBoundedMenu(&Stable,MenuRead,MenuWrite,MenuBarrier,MenuValidate,&C);
}
#if ENUMA_DEFERRED_MENU
STATIC BOOLEAN DeferredPolicyValid(const ENUMA_INHERITED_FRAME *Frame) {
  ENUMA_U64 El=0;
  return EnumaCpuStage1ArmDetailed(Frame,&El)==3&&El==4;
}
#include "EnumaDeferredMenuProvider.h"
#endif
#endif
