// SPDX-License-Identifier: BSD-2-Clause-Patent
// Private diagnostic transport through the existing driver/Core return path.
// These warnings are markers, not descriptions of glyph or stale-data errors.
#ifndef ENUMA_GOP_RESULT_H
#define ENUMA_GOP_RESULT_H
#define ENUMA_GOP_RESULT_PASS ((EFI_STATUS) 1) // EFI_WARN_UNKNOWN_GLYPH
#define ENUMA_GOP_RESULT_FAIL ((EFI_STATUS) 5) // EFI_WARN_STALE_DATA
#define ENUMA_GOP_RESULT(Status) \
  (EFI_ERROR (Status) ? ENUMA_GOP_RESULT_FAIL : ENUMA_GOP_RESULT_PASS)
#endif
