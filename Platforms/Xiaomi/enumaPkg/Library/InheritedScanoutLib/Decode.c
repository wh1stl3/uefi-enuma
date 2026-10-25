// SPDX-License-Identifier: BSD-2-Clause-Patent
// Derived from ht94911/uefi-cas 893f5025, EnumaInheritedLib/DisplayDecode.c.
// Parameterized board limits and explicit video/command policy; snapshot-only.
#include "InheritedScanout.h"

static ENUMA_U32 Staged (const ENUMA_U32 *Layers, ENUMA_U32 Pipe, ENUMA_U32 Virtual) {
  static const ENUMA_U32 Starts[6] = {0, 3, 6, 26, 18, 21};
  if (Virtual) {
    if (Pipe < 4) { return (Layers[3] >> (Pipe * 4)) & 15; }
    return (Layers[2] >> (8 + (Pipe - 4) * 4)) & 15;
  }
  if (Pipe < 6) {
    ENUMA_U32 Extra = Pipe < 4 ? Pipe * 2 : 16 + (Pipe - 4) * 2;
    return ((Layers[0] >> Starts[Pipe]) & 7) | (((Layers[1] >> Extra) & 1) << 3);
  }
  return (Layers[2] >> ((Pipe - 6) * 4)) & 15;
}

static ENUMA_U32 RowCovered (const ENUMA_INHERITED_SNAPSHOT *S, ENUMA_U32 Mask, ENUMA_U32 Y, const ENUMA_INHERITED_LIMITS *L) {
  ENUMA_U32 X = 0, Next, Pass, P, Px, Py, Pw, Ph;
  for (Pass = 0; Pass < ENUMA_PIPE_COUNT; ++Pass) {
    Next = X;
    for (P = 0; P < ENUMA_PIPE_COUNT; ++P) {
      if (!(Mask & (1u << P))) { continue; }
      Px = (S->Pipes[P].OutXy & 0xffff) + L->OriginX[P];
      Py = (S->Pipes[P].OutXy >> 16) + L->OriginY[P];
      Pw = S->Pipes[P].OutSize & 0xffff; Ph = S->Pipes[P].OutSize >> 16;
      if (Py <= Y && Y < Py + Ph && Px <= X && Px + Pw > Next) { Next = Px + Pw; }
    }
    if (Next == L->Width) { return 1; }
    if (Next == X) { return 0; }
    X = Next;
  }
  return 0;
}

static ENUMA_U32 Decode (const ENUMA_INHERITED_SNAPSHOT *S, const ENUMA_INHERITED_LIMITS *L, ENUMA_INHERITED_FRAME *R) {
  ENUMA_U32 C, M, P, I, Mask = 0, CtlMask = 0, Manual = 0, Intfs = 0;
  ENUMA_U32 Stride = 0, Pattern = 0, First = 1;
  ENUMA_U64 Base = 0, Bytes = 0;
  unsigned char *Clear = (void *)R;
  if (R == 0) { return EnumaInheritedArguments; }
  for (I = 0; I < sizeof (*R); ++I) { Clear[I] = 0; }
  if (S == 0 || L == 0 || L->Width == 0 || L->Width > 65535 || L->Height == 0 || L->Height > 65535 || L->Mode > ENUMA_MODE_COMMAND || L->SplashBase == 0 || L->SplashBytes == 0 || L->SplashBase > ~0ULL - L->SplashBytes) { return EnumaInheritedArguments; }
  if ((S->Revision >> 16) != 0x6000) { return EnumaInheritedRevision; }
  for (C = 0; C < ENUMA_CTL_COUNT; ++C) {
    const ENUMA_INHERITED_CTL *Ctl = &S->Ctls[C];
    ENUMA_U32 Auto = 0, Selected = 0, Local = 0;
    if (!(Ctl->IntfActive & 6)) { continue; }
    // CTL_TOP bit17 follows the pinned SDE video/command selector. Never change it.
    if (Ctl->IntfActive & ~6u) { return EnumaInheritedRoute; }
    if (((Ctl->Top >> 17) & 1u) != L->Mode) { return EnumaInheritedMode; }
    for (I = 0; I < 2; ++I) {
      ENUMA_U32 A;
      if (!(Ctl->IntfActive & (2u << I))) { continue; }
      A = S->IntfAutoRefresh[I]; ++Selected;
      if (L->Mode == ENUMA_MODE_COMMAND && (A & (1u << 31))) {
        if (!(A & 0xffff)) { return EnumaInheritedRefreshMode; }
        ++Auto;
      }
    }
    if (L->Mode == ENUMA_MODE_COMMAND && Auto != 0 && Auto != Selected) { return EnumaInheritedRefreshMode; }
    for (M = 0; M < ENUMA_MIXER_COUNT; ++M) {
      for (P = 0; P < ENUMA_PIPE_COUNT; ++P) {
        if (Staged (Ctl->Layers[M], P, 0) > 12) { return EnumaInheritedRoute; }
        if (Staged (Ctl->Layers[M], P, 1)) { return EnumaInheritedVirtualRect; }
        if (Staged (Ctl->Layers[M], P, 0)) { Local |= 1u << P; }
      }
      // cas has no RGB/cursor pipes; stale unsupported staging is not a match.
      if ((Ctl->Layers[M][0] & ((0x1ffu << 9) | (7u << 29))) ||
          (Ctl->Layers[M][1] & ((0xffu << 8) | (15u << 20) | (15u << 26)))) {
        return EnumaInheritedRoute;
      }
    }
    if (!Local) { return EnumaInheritedRoute; }
    CtlMask |= 1u << C; Mask |= Local; Intfs |= Ctl->IntfActive;
    if (L->Mode == ENUMA_MODE_COMMAND && !Auto) { Manual |= 1u << C; }
  }
  if (!Mask) { return EnumaInheritedRoute; }
  if ((L->OriginMask & Mask) != Mask) { return EnumaInheritedTopology; }
  for (P = 0; P < ENUMA_PIPE_COUNT; ++P) {
    const ENUMA_INHERITED_PIPE *Pipe = &S->Pipes[P];
    ENUMA_U32 W, H, X, Y, Sx, Sy, Pitch, N, Seen = 0;
    ENUMA_U64 Input, Output, Candidate, Extent;
    if (!(Mask & (1u << P))) { continue; }
    W = Pipe->OutSize & 0xffff; H = Pipe->OutSize >> 16;
    if (L->OriginX[P] > L->Width || L->OriginY[P] > L->Height) { return EnumaInheritedTopology; }
    X = (Pipe->OutXy & 0xffff) + L->OriginX[P];
    Y = (Pipe->OutXy >> 16) + L->OriginY[P];
    Sx = Pipe->Xy & 0xffff; Sy = Pipe->Xy >> 16;
    Pitch = Pipe->Stride & 0xffff;
    if (W == 0 || H == 0 || Pipe->Size != Pipe->OutSize ||
        X + W > L->Width || Y + H > L->Height ||
        Pitch < L->Width * 4 || (Pitch & 3) || (Pipe->Stride >> 16) ||
        (ENUMA_U64)(Sx + W) * 4 > Pitch) { return EnumaInheritedGeometry; }
    if (Pipe->Secure & 15) { return EnumaInheritedSecure; }
    if (Pipe->Multirect & 7) { return EnumaInheritedVirtualRect; }
    // Linear, tightly packed, interleaved 8:8:8:8; no scaling/rotation/YUV/UBWC.
    // Source alpha enable is permitted. Its byte is written opaque by the GOP.
    if ((Pipe->Format & ~(1u << 8)) != 0x000236ff ||
        (Pipe->OpMode & ((1u << 0) | (1u << 13) | (1u << 14) | (3u << 22)))) {
      return EnumaInheritedFormat;
    }
    for (N = 0; N < 4; ++N) {
      ENUMA_U32 Component = (Pipe->Unpack >> (N * 8)) & 255;
      if (Component > 3 || (Seen & (1u << Component))) { return EnumaInheritedFormat; }
      Seen |= 1u << Component;
    }
    Input = (ENUMA_U64)Sy * Pitch + (ENUMA_U64)Sx * 4;
    Output = (ENUMA_U64)Y * Pitch + (ENUMA_U64)X * 4;
    if ((Pipe->Address & 3) || (ENUMA_U64)Pipe->Address + Input < Output) { return EnumaInheritedBounds; }
    Candidate = (ENUMA_U64)Pipe->Address + Input - Output;
    Extent = (ENUMA_U64)Pitch * L->Height;
    if (Candidate < L->SplashBase || Candidate >= (ENUMA_U64)L->SplashBase + L->SplashBytes ||
        Extent > (ENUMA_U64)L->SplashBase + L->SplashBytes - Candidate) { return EnumaInheritedBounds; }
    if (First) { Base = Candidate; Bytes = Extent; Stride = Pitch; Pattern = Pipe->Unpack; First = 0; }
    else if (Base != Candidate || Stride != Pitch || Pattern != Pipe->Unpack) { return EnumaInheritedInconsistent; }
  }
  // Rectangles must cover the complete panel. Check every horizontal boundary;
  // coverage is constant between consecutive rectangle boundaries.
  if (!RowCovered (S, Mask, 0, L) || !RowCovered (S, Mask, L->Height - 1, L)) { return EnumaInheritedCoverage; }
  for (P = 0; P < ENUMA_PIPE_COUNT; ++P) {
    ENUMA_U32 Y, End;
    if (!(Mask & (1u << P))) { continue; }
    Y = (S->Pipes[P].OutXy >> 16) + L->OriginY[P]; End = Y + (S->Pipes[P].OutSize >> 16);
    if (!RowCovered (S, Mask, Y, L) || (End < L->Height && !RowCovered (S, Mask, End, L))) { return EnumaInheritedCoverage; }
  }
  R->Base = Base; R->Bytes = Bytes; R->Width = L->Width; R->Height = L->Height; R->Stride = Stride;
  for (I = 0; I < 4; ++I) {
    switch ((Pattern >> (I * 8)) & 255) {
      case 0: R->GreenShift = I * 8; R->GreenMask = 255u << (I * 8); break;
      case 1: R->BlueShift = I * 8; R->BlueMask = 255u << (I * 8); break;
      case 2: R->RedShift = I * 8; R->RedMask = 255u << (I * 8); break;
      case 3: R->ReservedMask = 255u << (I * 8); break;
    }
  }
  R->PipeMask = Mask; R->CtlMask = CtlMask; R->ManualCtlMask = Manual; R->IntfMask = Intfs;
  R->Requirements = ENUMA_REQUIRE_ALL;
  return EnumaInheritedOk;
}

ENUMA_U32 EnumaDecodeInherited(const ENUMA_INHERITED_SNAPSHOT *S,const ENUMA_INHERITED_LIMITS *L,ENUMA_INHERITED_FRAME *R) {
  ENUMA_INHERITED_FRAME Candidate = {0}; ENUMA_U32 I,Status;
  if (!R) { return EnumaInheritedArguments; }
  for (I=0; I<sizeof(*R); I++) { ((unsigned char *)R)[I]=0; }
  Status=Decode(S,L,&Candidate);
  if (Status==EnumaInheritedOk) { *R=Candidate; }
  return Status;
}
