APRIORI DXE {
  INF MdeModulePkg/Core/Dxe/DxeMain.inf
  INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf

  INF Binaries/enuma/QcomPkg/Drivers/EnvDxe/EnvDxe.inf

  INF MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  INF MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  INF EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf

  INF Binaries/enuma/QcomPkg/Drivers/SmemDxe/SmemDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/DALSYSDxe/DALSYSDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/HWIODxe/HWIODxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/ChipInfoDxe/ChipInfoDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/PlatformInfoDxe/PlatformInfoDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/HALIOMMUDxe/HALIOMMUDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/ULogDxe/ULogDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/CmdDbDxe/CmdDbDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/PwrUtilsDxe/PwrUtilsDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/NpaDxe/NpaDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/RpmhDxe/RpmhDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/PdcDxe/PdcDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/VcsDxe/VcsDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/ClockDxe/ClockDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/SPMIDxe/SPMIDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/ShmBridgeDxe/ShmBridgeDxeLA.inf
  INF Binaries/enuma/QcomPkg/Drivers/TzDxe/ScmDxeLA.inf
  INF Binaries/enuma/QcomPkg/Drivers/TLMMDxe/TLMMDxe.inf

  INF MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf

  INF Binaries/enuma/QcomPkg/Drivers/PmicDxe/PmicDxeLa.inf

  INF MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  INF MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  INF MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  INF Binaries/enuma/QcomPkg/Drivers/SdccDxe/SdccDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/UFSDxe/UFSDxe.inf

  INF FatPkg/EnhancedFatDxe/Fat.inf

  INF Binaries/enuma/QcomPkg/Drivers/TzDxe/TzDxeLA.inf

  INF MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf

  INF Binaries/enuma/QcomPkg/Drivers/FeatureEnablerDxe/FeatureEnablerDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/DDRInfoDxe/DDRInfoDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/SPIDxe/SPIDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/I2CDxe/I2CDxe.inf

!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  INF Binaries/enuma/QcomPkg/Drivers/DisplayDxe/DisplayDxe.inf
!else
  INF SiliciumPkg/Drivers/SimpleFbDxe/SimpleFbDxe.inf
!endif

  INF Binaries/enuma/QcomPkg/Drivers/PILProxyDxe/PILProxyDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/PILDxe/PILDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/SPSSDxe/SPSSDxe.inf
!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  INF Binaries/enuma/QcomPkg/Drivers/CPRDxe/CPRDxe.inf
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

  INF Binaries/enuma/QcomPkg/Drivers/AdcDxe/AdcDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/UsbPwrCtrlDxe/UsbPwrCtrlDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/QcomChargerDxe/QcomChargerDxeLA.inf
  INF Binaries/enuma/QcomPkg/Drivers/ChargerExDxe/ChargerExDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/UsbfnDwc3Dxe/UsbfnDwc3Dxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/XhciPciEmulationDxe/XhciPciEmulationDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/XhciDxe/XhciDxe.inf

  INF MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  INF MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  INF MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  INF Binaries/enuma/QcomPkg/Drivers/UsbMsdDxe/UsbMsdDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/UsbDeviceDxe/UsbDeviceDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/UsbConfigDxe/UsbConfigDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/UsbInitDxe/UsbInitDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/ButtonsDxe/ButtonsDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/TsensDxe/TsensDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/LimitsDxe/LimitsDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/GpiDxe/GpiDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/HashDxe/HashDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/CipherDxe/CipherDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/MacDxe/MacDxe.inf
  INF Binaries/enuma/QcomPkg/Drivers/RNGDxe/RngDxe.inf

  INF EmbeddedPkg/SimpleTextInOutSerial/SimpleTextInOutSerial.inf
  INF MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  INF MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  INF MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
}
