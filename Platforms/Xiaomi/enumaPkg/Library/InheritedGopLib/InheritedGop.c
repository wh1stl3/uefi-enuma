// SPDX-License-Identifier: BSD-2-Clause-Patent
// Actual Mu FrameBufferBltLib backend. No panel programming or CTL_START writes.
#include "InheritedGop.h"
#include <Library/BaseMemoryLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL Gop;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION Info;
  FRAME_BUFFER_CONFIGURE *Config;
  ENUMA_INHERITED_FRAME Frame;
  ENUMA_GOP_VALIDATE Validate;
  VOID *Context;
} ENUMA_GOP;
STATIC EFI_STATUS Ready(ENUMA_GOP *G) {
  EFI_STATUS S=G->Validate(G->Context,&G->Frame);
  return S==EFI_SUCCESS?EFI_SUCCESS:EFI_NOT_READY;
}
STATIC EFI_STATUS EFIAPI Query(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,UINT32 Number,UINTN *Size,EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info) {
  ENUMA_GOP *G=(VOID *)This; EFI_STATUS S;
  if(!This||!Size||!Info) { return EFI_INVALID_PARAMETER; }
  *Size=0; *Info=NULL;
  if(Number!=0) { return EFI_INVALID_PARAMETER; }
  S=Ready(G); if(EFI_ERROR(S)) { return S; }
  *Info=AllocateCopyPool(sizeof(G->Info),&G->Info);
  if(!*Info) { return EFI_OUT_OF_RESOURCES; }
  *Size=sizeof(G->Info); return EFI_SUCCESS;
}
STATIC BOOLEAN Rect(UINTN X,UINTN Y,UINTN W,UINTN H,UINTN BoundW,UINTN BoundH) {
  return X<BoundW&&Y<BoundH&&W<=BoundW-X&&H<=BoundH-Y;
}
STATIC EFI_STATUS EFIAPI Blt(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer,EFI_GRAPHICS_OUTPUT_BLT_OPERATION Op,UINTN Sx,UINTN Sy,UINTN Dx,UINTN Dy,UINTN W,UINTN H,UINTN Delta) {
  ENUMA_GOP *G=(VOID *)This; EFI_STATUS S; EFI_TPL Tpl; UINTN X,Y;
  if(!This||!W||!H||(UINT32)Op>=EfiGraphicsOutputBltOperationMax) { return EFI_INVALID_PARAMETER; }
  if(Op!=EfiBltVideoToVideo&&!Buffer) { return EFI_INVALID_PARAMETER; }
  if((Op==EfiBltVideoToBltBuffer||Op==EfiBltVideoToVideo)&&!Rect(Sx,Sy,W,H,G->Frame.Width,G->Frame.Height)) { return EFI_INVALID_PARAMETER; }
  if(Op!=EfiBltVideoToBltBuffer&&!Rect(Dx,Dy,W,H,G->Frame.Width,G->Frame.Height)) { return EFI_INVALID_PARAMETER; }
  if(Op==EfiBltVideoToBltBuffer||Op==EfiBltBufferToVideo) {
    X=Op==EfiBltBufferToVideo?Sx:Dx; Y=Op==EfiBltBufferToVideo?Sy:Dy;
    if(W>MAX_UINTN/4||X>MAX_UINTN/4-W) { return EFI_INVALID_PARAMETER; }
    if(!Delta) { Delta=W*4; }
    if((Delta&3)||Delta<(X+W)*4||Y>MAX_UINTN-H||Y+H>MAX_UINTN/Delta) { return EFI_INVALID_PARAMETER; }
  }
  Tpl=gBS->RaiseTPL(TPL_NOTIFY);
  S=Ready(G);
  if(!EFI_ERROR(S)) { S=FrameBufferBlt(G->Config,Buffer,Op,Sx,Sy,Dx,Dy,W,H,Delta); }
  gBS->RestoreTPL(Tpl);
  return S;
}
STATIC EFI_STATUS EFIAPI SetMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,UINT32 Number) {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Black={0}; ENUMA_GOP *G=(VOID *)This;
  if(!This) { return EFI_INVALID_PARAMETER; }
  if(Number) { return EFI_UNSUPPORTED; }
  // Even mode 0 must clear the visible area, per GOP contract. Blt gates access.
  return Blt(This,&Black,EfiBltVideoFill,0,0,0,0,G->Frame.Width,G->Frame.Height,0);
}
VOID EnumaGopDestroy(EFI_GRAPHICS_OUTPUT_PROTOCOL *This) {
  ENUMA_GOP *G=(VOID *)This;
  if(!G) { return; }
  if(G->Config) { FreePool(G->Config); }
  FreePool(G);
}
EFI_STATUS EnumaGopCreate(CONST ENUMA_INHERITED_FRAME *F,ENUMA_GOP_VALIDATE Validate,VOID *Context,EFI_GRAPHICS_OUTPUT_PROTOCOL **Out) {
  ENUMA_GOP *G; EFI_STATUS S; UINTN Size=0; UINT32 Masks[4],Union=0; UINTN I;
  if(!Out) { return EFI_INVALID_PARAMETER; } *Out=NULL;
  if(!F||!Validate||!gBS||!gBS->RaiseTPL||!gBS->RestoreTPL) { return EFI_INVALID_PARAMETER; }
  if(F->Requirements||F->ManualCtlMask||!F->PipeMask||!F->CtlMask||F->IntfMask!=6) { return EFI_NOT_READY; }
  if(!F->Width||F->Width>65535||!F->Height||F->Height>65535||!F->Base||(F->Base&3)||
     (F->Stride&3)||F->Stride<F->Width*4||F->Bytes!=(UINT64)F->Stride*F->Height||
     F->Bytes>MAX_UINTN||F->Base>MAX_UINTN-F->Bytes) { return EFI_UNSUPPORTED; }
  Masks[0]=F->RedMask; Masks[1]=F->GreenMask; Masks[2]=F->BlueMask; Masks[3]=F->ReservedMask;
  for(I=0;I<4;I++) {
    if((Masks[I]!=0xff&&Masks[I]!=0xff00&&Masks[I]!=0xff0000&&Masks[I]!=0xff000000)||(Union&Masks[I])) { return EFI_UNSUPPORTED; }
    Union|=Masks[I];
  }
  S=Validate(Context,F); if(S!=EFI_SUCCESS) { return EFI_NOT_READY; }
  G=AllocateZeroPool(sizeof(*G)); if(!G) { return EFI_OUT_OF_RESOURCES; }
  G->Frame=*F; G->Validate=Validate; G->Context=Context;
  G->Gop.QueryMode=Query; G->Gop.SetMode=SetMode; G->Gop.Blt=Blt; G->Gop.Mode=&G->Mode;
  G->Mode.MaxMode=1; G->Mode.Info=&G->Info; G->Mode.SizeOfInfo=sizeof(G->Info);
  G->Mode.FrameBufferBase=F->Base; G->Mode.FrameBufferSize=(UINTN)F->Bytes;
  G->Info.HorizontalResolution=F->Width; G->Info.VerticalResolution=F->Height;
  G->Info.PixelsPerScanLine=F->Stride/4; G->Info.PixelFormat=PixelBitMask;
  G->Info.PixelInformation.RedMask=F->RedMask; G->Info.PixelInformation.GreenMask=F->GreenMask;
  G->Info.PixelInformation.BlueMask=F->BlueMask; G->Info.PixelInformation.ReservedMask=F->ReservedMask;
  S=FrameBufferBltConfigure((VOID *)(UINTN)F->Base,&G->Info,NULL,&Size);
  if(S!=RETURN_BUFFER_TOO_SMALL||!Size) { S=EFI_UNSUPPORTED; goto Fail; }
  G->Config=AllocatePool(Size); if(!G->Config) { S=EFI_OUT_OF_RESOURCES; goto Fail; }
  S=FrameBufferBltConfigure((VOID *)(UINTN)F->Base,&G->Info,G->Config,&Size);
  if(S!=RETURN_SUCCESS) { S=EFI_UNSUPPORTED; goto Fail; }
  *Out=&G->Gop; return EFI_SUCCESS;
Fail:
  EnumaGopDestroy(&G->Gop); return S;
}
