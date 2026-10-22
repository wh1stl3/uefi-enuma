# SPDX-License-Identifier: BSD-2-Clause-Patent
# Common CPU/timer path reused from Mu. No foreign-board binary is named.
APRIORI DXE {
  INF MdeModulePkg/Core/Dxe/DxeMain.inf
  INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  # Must list qualified enuma vendor protocols and their dependency order.
!ifndef ENUMA_VENDOR_APRIORI
  !error ENUMA_VENDOR_APRIORI is missing: enuma vendor closure is not available
!endif
  !include $(ENUMA_VENDOR_APRIORI)
}
