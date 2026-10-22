# enuma OEM memory and numeric configuration. No foreign-board table.
[LibraryClasses]
  MemoryMapLib|enumaPkg/Library/MemoryMapLib/MemoryMapLib.inf
  ConfigurationMapLib|enumaPkg/Library/ConfigurationMapLib/ConfigurationMapLib.inf
  # Native GOP is established later. Avoid early framebuffer writes with guessed format.
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf

[PcdsFixedAtBuild]
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x9ff90000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x40000
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|1600
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferHeight|2560

# DisplayDxe and K81 raw resources come from this enuma's XBL.
# No assumed framebuffer address/pitch/color depth is supplied to SimpleFb.

!ifndef ENUMA_VENDOR_DSC
  !error ENUMA_VENDOR_DSC must bind the extracted enuma modules
!endif
!include $(ENUMA_VENDOR_DSC)
