# enuma 厂商 DXE 模块的 APRIORI 派发顺序。
#
# 依据：EnumaInputs/Vendor.apriori.inc（由 UniXNU-enuma/scripts/enuma_vendor_import.py
# 从 enuma 实机 xbl_a 的 FV / uefiplat.cfg 生成）。厂商模块使用扁平布局
# （Binaries/enuma 下同名目录的 <Module>.inf），非厂商模块（MdeModulePkg/ArmPkg/
# EmbeddedPkg/SiliciumPkg/FatPkg）保持 pipa 派生结构不变。
#
# enuma XBL 无以下 pipa 派生模块，已移除（NO-SRC，不伪造）：
#   SdccDxe, FeatureEnablerDxe, PILProxyDxe, PILDxe, SPSSDxe, AdcDxe,
#   UsbPwrCtrlDxe, QcomChargerDxe, ChargerExDxe, UsbfnDwc3Dxe,
#   XhciPciEmulationDxe, XhciDxe, UsbMsdDxe, UsbDeviceDxe, UsbConfigDxe,
#   UsbInitDxe, ButtonsDxe, TsensDxe, LimitsDxe
#
# 复位驱动：enuma 使用厂商 ResetRuntimeDxe（Vendor.apriori.inc），替换原 pipa 在此
# 槽位的 MdeModulePkg ResetSystemRuntimeDxe（后者仍保留在 DXE.inc 作为通用 ResetSystem API）。
#
APRIORI DXE {
  INF MdeModulePkg/Core/Dxe/DxeMain.inf
  INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf

  INF Binaries/enuma/EnvDxe/EnvDxe.inf

  INF MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  INF MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  INF EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf

  INF Binaries/enuma/SmemDxe/SmemDxe.inf
  INF Binaries/enuma/DALSys/DALSys.inf
  INF Binaries/enuma/HWIODxeDriver/HWIODxeDriver.inf
  INF Binaries/enuma/ChipInfo/ChipInfo.inf
  INF Binaries/enuma/PlatformInfoDxeDriver/PlatformInfoDxeDriver.inf
  INF Binaries/enuma/HALIOMMU/HALIOMMU.inf
  INF Binaries/enuma/ULogDxe/ULogDxe.inf
  INF Binaries/enuma/CmdDbDxe/CmdDbDxe.inf
  INF Binaries/enuma/PwrUtilsDxe/PwrUtilsDxe.inf
  INF Binaries/enuma/NpaDxe/NpaDxe.inf
  INF Binaries/enuma/RpmhDxe/RpmhDxe.inf
  INF Binaries/enuma/PdcDxe/PdcDxe.inf
  INF Binaries/enuma/VcsDxe/VcsDxe.inf
  INF Binaries/enuma/ClockDxe/ClockDxe.inf
  INF Binaries/enuma/SPMI/SPMI.inf
  INF Binaries/enuma/ShmBridgeDxe/ShmBridgeDxe.inf
  INF Binaries/enuma/ScmDxe/ScmDxe.inf
  INF Binaries/enuma/DALTLMM/DALTLMM.inf

  INF Binaries/enuma/ResetRuntimeDxe/ResetRuntimeDxe.inf

  INF Binaries/enuma/PmicDxe/PmicDxe.inf

  INF MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  INF MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  INF MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  INF Binaries/enuma/UFSDxe/UFSDxe.inf

  INF FatPkg/EnhancedFatDxe/Fat.inf

  INF Binaries/enuma/TzDxe/TzDxe.inf

  INF MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf

  INF Binaries/enuma/DDRInfoDxe/DDRInfoDxe.inf
  INF Binaries/enuma/SPI/SPI.inf
  INF Binaries/enuma/I2C/I2C.inf

!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  INF Binaries/enuma/DisplayDxe/DisplayDxe.inf
!else
  INF SiliciumPkg/Drivers/SimpleFbDxe/SimpleFbDxe.inf
!endif

!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  INF Binaries/enuma/CPRDxe/CPRDxe.inf
!endif

  INF MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  INF MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  INF EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  INF EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  INF MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  INF MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  INF MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  INF MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  INF MdeModulePkg/Universal/BdsDxe/BdsDxe.inf

  INF Binaries/enuma/GpiDxe/GpiDxe.inf
  INF Binaries/enuma/HashDxe/HashDxe.inf
  INF Binaries/enuma/CipherDxe/CipherDxe.inf
  INF Binaries/enuma/MacDxe/MacDxe.inf
  INF Binaries/enuma/RngDxe/RngDxe.inf

  INF EmbeddedPkg/SimpleTextInOutSerial/SimpleTextInOutSerial.inf
  INF MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  INF MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  INF MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
}
