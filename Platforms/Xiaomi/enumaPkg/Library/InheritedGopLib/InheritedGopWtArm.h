// SPDX-License-Identifier: BSD-2-Clause-Patent
// Access mechanism only. Caller must establish WT/identity/ownership/blend/route
// through same-boot Validate; this header does not certify any address/mapping.
#ifndef ENUMA_GOP_WT_ARM_H
#define ENUMA_GOP_WT_ARM_H
#include "InheritedGopAccess.h"
#if defined(MDE_CPU_AARCH64)
typedef struct {UINT64 Base,Bytes;} ENUMA_GOP_WT_WINDOW;
STATIC inline BOOLEAN EnumaWtAddress(CONST ENUMA_GOP_WT_WINDOW *W,UINT64 A) {
  return W&&W->Base&&W->Bytes>=4&&W->Base<=MAX_UINT64-W->Bytes&&!(A&3)&&A>=W->Base&&A-W->Base<=W->Bytes-4;
}
STATIC EFI_STATUS EFIAPI EnumaWtRead(VOID *Context,UINT64 A,UINT32 *V) {
  if(!V||!EnumaWtAddress(Context,A)){return EFI_INVALID_PARAMETER;}
  *V=*(volatile UINT32 *)(UINTN)A;return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI EnumaWtWrite(VOID *Context,UINT64 A,UINT32 V) {
  if(!EnumaWtAddress(Context,A)){return EFI_INVALID_PARAMETER;}
  *(volatile UINT32 *)(UINTN)A=V;return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI EnumaWtSync(VOID *Context) {
  if(!Context){return EFI_INVALID_PARAMETER;}
  __asm__ volatile("dsb sy" ::: "memory");return EFI_SUCCESS;
}
STATIC inline ENUMA_GOP_ACCESS EnumaWtAccess(ENUMA_GOP_WT_WINDOW *Window) {
  ENUMA_GOP_ACCESS A={1,sizeof(A),3,Window,EnumaWtRead,EnumaWtWrite,EnumaWtSync};return A;
}
#endif
#endif
