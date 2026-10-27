// SPDX-License-Identifier: BSD-2-Clause-Patent
// Included by InheritedGop.c after validated GOP rectangles/buffer arithmetic.
STATIC UINT32 ByteShift(UINT32 Mask) {UINT32 S=0;while(Mask>255){Mask>>=8;S+=8;}return S;}
STATIC EFI_STATUS AccessFault(ENUMA_GOP *G,EFI_STATUS S) {
  if(S==EFI_SUCCESS){return S;}
  if(!G->AccessState.Latched){G->AccessState.Fault=EFI_ERROR(S)?S:EFI_DEVICE_ERROR;G->AccessState.Latched=TRUE;}
  return G->AccessState.Fault;
}
STATIC EFI_STATUS AccessSync(ENUMA_GOP *G) {
  G->AccessState.Barriers++;return AccessFault(G,G->Access.Synchronize(G->Access.Context));
}
STATIC UINT64 PixelAddress(ENUMA_GOP *G,UINTN X,UINTN Y) {return G->Frame.Base+(UINT64)Y*G->Frame.Stride+X*4;}
STATIC EFI_STATUS OpaqueBlt(ENUMA_GOP *G,EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer,EFI_GRAPHICS_OUTPUT_BLT_OPERATION Op,
  UINTN Sx,UINTN Sy,UINTN Dx,UINTN Dy,UINTN W,UINTN H,UINTN Delta) {
  UINTN X,Y,Row,Col;UINT32 V=0,Rs=ByteShift(G->Frame.RedMask),Gs=ByteShift(G->Frame.GreenMask),Bs=ByteShift(G->Frame.BlueMask);
  EFI_STATUS S;EFI_GRAPHICS_OUTPUT_BLT_PIXEL *P;
  BOOLEAN ReverseY=Op==EfiBltVideoToVideo&&Dy>Sy&&Dy<Sy+H;
  BOOLEAN ReverseX=Op==EfiBltVideoToVideo&&Dy==Sy&&Dx>Sx&&Dx<Sx+W;
  S=AccessSync(G);if(S!=EFI_SUCCESS){return S;}
  for(Row=0;Row<H;Row++) {
    Y=ReverseY?H-1-Row:Row;
    for(Col=0;Col<W;Col++) {
      X=ReverseX?W-1-Col:Col;
      if(Op==EfiBltVideoFill||Op==EfiBltBufferToVideo) {
        P=Op==EfiBltVideoFill?Buffer:(VOID *)((UINT8 *)Buffer+(Sy+Y)*Delta+(Sx+X)*4);
        V=((UINT32)P->Red<<Rs)|((UINT32)P->Green<<Gs)|((UINT32)P->Blue<<Bs)|G->Frame.ReservedMask;
      } else {
        G->AccessState.Reads++;
        S=AccessFault(G,G->Access.Read32(G->Access.Context,PixelAddress(G,Sx+X,Sy+Y),&V));
        if(S!=EFI_SUCCESS){goto Done;}
        V|=G->Frame.ReservedMask;
      }
      if(Op==EfiBltVideoToBltBuffer) {
        P=(VOID *)((UINT8 *)Buffer+(Dy+Y)*Delta+(Dx+X)*4);
        P->Red=(UINT8)(V>>Rs);P->Green=(UINT8)(V>>Gs);P->Blue=(UINT8)(V>>Bs);P->Reserved=0;
      } else {
        G->AccessState.Writes++;
        S=AccessFault(G,G->Access.Write32(G->Access.Context,PixelAddress(G,Dx+X,Dy+Y),V));
        if(S!=EFI_SUCCESS){goto Done;}
      }
    }
  }
Done:
  // Order any partial stores even after an I/O failure; retain the first error.
  S=AccessSync(G);return G->AccessState.Latched?G->AccessState.Fault:S;
}
