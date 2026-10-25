// SPDX-License-Identifier: BSD-2-Clause-Patent
// NEW S-008 isolation control: same INF/GUID/constructors, constant body only.
// No map lookup, MMIO, pixels, wait, allocation or protocol publication.
#include <Uefi.h>
#include "EnumaGopResult.h"
EFI_STATUS EFIAPI RegisterFrameBuffer(EFI_HANDLE Image,EFI_SYSTEM_TABLE *ST) {
  (VOID)Image; (VOID)ST;
  return ENUMA_GOP_RESULT_PASS;
}
