#include <Uefi.h>
#include <Library/ConfigurationMapLib.h>

STATIC EFI_CONFIGURATION_ENTRY_DESCRIPTOR mConfig[] = {
  {"NumCpusFuseAddr", 0x5c04cULL},
  {"EnableShell", 0x1ULL},
  {"EUDEnableAddr", 0x88e2000ULL},
  {"SecPagePoolCount", 0x800ULL},
  {"SharedIMEMBaseAddr", 0x146bf000ULL},
  {"DloadCookieAddr", 0x1fd3000ULL},
  {"DloadCookieValue", 0x10ULL},
  {"NumCpus", 0x8ULL},
  {"NumActiveCores", 0x8ULL},
  {"MaxLogFileSize", 0x400000ULL},
  {"UefiMemUseThreshold", 0x77ULL},
  {"USBHS1_Config", 0x0ULL},
  {"UsbFnIoRevNum", 0x10001ULL},
  {"PwrBtnShutdownFlag", 0x0ULL},
  {"Sdc1GpioConfigOn", 0x1e92ULL},
  {"Sdc2GpioConfigOn", 0x1e92ULL},
  {"Sdc1GpioConfigOff", 0xa00ULL},
  {"Sdc2GpioConfigOff", 0xa00ULL},
  {"EnableSDHCSwitch", 0x1ULL},
  {"EnableUfsIOC", 0x0ULL},
  {"UfsSmmuConfigForOtherBootDev", 0x1ULL},
  {"SecurityFlag", 0xc4ULL},
  {"EnableLogFsSyncInRetail", 0x1ULL},
  {"ShmBridgememSize", 0xa00000ULL},
  {"EnableMultiThreading", 0x1ULL},
  {"MaxCoreCnt", 0x8ULL},
  {"EarlyInitCoreCnt", 0x1ULL},
  {"EnableUefiSecAppDebugLogDump", 0x0ULL},
  {"AllowNonPersistentVarsInRetail", 0x1ULL},
  {"EnableDisplayThread", 0x1ULL},
  {"EnableDisplayImageFv", 0x0ULL},
};

VOID GetConfigurationMap (EFI_CONFIGURATION_ENTRY_DESCRIPTOR **Map, UINT8 *Count) {
  *Map = mConfig;
  *Count = ARRAY_SIZE (mConfig);
}
