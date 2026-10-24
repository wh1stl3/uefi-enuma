// SPDX-License-Identifier: BSD-2-Clause-Patent
// Diagnostic-only reset gates. Zero leaves the production path unchanged.
// Reuses the fixed Mu TimerLib and ArmPsciResetSystemLib, no MMIO marker.
#ifndef ENUMA_STAGE_CHECKPOINT_H
#define ENUMA_STAGE_CHECKPOINT_H
#include <Library/BaseLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/TimerLib.h>
#ifndef ENUMA_DIAG_STAGE
#define ENUMA_DIAG_STAGE 0
#endif
#ifndef ENUMA_DIAG_DELAY_US
#define ENUMA_DIAG_DELAY_US 2000000
#endif
#if ENUMA_DIAG_DELAY_US != 2000000 && ENUMA_DIAG_DELAY_US != 7000000
#error Unsupported enuma diagnostic delay
#endif
#if ENUMA_DIAG_STAGE != 0 && (ENUMA_DIAG_STAGE < 3 || ENUMA_DIAG_STAGE > 13)
#error Unsupported enuma diagnostic stop stage
#endif
#ifndef ENUMA_DIAG_DELAY_FROM_DISPLAY
#define ENUMA_DIAG_DELAY_FROM_DISPLAY 0
#endif
#ifndef ENUMA_DIAG_DISPLAY_VALUE_INDEX
#define ENUMA_DIAG_DISPLAY_VALUE_INDEX 0
#endif
#if ENUMA_DIAG_DELAY_FROM_DISPLAY != 0 && ENUMA_DIAG_DELAY_FROM_DISPLAY != 1
#error Unsupported enuma display delay mode
#endif
#if ENUMA_DIAG_DISPLAY_VALUE_INDEX < 0 || ENUMA_DIAG_DISPLAY_VALUE_INDEX > 7
#error Unsupported enuma display value index
#endif
STATIC VOID EnumaStageCheckpoint (UINTN Stage)
{
  if (ENUMA_DIAG_STAGE != 0 && Stage == ENUMA_DIAG_STAGE) {
    UINTN Delay = ENUMA_DIAG_DELAY_US;
    //
    // R-008/A: the read runs inside the very branch that has already been shown
    // to execute (the configured delay is honoured here), so it cannot be lost to
    // a macro that never reaches this translation unit. Read-only, no wait, no
    // write. A stall here means the display window was unreachable by the time
    // this stage was reached.
    //
    if (Stage == 10 || Stage == 11 || Stage == 12) {
      (VOID)*(volatile UINT32 *)0x0ae6b800U;
      (VOID)*(volatile UINT32 *)0x0ae6c000U;
    }
#if ENUMA_DIAG_DELAY_FROM_DISPLAY
    if (Stage == 13) {
      //
      // R-008: carry one value out of the display window through the reset
      // delay. The window is primed with a single access first, because that
      // first access is exactly what used to hang the bus in this context.
      // Four bits per run, one second per step, so the host can read the value
      // back from the return time without any extra channel.
      //
      // The probes are the MDP version register and the SDE base-address
      // registers of the first two hardware pipes. The address registers are
      // 16-byte aligned, so their low nibble carries no information and the
      // encoded nibble is taken from bits 4 and up instead.
      //
      STAT CONST UINTN DisplayProbeAddress[8] = {
        0x0ae00000U, 0x0ae05014U, 0x0ae05014U, 0x0ae05014U,
        0x0ae07014U, 0x0ae07014U, 0x0ae07014U, 0x0ae6b800U
      };
      STAT CONST UINTN DisplayProbeShift[8] = { 4U, 4U, 8U, 12U, 4U, 8U, 12U, 4U };
      UINTN ProbeAddress = DisplayProbeAddress[ENUMA_DIAG_DISPLAY_VALUE_INDEX];
      UINTN ProbeShift = DisplayProbeShift[ENUMA_DIAG_DISPLAY_VALUE_INDEX];
      (VOID)*(volatile UINT32 *)0x0ae6b800U;
      Delay = 2000000U +
              ((((*(volatile UINT32 *)ProbeAddress) >> ProbeShift) & 0x0fU) * 1000000U);
    }
#endif
    MicroSecondDelay (Delay);
    ResetCold ();
    CpuDeadLoop (); // Reset returned: never accidentally continue the probe.
  }
}
#endif
