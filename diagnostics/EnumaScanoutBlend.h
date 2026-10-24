// SPDX-License-Identifier: BSD-2-Clause-Patent
// Conservative single-staged-source/no-dim configuration, not visibility proof.
#ifndef ENUMA_SCANOUT_BLEND_H
#define ENUMA_SCANOUT_BLEND_H
#include "EnumaScanoutGeometry.h"
// Return the encoded stage (native stage+1), or zero for unsupported/multiple state.
STATIC inline UINTN
EnumaBlendSingleStage(UINTN Pipe, CONST UINT32 Layers[4])
{
  CONST UINT8 Ext[8] = {0,0,0,0,0,0,2,2};
  CONST UINT8 Shift[8] = {0,3,6,26,18,21,0,4};
  CONST UINT32 Secondary[8] = {1,4,16,64,65536,262144,0,0};
  UINT32 Expected[4] = {0,0,0,0}, Value, Mask;
  UINTN J;
  if (Pipe >= 8 || Layers == NULL) { return 0; }
  Mask = Pipe < 6 ? 7U : 15U;
  Value = (Layers[Ext[Pipe]] >> Shift[Pipe]) & Mask;
  Expected[Ext[Pipe]] = Value << Shift[Pipe];
  if ((Layers[1] & Secondary[Pipe]) != 0) { Value |= 8; Expected[1] = Secondary[Pipe]; }
  if (Value == 0 || Value > 12) { return 0; }
  for (J = 0; J < 4; J++) { if (Layers[J] != Expected[J]) { return 0; } }
  return Value;
}
// Step21 retains step20. <=289 reads, two existing 100000us waits; no writes.
STATIC inline EFI_STATUS
EnumaScanoutSingleBlend (ENUMA_SCANOUT_READ32 Read, ENUMA_DSI_WAIT Wait)
{
  CONST UINT32 Pipes[8] = {0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  ENUMA_SCANOUT_DESCRIPTOR D, Check;
  UINT32 OutSize, OutXY, MixerSize;
  UINTN Mixer, BlendStage;
  UINT32 BlendOps[11], MixerMode, Alpha = 0;
  UINT32 Version, Active[2], Layers[4], Raw[6], Intfs, Previous, Timing, Enable;
  UINTN Pipe, Ctl, Lm, J, SelectedCtl = 0, SelectedLm = 0, Intf = 0, Window;
  EFI_STATUS Status;
  BOOLEAN Found = FALSE;
  if (Read == NULL || Wait == NULL) { return EFI_INVALID_PARAMETER; }
  Version = Read(0x0ae00000U);
  if ((Version >> 16) != 0x6000) { return EFI_UNSUPPORTED; }
  Status = EnumaScanoutFindBoundedDescriptor(Read, &D);
  if (EFI_ERROR(Status)) { return Status; }
  for (Pipe = 0; Pipe < 8; Pipe++) { if (D.Source == 0x0ae00000U + Pipes[Pipe]) { break; } }
  if (Pipe == 8) { return EFI_UNSUPPORTED; }
  Active[0] = Read(0x0ae6ba6cU) & 1U; Active[1] = Read(0x0ae6c26cU) & 1U;
  if ((Active[0] | Active[1]) == 0) { return EFI_NOT_READY; }
  Intfs = 0;
  for (Ctl = 0; Ctl < 6 && !Found; Ctl++) {
    UINTN Base = 0x0ae02000U + Ctl * 0x200;
    UINT32 Bits = Read(Base + 0xf4);
    if (((Bits & 2U) == 0 || Active[0] == 0) && ((Bits & 4U) == 0 || Active[1] == 0)) { continue; }
    for (Lm = 0; Lm < 6; Lm++) {
      for (J = 0; J < 4; J++) { Layers[J] = Read(Base + EnumaRouteLayerOffset(Lm, J)); }
      if (EnumaRouteStaged(Pipe, Layers)) {
        SelectedCtl = Base; SelectedLm = Lm; Intfs = Bits;
        Intf = (Bits & 2U) != 0 && Active[0] != 0 ? 0x0ae6b800U : 0x0ae6c000U;
        Found = TRUE; break;
      }
    }
  }
  if (!Found) { return EFI_NOT_READY; }
  EnumaRouteReadDescriptor(Read, D.Source, Raw);
  Status = EnumaScanoutDecodeDescriptor(Raw[0], Raw[1], Raw[2], Raw[3], Raw[4], Raw[5], &Check);
  if (EFI_ERROR(Status) || Check.Base != D.Base || Check.Start != D.Start || Check.End != D.End ||
      Check.Width != D.Width || Check.Height != D.Height || Check.Pitch != D.Pitch || Check.X != D.X || Check.Y != D.Y) { return EFI_NOT_READY; }
  // Saved enuma FDT: six mixers at 0x45000..0x4a000, length 0x320.
  Mixer = 0x0ae45000U + SelectedLm * 0x1000;
  OutSize = Read(D.Source + 0x0c); OutXY = Read(D.Source + 0x10);
  MixerSize = Read(Mixer + 4);
  if (!EnumaGeometryContained(D.Width, D.Height, OutSize, OutXY, MixerSize)) { return EFI_NOT_READY; }
  BlendStage = EnumaBlendSingleStage(Pipe, Layers);
  if (BlendStage == 0) { return EFI_NOT_READY; }
  MixerMode = Read(Mixer);
  for (J = 0; J < 11; J++) {
    BlendOps[J] = Read(Mixer + 0x20 + J * 0x18);
    if ((BlendOps[J] & (1U << 16)) != 0) { return EFI_NOT_READY; }
  }
  // Encoded 1 is BASE: native driver has no per-stage blend registers.
  if (BlendStage > 1) {
    Alpha = Read(Mixer + 0x24 + (BlendStage - 2) * 0x18);
    if (BlendOps[BlendStage - 2] != 0x100 || Alpha != 0x00ff0000) { return EFI_NOT_READY; }
  }
  Previous = Read(Intf + 0xac); Timing = Read(Intf); Enable = Read(Intf + 0xa8);
  for (Window = 0; Window < 2; Window++) {
    UINT32 Frame, NowActive, NowTiming, NowEnable;
    Status = Wait(ENUMA_DSI_SAMPLE_US);
    if (Status != EFI_SUCCESS) { return EFI_ERROR(Status) ? Status : EFI_NOT_READY; }
    Frame = Read(Intf + 0xac); NowActive = Read(Intf + 0x26c) & 1U;
    NowTiming = Read(Intf); NowEnable = Read(Intf + 0xa8);
    if (Frame <= Previous || Frame - Previous > 64 || NowActive == 0 || NowTiming != Timing || NowEnable != Enable ||
        Read(SelectedCtl + 0xf4) != Intfs) { return EFI_NOT_READY; }
    for (J = 0; J < 4; J++) {
      if (Read(SelectedCtl + EnumaRouteLayerOffset(SelectedLm, J)) != Layers[J]) { return EFI_NOT_READY; }
    }
    for (J = 0; J < 6; J++) {
      CONST UINTN Offsets[6] = {0x14,0,0x24,0x08,0x30,0x38};
      if (Read(D.Source + Offsets[J]) != Raw[J]) { return EFI_NOT_READY; }
    }
    if (Read(D.Source + 0x0c) != OutSize || Read(D.Source + 0x10) != OutXY ||
        Read(Mixer + 4) != MixerSize) { return EFI_NOT_READY; }
    if (Read(Mixer) != MixerMode) { return EFI_NOT_READY; }
    for (J = 0; J < 11; J++) {
      if (Read(Mixer + 0x20 + J * 0x18) != BlendOps[J]) { return EFI_NOT_READY; }
    }
    if (BlendStage > 1 && Read(Mixer + 0x24 + (BlendStage - 2) * 0x18) != Alpha) { return EFI_NOT_READY; }
    Previous = Frame;
  }
  return Read(0x0ae00000U) == Version ? EFI_SUCCESS : EFI_NOT_READY;
}
#endif
