// SPDX-License-Identifier: BSD-2-Clause-Patent
// R-005 proposal only. No new library/constructor or display-register write.
#ifndef ENUMA_PIXEL_PREVIEW_ARM_H
#define ENUMA_PIXEL_PREVIEW_ARM_H
#include "EnumaPixelPreview.h"
#include <Library/UefiBootServicesTableLib.h>
STATIC UINT32 EnumaPixelRegisterRead (UINTN A) { return *(volatile UINT32 *)A; }
STATIC EFI_STATUS EnumaPixelSnapshot (ENUMA_SCANOUT_DESCRIPTOR *D)
{ return EnumaScanoutFindBoundedDescriptor (EnumaPixelRegisterRead, D); }
STATIC UINT32 EnumaPixelRead (UINTN A) { return *(volatile UINT32 *)A; }
STATIC VOID EnumaPixelWrite (UINTN A, UINT32 V) { *(volatile UINT32 *)A = V; }
STATIC VOID EnumaPixelSync (UINTN A, UINTN Bytes)
{
  UINT64 Ctr;
  UINTN Line, P, End = A + Bytes;
  __asm__ __volatile__ ("dsb sy\n\tmrs %0, ctr_el0" : "=r" (Ctr) :: "memory");
  Line = (UINTN)4 << ((Ctr >> 16) & 15);
  P = A & ~(Line - 1);
  for (; P < End; P += Line) { __asm__ __volatile__ ("dc civac, %0" :: "r" (P) : "memory"); }
  __asm__ __volatile__ ("dsb sy" ::: "memory");
}
STATIC EFI_STATUS EnumaPixelWait (UINTN Microseconds) { return gBS->Stall (Microseconds); }
STATIC EFI_STATUS EnumaPixelPreviewArm (UINT32 *Scratch, UINTN ScratchPixels)
{
  if (gBS == NULL || gBS->Stall == NULL) { return EFI_NOT_READY; }
  return EnumaPixelPreview (EnumaPixelSnapshot, EnumaPixelRead, EnumaPixelWrite,
    EnumaPixelSync, EnumaPixelWait, Scratch, ScratchPixels);
}
#endif
