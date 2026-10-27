// SPDX-License-Identifier: BSD-2-Clause-Patent
// Experimental bounded output window, never production GOP admission.
#ifndef ENUMA_MENU_WINDOW_H
#define ENUMA_MENU_WINDOW_H
#include "PixelTransaction.h"
#define ENUMA_MENU_WIDTH 800u
#define ENUMA_MENU_HEIGHT 600u
#define ENUMA_MENU_X 64u
#define ENUMA_MENU_Y 96u
#define ENUMA_MENU_PIXELS (ENUMA_MENU_WIDTH*ENUMA_MENU_HEIGHT)
#define ENUMA_MENU_MAX_WRITES (12u*ENUMA_MENU_PIXELS)
#define ENUMA_MENU_MAX_CALLS 8192u
typedef struct {
  ENUMA_INHERITED_FRAME Frame;
  ENUMA_U32 *Saved;
  ENUMA_PIXEL_READ Read;
  ENUMA_PIXEL_WRITE Write;
  ENUMA_PIXEL_BARRIER Barrier;
  void *Context;
  ENUMA_U32 Ready,Attempted,Fault,Calls,Reads,Writes,RestoreWrites;
} ENUMA_MENU_WINDOW;
static inline ENUMA_U64 EnumaMenuAddress(const ENUMA_MENU_WINDOW *C,ENUMA_U32 X,ENUMA_U32 Y) {
  return C->Frame.Base+(ENUMA_U64)(Y+ENUMA_MENU_Y)*C->Frame.Stride+(X+ENUMA_MENU_X)*4;
}
static inline ENUMA_U32 EnumaMenuPack(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Bgr) {
  return F->ReservedMask+((Bgr&255)*(F->BlueMask&(~F->BlueMask+1)))+
    (((Bgr>>8)&255)*(F->GreenMask&(~F->GreenMask+1)))+
    (((Bgr>>16)&255)*(F->RedMask&(~F->RedMask+1)));
}
static inline int EnumaMenuSave(ENUMA_MENU_WINDOW *C,const ENUMA_INHERITED_FRAME *F,ENUMA_U32 *Saved,
  ENUMA_PIXEL_READ Read,ENUMA_PIXEL_WRITE Write,ENUMA_PIXEL_BARRIER Barrier,
  ENUMA_PIXEL_REVALIDATE Validate,void *Context) {
  ENUMA_U32 I;
  if(!C||!Saved||!Read||!Write||!Barrier||!Validate||!EnumaPixelFrameValid(F)) {return 0;}
  // A context is single-use. The caller initializes it to zero.
  if(C->Ready||C->Attempted||C->Calls||C->Reads||C->Writes) {return 0;}
  C->Frame=*F;C->Saved=Saved;C->Read=Read;C->Write=Write;C->Barrier=Barrier;C->Context=Context;
  if(Barrier(Context)!=1) {return 0;}
  for(I=0;I<ENUMA_MENU_PIXELS;I++) {
    C->Reads++;
    if(Read(Context,EnumaMenuAddress(C,I%ENUMA_MENU_WIDTH,I/ENUMA_MENU_WIDTH),&Saved[I])!=1) {return 0;}
  }
  if(Validate(Context,F)!=1) {return 0;}
  C->Ready=1;return 1;
}
// Source is a separate RAM BGRX surface, never a physical display pointer.
// Every attempted store is counted; first fault freezes further presents.
static inline int EnumaMenuPresent(ENUMA_MENU_WINDOW *C,const ENUMA_U32 *Source,
  ENUMA_U32 X,ENUMA_U32 Y,ENUMA_U32 Width,ENUMA_U32 Height) {
  ENUMA_U32 Row,Col,V;int Ok=1;
  if(!C||!C->Ready||C->Fault||!Source) {return 0;}
  if(C->Calls>=ENUMA_MENU_MAX_CALLS||X>=ENUMA_MENU_WIDTH||Y>=ENUMA_MENU_HEIGHT||
     !Width||!Height||Width>ENUMA_MENU_WIDTH-X||Height>ENUMA_MENU_HEIGHT-Y||
     Width*Height>ENUMA_MENU_MAX_WRITES-C->Writes) {C->Fault=1;return 0;}
  C->Calls++;
  for(Row=Y;Row<Y+Height&&Ok;Row++) {for(Col=X;Col<X+Width;Col++) {
    C->Attempted=1;C->Writes++;
    if(C->Write(C->Context,EnumaMenuAddress(C,Col,Row),
       EnumaMenuPack(&C->Frame,Source[Row*ENUMA_MENU_WIDTH+Col]))!=1) {Ok=0;break;}
  }}
  if(C->Barrier(C->Context)!=1) {Ok=0;}
  if(Ok) {for(Row=Y;Row<Y+Height&&Ok;Row++) {for(Col=X;Col<X+Width;Col++) {
    C->Reads++;
    if(C->Read(C->Context,EnumaMenuAddress(C,Col,Row),&V)!=1||
       V!=EnumaMenuPack(&C->Frame,Source[Row*ENUMA_MENU_WIDTH+Col])) {Ok=0;break;}
  }}}
  if(!Ok) {C->Fault=1;}return Ok;
}
// Restoration attempts every original word, including after a partial present.
// Returns4 on any failed restore/write/readback,5 on restored success,2 on a
// restored present fault,1 if no pixel store was attempted.
static inline ENUMA_U32 EnumaMenuRestore(ENUMA_MENU_WINDOW *C) {
  ENUMA_U32 I,V;int Ok=1;
  if(!C||!C->Ready||!C->Attempted) {return 1;}
  C->Ready=0;
  for(I=0;I<ENUMA_MENU_PIXELS;I++) {
    C->RestoreWrites++;
    if(C->Write(C->Context,EnumaMenuAddress(C,I%ENUMA_MENU_WIDTH,I/ENUMA_MENU_WIDTH),C->Saved[I])!=1) {Ok=0;}
  }
  if(C->Barrier(C->Context)!=1) {Ok=0;}
  for(I=0;I<ENUMA_MENU_PIXELS;I++) {
    C->Reads++;
    if(C->Read(C->Context,EnumaMenuAddress(C,I%ENUMA_MENU_WIDTH,I/ENUMA_MENU_WIDTH),&V)!=1||V!=C->Saved[I]) {Ok=0;}
  }
  return !Ok?4:C->Fault?2:5;
}
#endif
