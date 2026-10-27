// SPDX-License-Identifier: BSD-2-Clause-Patent
// Separate WT/opaque profile. Existing Mu/uncached entry remains unchanged.
#ifndef ENUMA_GOP_ACCESS_H
#define ENUMA_GOP_ACCESS_H
#include "InheritedGop.h"
typedef struct {
  UINT32 Version,Bytes;
  // Policy3 is the separately validated Normal WT profile, not an observation.
  UINT32 Policy;
  VOID *Context;
  EFI_STATUS (EFIAPI *Read32)(VOID *,UINT64,UINT32 *);
  EFI_STATUS (EFIAPI *Write32)(VOID *,UINT64,UINT32);
  EFI_STATUS (EFIAPI *Synchronize)(VOID *);
} ENUMA_GOP_ACCESS;
typedef struct {UINT64 Reads,Writes,Barriers;EFI_STATUS Fault;BOOLEAN Latched;} ENUMA_GOP_ACCESS_STATE;
// Still requires Requirements==0 and same-boot Validate of route, ownership,
// identity/WT mapping and compatible opaque blend semantics before each Blt.
// No page-table/cache-attribute setters. Sync must provide required ordering;
// no generic cache maintenance fallback. All written pixels carry ReservedMask.
EFI_STATUS EnumaGopCreateWtOpaque(CONST ENUMA_INHERITED_FRAME *,ENUMA_GOP_VALIDATE,VOID *,CONST ENUMA_GOP_ACCESS *,EFI_GRAPHICS_OUTPUT_PROTOCOL **);
EFI_STATUS EnumaGopGetAccessState(EFI_GRAPHICS_OUTPUT_PROTOCOL *,ENUMA_GOP_ACCESS_STATE *);
#endif
