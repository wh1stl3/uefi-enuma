// SPDX-License-Identifier: BSD-2-Clause-Patent
// Opt-in bounded experiment, never GOP admission. All pixel I/O is caller-owned.
#ifndef ENUMA_PIXEL_TRANSACTION_H
#define ENUMA_PIXEL_TRANSACTION_H
#include "Evidence.h"
#ifndef ENUMA_NATIVE_ROUTE_CPU_TEXT
#define ENUMA_NATIVE_ROUTE_CPU_TEXT 0
#endif
#ifndef ENUMA_BOOT_LOG_REPLAY
#define ENUMA_BOOT_LOG_REPLAY 0
#endif
#define ENUMA_PIXEL_WIDTH (ENUMA_BOOT_LOG_REPLAY>=2?320:(ENUMA_BOOT_LOG_REPLAY?256:128))
#define ENUMA_PIXEL_ROWS (ENUMA_BOOT_LOG_REPLAY>=2?160:(ENUMA_BOOT_LOG_REPLAY?96:(ENUMA_NATIVE_ROUTE_CPU_TEXT?32:8)))
#define ENUMA_PIXEL_ORIGIN_X (ENUMA_BOOT_LOG_REPLAY>=2?64:0)
#define ENUMA_PIXEL_ORIGIN_Y (ENUMA_BOOT_LOG_REPLAY>=2?96:0)
#define ENUMA_PIXEL_WORDS (ENUMA_PIXEL_WIDTH*ENUMA_PIXEL_ROWS)
#ifndef ENUMA_BOOT_LOG_HOLD_US
#define ENUMA_BOOT_LOG_HOLD_US 2000000
#endif
#if ENUMA_BOOT_LOG_HOLD_US != 2000000 && ENUMA_BOOT_LOG_HOLD_US != 8000000 && !(ENUMA_OEM_CONTEXT && ENUMA_BOOT_LOG_HOLD_US == 4000000)
#error Unsupported cached log hold duration
#endif
#define ENUMA_PIXEL_HOLD_US (ENUMA_BOOT_LOG_REPLAY?ENUMA_BOOT_LOG_HOLD_US:500000)
#if ENUMA_BOOT_LOG_REPLAY == 3
#include "PixelDiagnosticLogPage.h"
#elif ENUMA_BOOT_LOG_REPLAY == 2
#include "PixelAttributedLogPage.h"
#elif ENUMA_BOOT_LOG_REPLAY
#include "PixelLogPage.h"
#endif
#if ENUMA_NATIVE_ROUTE_CPU_TEXT
#include "PixelText.h"
#endif
static inline ENUMA_U32 EnumaPixelPatternAt(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 N,ENUMA_U32 Color) {
#if ENUMA_BOOT_LOG_REPLAY
  (void)Color;return EnumaPixelLogAt(F,N);
#elif ENUMA_NATIVE_ROUTE_CPU_TEXT
  (void)Color;return EnumaPixelTextAt(F,N);
#else
  (void)F;(void)N;return Color;
#endif
}
typedef int (*ENUMA_PIXEL_READ)(void *,ENUMA_U64,ENUMA_U32 *);
typedef int (*ENUMA_PIXEL_WRITE)(void *,ENUMA_U64,ENUMA_U32);
typedef int (*ENUMA_PIXEL_REVALIDATE)(void *,const ENUMA_INHERITED_FRAME *);
typedef int (*ENUMA_PIXEL_BARRIER)(void *);
typedef int (*ENUMA_PIXEL_HOLD)(void *,ENUMA_U32);
static inline int EnumaPixelBytesEqual(const void *A,const void *B,ENUMA_U32 N) {
  const unsigned char *X=A,*Y=B;ENUMA_U32 I;
  for(I=0;I<N;I++) { if(X[I]!=Y[I]) { return 0; } } return 1;
}
// Each evidence object has already passed the complete Native6 observation.
// Compare raw configuration, not advancing frame counters or status side bits.
static inline int EnumaPixelEvidenceSame(const ENUMA_VIDEO_EVIDENCE *A,const ENUMA_VIDEO_EVIDENCE *B) {
  ENUMA_U32 I;
  if(!A||!B||!EnumaPixelBytesEqual(&A->Frame,&B->Frame,sizeof(A->Frame))||
     !EnumaPixelBytesEqual(&A->Limits,&B->Limits,sizeof(A->Limits))||
     !EnumaPixelBytesEqual(&A->Samples[2].Scanout,&B->Samples[0].Scanout,sizeof(A->Samples[2].Scanout))||
     !EnumaPixelBytesEqual(&A->Samples[2].Mixers,&B->Samples[0].Mixers,sizeof(A->Samples[2].Mixers))||
     !EnumaPixelBytesEqual(&A->Samples[2].Scope,&B->Samples[0].Scope,sizeof(A->Samples[2].Scope))) { return 0; }
  for(I=0;I<2;I++) {
    if(A->Samples[2].Interfaces[I].Timing!=B->Samples[0].Interfaces[I].Timing||
       A->Samples[2].Interfaces[I].CounterEnable!=B->Samples[0].Interfaces[I].CounterEnable) { return 0; }
  }
  return 1;
}
static inline int EnumaPixelFrameValid(const ENUMA_INHERITED_FRAME *F) {
  ENUMA_U32 M[4],I,U=0;
  if(!F||F->Requirements!=22||F->Width!=1600||F->Height!=2560||F->Stride<6400||
     (F->Stride&3)||F->Bytes!=(ENUMA_U64)F->Stride*F->Height||(F->Base&3)||
     F->Base<0x9c000000ULL||F->Base>=0x9e400000ULL||!F->Bytes||F->Bytes>0x9e400000ULL-F->Base) { return 0; }
  M[0]=F->RedMask;M[1]=F->GreenMask;M[2]=F->BlueMask;M[3]=F->ReservedMask;
  for(I=0;I<4;I++) {
    if((M[I]!=0xff&&M[I]!=0xff00&&M[I]!=0xff0000&&M[I]!=0xff000000)||(U&M[I])) { return 0; }
    U|=M[I];
  }
  return U==0xffffffffu;
}
static inline ENUMA_U64 EnumaPixelAddress(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 N) {
  return F->Base+(ENUMA_U64)(ENUMA_PIXEL_ORIGIN_Y+N/ENUMA_PIXEL_WIDTH)*F->Stride+(ENUMA_U64)(ENUMA_PIXEL_ORIGIN_X+N%ENUMA_PIXEL_WIDTH)*4;
}
// Returns1 no stores;2 pattern rejected/restored;3 hold rejected/restored;
// 4 restoration not established (hard stop);5 CPU transaction/restoration done.
// A timeout has no result. No code establishes visible output or GOP semantics.
static inline ENUMA_U32 EnumaPixelTransaction(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Policy,ENUMA_U64 El,
  ENUMA_PIXEL_READ Read,ENUMA_PIXEL_WRITE Write,ENUMA_PIXEL_REVALIDATE Validate,
  ENUMA_PIXEL_BARRIER Barrier,ENUMA_PIXEL_HOLD Hold,void *Ctx) {
  ENUMA_U32 Saved[ENUMA_PIXEL_WORDS],I,V,Color=0xffffffffu,AllWhite=1,PatternOk=1,HoldOk,RestoreOk=1;
#if ENUMA_BOOT_LOG_REPLAY
  if(!mEnumaLogPage.Ready) { return 1; }
#endif
  if(Policy!=3||El!=4||!EnumaPixelFrameValid(F)||!Read||!Write||!Validate||!Barrier||!Hold) { return 1; }
  if(Barrier(Ctx)!=1) { return 1; }
  for(I=0;I<ENUMA_PIXEL_WORDS;I++) {
    if(Read(Ctx,EnumaPixelAddress(F,I),&Saved[I])!=1) { return 1; }
    if(Saved[I]!=0xffffffffu) { AllWhite=0; }
  }
  if(Validate(Ctx,F)!=1) { return 1; }
  if(AllWhite) { Color=F->ReservedMask; } // opaque black; white otherwise
  for(I=0;I<ENUMA_PIXEL_WORDS;I++) {
    if(Write(Ctx,EnumaPixelAddress(F,I),EnumaPixelPatternAt(F,I,Color))!=1) { PatternOk=0;break; }
  }
  if(Barrier(Ctx)!=1) { PatternOk=0; }
  if(PatternOk) { for(I=0;I<ENUMA_PIXEL_WORDS;I++) {
    if(Read(Ctx,EnumaPixelAddress(F,I),&V)!=1||V!=EnumaPixelPatternAt(F,I,Color)) { PatternOk=0;break; }
  } }
  HoldOk=Hold(Ctx,ENUMA_PIXEL_HOLD_US)==1;
  // Every path after the first write attempt tries the complete saved region.
  // Do not stop restoration at a rejected callback or a mismatching word.
  for(I=0;I<ENUMA_PIXEL_WORDS;I++) {
    if(Write(Ctx,EnumaPixelAddress(F,I),Saved[I])!=1) { RestoreOk=0; }
  }
  if(Barrier(Ctx)!=1) { RestoreOk=0; }
  for(I=0;I<ENUMA_PIXEL_WORDS;I++) {
    if(Read(Ctx,EnumaPixelAddress(F,I),&V)!=1||V!=Saved[I]) { RestoreOk=0; }
  }
  if(!RestoreOk) { return 4; }
  if(!PatternOk) { return 2; }
  if(!HoldOk) { return 3; }
  return 5;
}
#endif
