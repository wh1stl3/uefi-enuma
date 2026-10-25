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
  # Reuse UniXNUDxe's boot-option registration for the diagnostic wrapper.
  gUniXNUPkgTokenSpaceGuid.PcdUniXNUApplicationFile|{ GUID("AEF8C196-230E-4E19-A306-D62E21E3D780") }
