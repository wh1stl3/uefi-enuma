// SPDX-License-Identifier: BSD-2-Clause-Patent
// QEMU-only invocation. Allocated RAM framebuffer, no protocol installation,
// display MMIO, physical splash pointer or board verification flag changes.
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include "../../Library/InheritedGopLib/InheritedGop.h"
STATIC BOOLEAN mReject;
STATIC EFI_STATUS EFIAPI Validate(VOID *Context,CONST ENUMA_INHERITED_FRAME *F) {
  return !mReject&&F->Base==(UINTN)Context?EFI_SUCCESS:EFI_NOT_READY;
}
EFI_STATUS EFIAPI EnumaGopSelfTest(EFI_HANDLE Handle,EFI_SYSTEM_TABLE *ST) {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *G=NULL; EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info=NULL;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color={0x12,0x34,0x56,0},Pixels[12];
  ENUMA_INHERITED_FRAME F={0}; UINT8 *Memory=NULL; UINTN Size=0,I,Y; EFI_STATUS S=EFI_DEVICE_ERROR;
  (VOID)Handle;
  Memory=AllocatePool(32+80*8+32); if(!Memory) { goto Done; }
  SetMem(Memory,32+80*8+32,0xa5);
  F.Base=(UINTN)(Memory+32); F.Bytes=80*8; F.Width=16; F.Height=8; F.Stride=80;
  F.PipeMask=3; F.CtlMask=1; F.IntfMask=6;
  F.RedMask=0xff0000; F.GreenMask=0xff00; F.BlueMask=0xff; F.ReservedMask=0xff000000;
#define CHECK(E) do { if(!(E)) { goto Done; } } while(0)
  F.Requirements=ENUMA_REQUIRE_ALL;
  CHECK(EnumaGopCreate(&F,Validate,Memory+32,&G)==EFI_NOT_READY&&G==NULL);
  F.Requirements=0; // Explicit synthetic fixture, never an actual display claim.
  CHECK(EnumaGopCreate(&F,Validate,Memory+32,&G)==EFI_SUCCESS);
  for(I=0;I<32+80*8+32;I++) { CHECK(Memory[I]==0xa5); }
  CHECK(G->QueryMode(G,1,&Size,&Info)==EFI_INVALID_PARAMETER&&Info==NULL);
  CHECK(G->QueryMode(G,0,&Size,&Info)==EFI_SUCCESS&&Info!=G->Mode->Info&&Info->PixelsPerScanLine==20);
  FreePool(Info); Info=NULL;
  CHECK(G->Blt(G,&Color,EfiBltVideoFill,0,0,2,1,3,2,0)==EFI_SUCCESS);
  CHECK(G->Blt(G,Pixels,EfiBltVideoToBltBuffer,2,1,1,1,3,2,16)==EFI_SUCCESS);
  CHECK(Pixels[5].Red==0x56&&Pixels[5].Blue==0x12&&Pixels[10].Green==0x34);
  CHECK(G->Blt(G,Pixels,EfiBltBufferToVideo,1,1,6,3,3,2,16)==EFI_SUCCESS);
  CHECK(G->Blt(G,NULL,EfiBltVideoToVideo,6,3,7,4,3,2,0)==EFI_SUCCESS);
  mReject=TRUE; CHECK(G->SetMode(G,0)==EFI_NOT_READY); mReject=FALSE;
  CHECK(G->SetMode(G,1)==EFI_UNSUPPORTED&&G->SetMode(G,0)==EFI_SUCCESS);
  for(Y=0;Y<8;Y++) {
    for(I=0;I<64;I++) { CHECK(Memory[32+Y*80+I]==0); }
    for(I=64;I<80;I++) { CHECK(Memory[32+Y*80+I]==0xa5); }
  }
  for(I=0;I<32;I++) { CHECK(Memory[I]==0xa5&&Memory[32+80*8+I]==0xa5); }
  S=EFI_SUCCESS;
Done:
  if(Info) { FreePool(Info); } EnumaGopDestroy(G); if(Memory) { FreePool(Memory); }
  if(ST&&ST->ConOut) { ST->ConOut->OutputString(ST->ConOut,EFI_ERROR(S)?L"ENUMA_GOP_SELFTEST FAIL\r\n":L"ENUMA_GOP_SELFTEST PASS RAM ONLY\r\n"); }
  // This application is not selected or launched in normal enuma firmware.
  gRT->ResetSystem(EfiResetShutdown,S,0,NULL);
  return S;
}
