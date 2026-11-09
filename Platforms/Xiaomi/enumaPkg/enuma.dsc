##
#  Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.
#  Copyright (c) 2018, Bingxing Wang. All rights reserved.
#  Copyright (c) Microsoft Corporation.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = enuma
  PLATFORM_GUID                  = 44840A31-C9D0-4FD6-B204-47FB186CEBC1
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/enumaPkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = RELEASE|DEBUG
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = enumaPkg/enuma.fdf
  # 2026-11-08 实机：厂商 DisplayDxe 一进入即挂住并熄灭屏幕，且 ABL 留下的显示一直可用
  #（SEC/DXE 日志在屏上可见），故放弃自定义显示驱动，改回 SimpleFbDxe。
  USE_CUSTOM_DISPLAY_DRIVER      = 0

  #
  # 0 = SM8250
  # 1 = SM8250-AB
  # 2 = SM8250-AC
  #
  SOC_TYPE                       = 2

!include KonaPkg/KonaPkg.dsc.inc

[PcdsFixedAtBuild]
  gUniXNUPkgTokenSpaceGuid.PcdUniXNUDeviceModel|"enuma"
  #
  # UniXNULog defaults to 0x40 (DEBUG_INFO) which the global PcdDebugPrintErrorLevel=0x8007EE0F drops; use the enabled DEBUG_LOAD (0x4) so UNIXNU: lines reach AdvancedLogger/ramoops.
  gUniXNUPkgTokenSpaceGuid.PcdUniXNUDebugLevel|0x00000004
  # DDR Memory
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000

  #
  # UEFI Stack
  #
  # 0x9FF90000/0x40000 与 enuma 原厂 XBL uefiplat.cfg 的 "UEFI Stack" 行一致，
  # 同 SoC 的 pipa/alioth/nabu 取值相同（用户 2026-11-08 确认保持）。
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x9FF90000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x40000

  #
  # SMBIOS
  #
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemManufacturer|"Xiaomi"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemModel|"Pad 5 Pro 5G"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"enuma"
  # SKU 取实机 ro.product.model；Board 按 alioth 规则（SKU 去掉 "M2012" 型的前缀）派生，仅影响显示。
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailSku|"M2105K81C"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemBoardModel|"K81C"

  #
  # Simple Frame Buffer
  #
  # Xiaomi Pad 5 Pro 11" 的公开面板规格；以实机 GOP/DisplayDxe 模式为准。
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|1600
    gSiliciumPkgTokenSpaceGuid.PcdFrameBufferHeight|2560
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferColorDepth|32

  #
  # Platform PEI
  #
  gQcomPkgTokenSpaceGuid.PcdPlatformType|"LA"

[LibraryClasses]
  UniXNULib|UniXNUPkg/Library/UniXNULib/UniXNULib.inf
  #
  # Memory Libraries
  #
  MemoryMapLib|enumaPkg/Library/MemoryMapLib/MemoryMapLib.inf

  #
  # Debug output also in the stock kernel's ramoops console zone (0xB0000000); an ASSERT restarts into fastboot with a
  # warm reset, so the log survives a hang that the user ends with the keys
  #
  RamLogLib|enumaPkg/Library/RamoopsLogLib/RamoopsLogLib.inf

  #
  # QCOM Libraries
  #
  ConfigurationMapLib|enumaPkg/Library/ConfigurationMapLib/ConfigurationMapLib.inf

  #
  # Advanced Logger (firmware log exporter, Tools -> Advanced Logger)
  #
  # The in-memory log buffer is allocated dynamically by the DXE Core logger
  # instance; PcdAdvancedLoggerBase stays at its default 0 (no fabricated fixed
  # address). The hardware port routes logger output to the platform SerialPortLib
  # (FrameBufferSerialPortLib in DEBUG) so early screen output is preserved.
  AdvancedLoggerAccessLib|AdvLoggerPkg/Library/AdvancedLoggerAccessLib/AdvancedLoggerAccessLib.inf
  AdvancedLoggerHdwPortLib|AdvLoggerPkg/Library/AdvancedLoggerHdwPortLib/AdvancedLoggerHdwPortLib.inf
  QrEncoderLib|MsGraphicsPkg/Library/QrEncoderLib/QrEncoderLib.inf
  AssertLib|AdvLoggerPkg/Library/AssertLib/AssertLib.inf
!if $(TARGET) == DEBUG
  DebugLib|AdvLoggerPkg/Library/BaseDebugLibAdvancedLogger/BaseDebugLibAdvancedLogger.inf
!endif

[LibraryClasses.common.SEC]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/PeilessArm/AdvancedLoggerLib.inf
  # Sec.c calls BaseDebugLibSerialPortConstructor() directly, so the SEC phase keeps
  # the serial DebugLib (early screen output) and does not take the AdvancedLogger DebugLib.
  !if $(TARGET) == DEBUG
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  !endif

[LibraryClasses.common.DXE_CORE]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/DxeCore/AdvancedLoggerLib.inf

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Dxe/AdvancedLoggerLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  AdvancedLoggerLib|AdvLoggerPkg/Library/AdvancedLoggerLib/Runtime/AdvancedLoggerLib.inf

[Components]
  UniXNUPkg/Application/UniXNU/UniXNU.inf
  UniXNUPkg/Driver/UniXNUDxe/UniXNUDxe.inf
  UniXNUPkg/Driver/UniXNURamDiskDxe/UniXNURamDiskDxe.inf
  UniXNUPkg/Driver/AdvancedLogDxe/AdvancedLogDxe.inf
  DarwinBootPkg/Driver/DarwinBoot.inf
  #
  # ACPI Tables
  #
  # TODO: enuma/AcpiTables.inf —— 需要本机 ACPI 表后再启用
