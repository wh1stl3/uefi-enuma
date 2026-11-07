/*
 * enumaPkg 平台配置表 —— 取自 Mu-Silicium SM8250 通用值（IMEM、CPU 数、EUD、UFS 等）。
 * Sdc*GpioConfigOn/Off、SharedIMEMBaseAddr、EUDEnableAddr、SecurityFlag、NumCpusFuseAddr、
 * NumCpus、UefiMemUseThreshold 已与 enuma 原厂 XBL（xbl_a sha256 7ac67cbf…c65057）内嵌
 * uefiplat.cfg 的 [ConfigParameters] 逐项核对（2026-11-08）。全表 31 项，现与 cfg 完全一致：
 * EnableUfsIOC 取 cfg 的 0（此前沿用 pipa 的 1）。EnableLogFsSyncInRetail 保持 cfg 的 1：
 * 厂商固件正常启动的附带写入与 stock 相同，不为此偏离（见 docs/launch-route「写入分级」）。
 */
#include <Library/ConfigurationMapLib.h>

STATIC
EFI_CONFIGURATION_ENTRY_DESCRIPTOR
gConfigurationDescriptor[] = {
  // Configuration Map
  {"NumCpusFuseAddr", 0x5C04C},
  {"EnableShell", 0x1},
  {"EUDEnableAddr", 0x88E2000},
  {"SecPagePoolCount", 0x800},
  {"SharedIMEMBaseAddr", 0x146BF000},
  {"DloadCookieAddr", 0x01FD3000},
  {"DloadCookieValue", 0x10},
  {"NumCpus", 8},
  {"NumActiveCores", 8},
  {"MaxLogFileSize", 0x400000},
  {"UefiMemUseThreshold", 0x77},
  {"USBHS1_Config", 0x0},
  {"UsbFnIoRevNum", 0x00010001},
  {"PwrBtnShutdownFlag", 0x0},
  {"Sdc1GpioConfigOn", 0x1E92},
  {"Sdc2GpioConfigOn", 0x1E92},
  {"Sdc1GpioConfigOff", 0xA00},
  {"Sdc2GpioConfigOff", 0xA00},
  {"EnableSDHCSwitch", 0x1},
  {"EnableUfsIOC", 0}, // enuma XBL uefiplat.cfg L117 = 0 (pipa/alioth use 1)
  {"UfsSmmuConfigForOtherBootDev", 1},
  {"SecurityFlag", 0xC4},
  {"EnableLogFsSyncInRetail", 0x1},
  {"ShmBridgememSize", 0xA00000},
  {"EnableMultiThreading", 1},
  {"MaxCoreCnt", 8},
  {"EarlyInitCoreCnt", 1},
  {"EnableUefiSecAppDebugLogDump", 0x0},
  {"AllowNonPersistentVarsInRetail", 0x1},
  {"EnableDisplayThread", 0x1},
  {"EnableDisplayImageFv", 0x0}
};

VOID
GetConfigurationMap (
  OUT EFI_CONFIGURATION_ENTRY_DESCRIPTOR **ConfigurationDescriptor,
  OUT UINT8                               *ConfigurationDescriptorCount)
{
  // Pass Data
  *ConfigurationDescriptor      = gConfigurationDescriptor;
  *ConfigurationDescriptorCount = ARRAY_SIZE (gConfigurationDescriptor);
}
