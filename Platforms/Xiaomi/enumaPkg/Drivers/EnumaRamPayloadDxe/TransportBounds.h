// SPDX-License-Identifier: BSD-2-Clause-Patent
// Pure bounds helpers shared by firmware and sanitizer tests. No dereferences.
#ifndef ENUMA_TRANSPORT_BOUNDS_H
#define ENUMA_TRANSPORT_BOUNDS_H
STATIC BOOLEAN Contains (UINT64 Base, UINT64 Bytes, UINT64 Start, UINT64 Size) {
  return Bytes != 0 && Bytes <= MAX_UINT64 - Base && Size != 0 &&
    Start >= Base && Start - Base < Bytes && Size <= Bytes - (Start - Base);
}
STATIC BOOLEAN TransportSpan (UINT64 Base, UINT64 Bytes, UINT64 Dtb, UINT64 TreeSize,
  UINT64 Start, UINT64 End, UINT64 Prefix, UINT64 FatSize) {
  return Contains (Base, Bytes, Dtb, TreeSize) && End > Start &&
    Contains (Base, Bytes, Start, End - Start) &&
    Prefix <= MAX_UINT64 - FatSize && End - Start == Prefix + FatSize &&
    FatSize >= 512 && FatSize % 512 == 0 &&
    !(Start < Dtb + TreeSize && Dtb < End);
}
#endif
