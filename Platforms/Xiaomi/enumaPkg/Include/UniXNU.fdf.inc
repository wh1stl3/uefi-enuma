# SPDX-License-Identifier: BSD-2-Clause-Patent
# Included inside FV.FvMain, including the diagnostic entry wrapper.
!if $(ENUMA_RAM_PAYLOAD) == 1
  INF enumaPkg/Drivers/EnumaRamPayloadDxe/EnumaRamPayloadDxe.inf
!endif
  INF enumaPkg/Application/EnumaProbe/EnumaProbe.inf
  INF UniXNUPkg/Application/UniXNU/UniXNU.inf
  INF UniXNUPkg/Driver/UniXNUDxe/UniXNUDxe.inf
  INF DarwinBootPkg/Driver/DarwinBoot.inf
