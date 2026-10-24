// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_TRANSPORT_REPORT_H
#define ENUMA_TRANSPORT_REPORT_H
#define ENUMA_TRANSPORT_REPORT_GUID {0xc36a09fd,0xce63,0x47b2,{0x88,0xc3,0x97,0x17,0x28,0x57,0x9c,0x16}}
#define ENUMA_TRANSPORT_STEPS 13
typedef struct ENUMA_TRANSPORT_REPORT ENUMA_TRANSPORT_REPORT;
struct ENUMA_TRANSPORT_REPORT {
  UINT32 Version;
  UINT32 Passed; // Passed/pending distinguish EFI_NOT_READY from a real result.
  EFI_STATUS Status[ENUMA_TRANSPORT_STEPS];
  UINT64 Dtb, InitrdStart, InitrdEnd, FatStart, FatBytes;
  UINTN Reads;
  EFI_STATUS (EFIAPI *CheckFiles)(ENUMA_TRANSPORT_REPORT *Report);
};
#endif
