# SPDX-License-Identifier: BSD-2-Clause-Patent
# Binding shared implementations, not local driver stubs.
[LibraryClasses]
  UniXNULib|UniXNUPkg/Library/UniXNULib/UniXNULib.inf
  FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf

[Components]
!ifdef ENUMA_GOP_SELFTEST
!if $(ENUMA_GOP_SELFTEST) == 1
  enumaPkg/Application/EnumaGopSelfTest/EnumaGopSelfTest.inf {
    <LibraryClasses>
      InheritedGopLib|enumaPkg/Library/InheritedGopLib/InheritedGopLib.inf
  }
!endif
!endif
!if $(ENUMA_RAM_PAYLOAD) == 1
  enumaPkg/Drivers/EnumaRamPayloadDxe/EnumaRamPayloadDxe.inf
!endif
  enumaPkg/Application/EnumaProbe/EnumaProbe.inf
  UniXNUPkg/Application/UniXNU/UniXNU.inf
  UniXNUPkg/Driver/UniXNUDxe/UniXNUDxe.inf
  DarwinBootPkg/Driver/DarwinBoot.inf

[PcdsFixedAtBuild]
!ifndef ENUMA_DIRECT_MENU
  DEFINE ENUMA_DIRECT_MENU = 0
!endif
!if $(ENUMA_DIRECT_MENU) == 1
  # Direct graphics menu: ConOut/journal replay are not entry prerequisites.
  # This selects the application only; it does not enable physical GOP or
  # remove the early diagnostic stop in diagnostic builds.
  gUniXNUPkgTokenSpaceGuid.PcdUniXNUApplicationFile|{ GUID("9B665204-A133-47A2-9396-9E2B370A4E25") }
!else
  # Reuse UniXNUDxe's boot-option registration for the diagnostic wrapper.
  gUniXNUPkgTokenSpaceGuid.PcdUniXNUApplicationFile|{ GUID("AEF8C196-230E-4E19-A306-D62E21E3D780") }
!endif
