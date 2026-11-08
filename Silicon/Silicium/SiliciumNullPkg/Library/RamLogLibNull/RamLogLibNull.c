#include <Uefi.h>

#include <Library/RamLogLib.h>

//
// No-op instance: platforms without a persistent log channel link this so the shared Sec and FrameBufferSerialPortLib
// still resolve RamLogLib. Interface identical to xVanTuring/uefi-pipa's RamLogLibNull.
//
VOID
EFIAPI
RamLogReset (VOID)
{
}

VOID
EFIAPI
RamLogWrite (
  IN CONST UINT8 *Buffer,
  IN UINTN        NumberOfBytes)
{
}
