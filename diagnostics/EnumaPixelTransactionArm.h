// SPDX-License-Identifier: BSD-2-Clause-Patent
// R-018 candidate only. Included after the bounded MDP Read/Wait callbacks.
#ifndef ENUMA_PIXEL_TRANSACTION_ARM_H
#define ENUMA_PIXEL_TRANSACTION_ARM_H
#include "InheritedVideo/PixelTransaction.h"
STATIC ENUMA_INHERITED_LIMITS mPixelLimits;
STATIC ENUMA_VIDEO_EVIDENCE mPixelVerify;
typedef struct {
  const ENUMA_INHERITED_FRAME *Frame;
  ENUMA_U32 Reads,Writes,Barriers,Holds,Validations;
  volatile ENUMA_U32 Sink;
} ENUMA_PIXEL_CONTEXT;
STATIC int PixelAddressOk(ENUMA_PIXEL_CONTEXT *C,ENUMA_U64 A) {
  ENUMA_U64 Offset;
  if(!C||!EnumaPixelFrameValid(C->Frame)||(A&3)||A<C->Frame->Base||
     A<0x9c000000ULL||A>0x9e3ffffcULL) { return 0; }
  Offset=A-C->Frame->Base;
  return Offset/C->Frame->Stride<8&&Offset%C->Frame->Stride<512;
}
STATIC int PixelRead(void *Opaque,ENUMA_U64 A,ENUMA_U32 *V) {
  ENUMA_PIXEL_CONTEXT *C=Opaque;
  if(!V||!PixelAddressOk(C,A)||C->Reads>=3072) { return 0; }
  C->Reads++;*V=*(volatile ENUMA_U32 *)(UINTN)A;C->Sink=*V;return 1;
}
STATIC int PixelWrite(void *Opaque,ENUMA_U64 A,ENUMA_U32 V) {
  ENUMA_PIXEL_CONTEXT *C=Opaque;
  if(!PixelAddressOk(C,A)||C->Writes>=2048) { return 0; }
  C->Writes++;*(volatile ENUMA_U32 *)(UINTN)A=V;return 1;
}
STATIC int PixelBarrier(void *Opaque) {
  ENUMA_PIXEL_CONTEXT *C=Opaque;
  if(!C||C->Barriers>=3) { return 0; } C->Barriers++;
  __asm__ volatile("dsb sy" ::: "memory");return 1;
}
STATIC int PixelHold(void *Opaque,ENUMA_U32 Us) {
  ENUMA_PIXEL_CONTEXT *C=Opaque;
  if(!C||C->Holds||Us!=500000||!gBS||!gBS->Stall) { return 0; }
  C->Holds++;return gBS->Stall(Us)==EFI_SUCCESS?1:0;
}
STATIC int PixelValidate(void *Opaque,const ENUMA_INHERITED_FRAME *F) {
  ENUMA_PIXEL_CONTEXT *C=Opaque;ENUMA_U32 Status,Reason=0;
  if(!C||C->Validations||!F||!EnumaPixelBytesEqual(F,&mEvidence.Frame,sizeof(*F))) { return 0; }
  C->Validations++;
  Status=EnumaSampleNativeAlphaViewDetailed(Read,Wait,NULL,&mPixelLimits,&mPixelVerify,&Reason);
  return Status==EnumaInheritedOk&&Reason==EnumaVideoReasonComplete&&
    EnumaPixelEvidenceSame(&mEvidence,&mPixelVerify);
}
STATIC ENUMA_U32 PixelArmProbe(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Policy,ENUMA_U64 El) {
  ENUMA_INHERITED_FRAME Stable=*F;ENUMA_PIXEL_CONTEXT C={&Stable,0,0,0,0,0,0};
  return EnumaPixelTransaction(&Stable,Policy,El,PixelRead,PixelWrite,PixelValidate,PixelBarrier,PixelHold,&C);
}
#endif
