# enuma 厂商 DXE 模块的 APRIORI 派发顺序。
#
# 依据：EnumaInputs/Vendor.apriori.inc（由 UniXNU-enuma/scripts/enuma_vendor_import.py
# 从 enuma 实机 xbl_a 的 FV / uefiplat.cfg 生成）。厂商模块使用扁平布局
# （Binaries/enuma 下同名目录的 <Module>.inf），非厂商模块（MdeModulePkg/ArmPkg/
# EmbeddedPkg/SiliciumPkg/FatPkg）保持 pipa 派生结构不变。
#
# 模块清单与 APRIORI 顺序来自 enuma 实机 xbl_a（52 个厂商模块；xbl_a 内
# QcomChargerDxe 名为 QcomChargerDxeLA、XhciPciEmulationDxe 名为 XhciPciEmulation）。
# 更正（2026-11-08）：以下 19 个模块实为 xbl_a 内原件，此前只因 extract_enuma_xbl.py
# 的 SELECT 白名单遗漏而被误判为「enuma XBL 无」并删除；现已按原件导入恢复：
#   SdccDxe, FeatureEnablerDxe, PILProxyDxe, PILDxe, SPSSDxe, AdcDxe,
#   UsbPwrCtrlDxe, QcomChargerDxeLA, ChargerExDxe, UsbfnDwc3Dxe,
#   XhciPciEmulation, XhciDxe, UsbMsdDxe, UsbDeviceDxe, UsbConfigDxe,
#   UsbInitDxe, ButtonsDxe, TsensDxe, LimitsDxe
# UsbDeviceDxe / UsbInitDxe / UsbMsdDxe 不在 xbl_a 的 APRIORI 列表里，只进 DXE.inc。
#
# 复位驱动：与 pipa 一致，使用 MdeModulePkg ResetSystemRuntimeDxe（DXE.inc 同样包含它）。
# 厂商 ResetRuntimeDxe 不放入 FV：两个实现都会安装 Reset 架构协议，且此前它只出现在本文件而
# 不在 FV 里（FVMAIN 无其 GUID，reviewer 2026-11-08 发现）。复位回 bootloader 的实际行为需实机验证。
#
# FeatureEnablerDxe 已移除（2026-11-08 实机）：TzDxe 提供 QSEECOM 后，FeatureEnablerDxe 加载
# featenabler TA 时 QseeComDxe.c +131 `Status == 0` 断言（ERROR C90000002:V03000007 I0
# 6925FAD3-E243-4B69-AD61-3F978C8ABC60）。Windows 功能使能 TA，UniXNU 用不到；二进制与 INF
# 保留在 Binaries/enuma/FeatureEnablerDxe，只是不进 FV。
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

  INF MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf

  INF Binaries/enuma/PmicDxe/PmicDxe.inf

  INF MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  INF MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  INF MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  INF Binaries/enuma/SdccDxe/SdccDxe.inf
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

  INF Binaries/enuma/PILProxyDxe/PILProxyDxe.inf
  INF Binaries/enuma/PILDxe/PILDxe.inf
  INF Binaries/enuma/SPSSDxe/SPSSDxe.inf

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

  INF Binaries/enuma/AdcDxe/AdcDxe.inf
  INF Binaries/enuma/UsbPwrCtrlDxe/UsbPwrCtrlDxe.inf
  INF Binaries/enuma/QcomChargerDxeLA/QcomChargerDxeLA.inf
  INF Binaries/enuma/ChargerExDxe/ChargerExDxe.inf
  INF Binaries/enuma/UsbfnDwc3Dxe/UsbfnDwc3Dxe.inf
  INF Binaries/enuma/XhciPciEmulation/XhciPciEmulation.inf
  INF Binaries/enuma/XhciDxe/XhciDxe.inf
  INF Binaries/enuma/UsbConfigDxe/UsbConfigDxe.inf
  INF Binaries/enuma/ButtonsDxe/ButtonsDxe.inf
  INF Binaries/enuma/TsensDxe/TsensDxe.inf
  INF Binaries/enuma/LimitsDxe/LimitsDxe.inf

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
