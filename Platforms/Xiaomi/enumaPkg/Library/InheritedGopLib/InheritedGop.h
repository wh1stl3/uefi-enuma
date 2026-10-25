// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_INHERITED_GOP_H
#define ENUMA_INHERITED_GOP_H
#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include "../InheritedScanoutLib/InheritedScanout.h"
// Recheck same-boot ownership/mapping/route before every operation. Must be
// bounded, non-blocking and safe at TPL_NOTIFY. Only EFI_SUCCESS permits access.
typedef EFI_STATUS (EFIAPI *ENUMA_GOP_VALIDATE)(VOID *,CONST ENUMA_INHERITED_FRAME *);
// A qualified, CPU identity-mapped UNCACHED linear video frame is required.
// Creation configures software only; no pixels, MMIO or protocols are touched.
// Board caller owns qualification and install/uninstall/DriverBinding lifetime.
EFI_STATUS EnumaGopCreate(CONST ENUMA_INHERITED_FRAME *,ENUMA_GOP_VALIDATE,VOID *,EFI_GRAPHICS_OUTPUT_PROTOCOL **);
// Only call after uninstalling all published interfaces and closing consumers.
VOID EnumaGopDestroy(EFI_GRAPHICS_OUTPUT_PROTOCOL *);
#endif
