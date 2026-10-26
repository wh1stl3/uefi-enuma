// SPDX-License-Identifier: BSD-2-Clause-Patent
// Fixed four-letter diagnostic bitmap, not a general console or font library.
#ifndef ENUMA_PIXEL_TEXT_H
#define ENUMA_PIXEL_TEXT_H
static inline ENUMA_U32 EnumaPixelTextAt(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 N) {
  static const unsigned char Glyph[4][7]={
    {17,17,17,17,17,17,14}, // U
    {31,16,16,30,16,16,31}, // E
    {31,16,16,30,16,16,16}, // F
    {31,4,4,4,4,4,31}      // I
  };
  ENUMA_U32 X=N%128,Y=N/128,C,Column;
  if(X<4||X>=100||Y<2||Y>=30) { return F->ReservedMask; }
  C=(X-4)/24;Column=((X-4)%24)/4;
  if(Column<5&&(Glyph[C][(Y-2)/4]&(1u<<(4-Column)))) { return 0xffffffffu; }
  return F->ReservedMask;
}
#endif
