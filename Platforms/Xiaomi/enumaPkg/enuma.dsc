##
# Derived from UniXNU/uefi-sheng alioth.dsc at 329b473df1e5.
# Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.
# Copyright (c) 2014, Linaro Limited. All rights reserved.
# Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.
# Copyright (c) 2018, Bingxing Wang. All rights reserved.
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
[Defines]
  PLATFORM_NAME = enuma
  PLATFORM_GUID = 4B8A1A2F-8F36-4D16-8E90-A25081C251E1
  PLATFORM_VERSION = 0.1
  DSC_SPECIFICATION = 0x00010005
  OUTPUT_DIRECTORY = Build/enumaPkg
  SUPPORTED_ARCHITECTURES = AARCH64
  BUILD_TARGETS = DEBUG
  SKUID_IDENTIFIER = DEFAULT
  FLASH_DEFINITION = enumaPkg/enuma.fdf
  USE_CUSTOM_DISPLAY_DRIVER = 1
  SOC_TYPE = 2

# This is a source overlay, not an alioth memory-map copy. The reviewed file
# must supply MemoryMapLib, ConfigurationMapLib, DDR/stack and GOP facts.
!ifndef ENUMA_PLATFORM_CONFIG
  !error ENUMA_PLATFORM_CONFIG is missing: memory/GOP configuration needs review
!endif

!include KonaPkg/KonaPkg.dsc.inc
!include $(ENUMA_PLATFORM_CONFIG)
!include enumaPkg/Include/UniXNU.dsc.inc

[BuildOptions]
  *_*_AARCH64_CC_FLAGS = -DENUMA_PRESERVE_DISPLAY=1 -DENUMA_DIAG_STAGE=$(ENUMA_DIAG_STAGE) -DENUMA_DIAG_DELAY_US=$(ENUMA_DIAG_DELAY_US) -DENUMA_DIAG_DISPATCH_ORDINAL=$(ENUMA_DIAG_DISPATCH_ORDINAL)

[PcdsFixedAtBuild]
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemManufacturer|"Xiaomi"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemModel|"Xiaomi Pad 5 Pro 5G"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"enuma"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailSku|"enuma-research"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemBoardModel|"enuma"
  gQcomPkgTokenSpaceGuid.PcdPlatformType|"LA"
