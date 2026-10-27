// No display, MMIO, retained RAM or Boot Services dependency. Not a watchdog.
#ifndef ENUMA_OEM_EXIT_CLOCK_H
#define ENUMA_OEM_EXIT_CLOCK_H
#include <Uefi.h>
#include <Library/TimerLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/BaseLib.h>
#ifndef ENUMA_OEM_CLOCK_FORCE
#define ENUMA_OEM_CLOCK_FORCE 0
#endif
#if ENUMA_OEM_CLOCK_FORCE != 0 && ENUMA_OEM_CLOCK_FORCE != 1 && ENUMA_OEM_CLOCK_FORCE != 5
#error Invalid clock calibration constant
#endif
STATIC UINTN EnumaOemClockDelay(UINT32 Code){
  if(ENUMA_OEM_CLOCK_FORCE){Code=ENUMA_OEM_CLOCK_FORCE;}
  if(Code<1||Code>5){Code=2;}
  return 2000000U+5000000U*(Code-1);
}
STATIC inline UINT32 EnumaOemClockClass(BOOLEAN Owner,BOOLEAN Returned,EFI_STATUS Status){
  if(!Owner||!Returned){return 3;}
  return Status==EFI_SUCCESS?5:4;
}
STATIC __attribute__((noinline,noreturn)) VOID EnumaOemClockExit(UINT32 Code){
  MicroSecondDelay(EnumaOemClockDelay(Code));
  ResetCold();
  for(;;){CpuPause();}
}
#endif
