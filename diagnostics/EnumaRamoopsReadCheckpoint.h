// SPDX-License-Identifier: BSD-2-Clause-Patent
// S-017: post-MMU, three volatile header loads only, original cold return.
#ifndef ENUMA_STAGE_CHECKPOINT_H
#define ENUMA_STAGE_CHECKPOINT_H
#include <Library/BaseLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/TimerLib.h>
#if ENUMA_RAMOOPS_READ_HEADER != 1 || ENUMA_DIAG_STAGE != 10 || ENUMA_DIAG_DELAY_US != 2000000
#error Header read requires explicit stage10/delay2 read-only profile
#endif
STATIC VOID EnumaStageCheckpoint (UINTN Stage)
{
  volatile CONST UINT32 *Header;
  UINT32 Signature;
  UINT32 Start;
  UINT32 Size;
  if (Stage == 10) {
    Header = (volatile CONST UINT32 *)(UINTN)0xB0000000ULL;
    Signature = Header[0];
    Start = Header[1];
    Size = Header[2];
    (VOID)Signature;
    (VOID)Start;
    (VOID)Size;
    MicroSecondDelay (ENUMA_DIAG_DELAY_US);
    ResetCold ();
    CpuDeadLoop ();
  }
}
#endif
