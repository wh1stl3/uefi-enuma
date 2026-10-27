#ifndef ENUMA_OEM_CLOCK_CAPTURE_H
#define ENUMA_OEM_CLOCK_CAPTURE_H
#include "EnumaOemExitClock.h"
STATIC EFI_BOOT_SERVICES *mOemOriginalBoot;
STATIC VOID OemCaptureTerminal(VOID *C){(VOID)C;EnumaOemClockExit(2);}
STATIC BOOLEAN OemProgressPresent(UINT32 Phase,CONST ENUMA_SERVICE_EVENT *E,UINT64 Session){
  (VOID)Phase;(VOID)E;(VOID)Session;return FALSE; // no screen path in this scope
}
#endif
