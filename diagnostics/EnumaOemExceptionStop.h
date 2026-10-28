// SPDX-License-Identifier: BSD-2-Clause-Patent
// Runtime OEM experiment: stop before formatting or another pixel/MMIO access.
#ifndef ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#define ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#include <Library/ResetSystemLib.h>
#include <Library/BaseLib.h>
#ifndef ENUMA_OEM_EXCEPTION
#define ENUMA_OEM_EXCEPTION 0
#endif
#if ENUMA_OEM_EXIT_CLOCK
#include "EnumaOemExitClock.h"
#endif
#if ENUMA_OEM_EXCEPTION
#include <Library/UefiBootServicesTableLib.h>
#include "EnumaOemExceptionIdentity.h"
#if !defined(ENUMA_EXCEPTION_HOST_FIXTURE)
#include "EnumaOemExceptionDisplay.h"
#endif
STATIC volatile BOOLEAN mExActive;
STATIC ENUMA_EXCEPTION_RECORD mExRecord;
STATIC __attribute__((noinline,noreturn)) VOID EnumaUnhandledExceptionCheckpoint(
    EFI_EXCEPTION_TYPE Type,EFI_SYSTEM_CONTEXT Context){
  // Latch BEFORE touching a possibly corrupt context or display/metadata.
  if(mExActive){ResetCold();for(;;){CpuPause();}}
  mExActive=TRUE;
#if !defined(ENUMA_EXCEPTION_HOST_FIXTURE)
  __asm__ volatile("msr daifset, #15":::"memory");
#endif
  if(ExSnapshot(&mExRecord,(UINTN)Type,Context.SystemContextAArch64,ENUMA_EXCEPTION_SESSION)&&
     ExPresent(&mExRecord,1)){
    // Raw PC/ESR/FAR is already visible before optional ownership reads.
    ExFindOwner(&mExRecord,(UINTN)gST,ExReadMeta,NULL);
    if(ExPresent(&mExRecord,2)){ExPresent(&mExRecord,3);}
  }
  EnumaOemClockExit(1);
}
#else
STATIC VOID EnumaUnhandledExceptionCheckpoint(VOID){
#if ENUMA_OEM_EXIT_CLOCK
  EnumaOemClockExit(1);
#else
  ResetCold();for(;;){CpuPause();}
#endif
}
#endif
#endif
