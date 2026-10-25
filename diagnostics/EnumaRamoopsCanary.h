// SPDX-License-Identifier: BSD-2-Clause-Patent
// R-017: only called at the explicit post-MMU DxeCore stage10 stop.
#ifndef ENUMA_RAMOOPS_CANARY_H
#define ENUMA_RAMOOPS_CANARY_H
#include "EnumaRamoopsBuffer.h"

#if ENUMA_DIAG_STAGE != 10 || ENUMA_DIAG_DELAY_US != 2000000
#error R-017 requires exactly stage10/delay2
#endif

STATIC UINTN EnumaWriteRamoopsCanary (VOID)
{
  CONST UINT32 Words[4] = { ENUMA_RAMOOPS_NONCE_0, ENUMA_RAMOOPS_NONCE_1,
                           ENUMA_RAMOOPS_NONCE_2, ENUMA_RAMOOPS_NONCE_3 };
  CONST UINT8 Prefix[] = "ENUMA-R017:";
  CONST UINT8 Hex[] = "0123456789abcdef";
  UINT8 Marker[44];
  UINTN Index;
  UINTN Word;
  UINTN Written;
  BOOLEAN InterruptState;
  for (Index = 0; Index < sizeof (Prefix) - 1; Index++) {
    Marker[Index] = Prefix[Index];
  }
  for (Word = 0; Word < 4; Word++) {
    for (Index = 0; Index < 8; Index++) {
      Marker[11 + Word * 8 + Index] = Hex[(Words[Word] >> (28 - Index * 4)) & 15];
    }
  }
  Marker[43] = '\n';
  InterruptState = SaveAndDisableInterrupts ();
  Written = EnumaRamoopsAppend ((volatile VOID *)(UINTN)0xB0000000ULL,
                               0x200000U, Marker, sizeof (Marker));
  SetInterruptState (InterruptState);
  return Written;
}
#endif
