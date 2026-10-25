# SPDX-License-Identifier: BSD-2-Clause-Patent
# Included inside FV.FvMain, including the diagnostic entry wrapper.
!ifdef ENUMA_GOP_SELFTEST
!if $(ENUMA_GOP_SELFTEST) == 1
  INF enumaPkg/Application/EnumaGopSelfTest/EnumaGopSelfTest.inf
!endif
!endif
!if $(ENUMA_RAM_PAYLOAD) == 1
  INF enumaPkg/Drivers/EnumaRamPayloadDxe/EnumaRamPayloadDxe.inf
!endif
  INF enumaPkg/Application/EnumaProbe/EnumaProbe.inf
  INF UniXNUPkg/Application/UniXNU/UniXNU.inf
  INF UniXNUPkg/Driver/UniXNUDxe/UniXNUDxe.inf
  INF DarwinBootPkg/Driver/DarwinBoot.inf
