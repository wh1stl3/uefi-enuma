// SPDX-License-Identifier: BSD-2-Clause-Patent
// Separate timing transport. Constants never call Read; no pixel/cache access.
#ifndef ENUMA_VALUE_PROBE_H
#define ENUMA_VALUE_PROBE_H
typedef UINT32 (*ENUMA_VALUE_READ)(UINTN Address);
typedef EFI_STATUS (*ENUMA_VALUE_WAIT)(UINTN Microseconds);
STATIC inline EFI_STATUS EnumaValueProbe (
  UINTN Source, UINTN Index, UINTN Shift, UINTN Constant,
  ENUMA_VALUE_READ Read, ENUMA_VALUE_WAIT Wait)
{
  CONST UINTN Addresses[7] = {
    0x0ae00000U, 0x0ae05014U, 0x0ae07014U, 0x0ae09014U,
    0x0ae0b014U, 0x0ae25014U, 0x0ae27014U
  };
  UINT32 Code;
  if (Wait == NULL || Source < 1 || Source > 2 || Constant > 15 ||
      Index >= 7 || Shift > 28 || (Shift & 3) != 0) { return EFI_INVALID_PARAMETER; }
  if (Source == 1) {
    Code = (UINT32)Constant;
  } else {
    if (Read == NULL || Constant != 0) { return EFI_INVALID_PARAMETER; }
    // Historical priming is an experimental variable, not a safety guarantee.
    (VOID)Read (0x0ae6b800U);
    Code = (Read (Addresses[Index]) >> Shift) & 15U;
  }
  // Zero code performs no extra wait. DxeCore owns the ordinary 2-second return.
  return Code == 0 ? EFI_SUCCESS : Wait ((UINTN)Code * 3000000U);
}
#endif
