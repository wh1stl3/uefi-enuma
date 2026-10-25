// SPDX-License-Identifier: BSD-2-Clause-Patent
// Translation queries only. No framebuffer data pointers or mapping changes.
#ifndef ENUMA_CPU_STAGE1_H
#define ENUMA_CPU_STAGE1_H
#include "InheritedScanout.h"
#define ENUMA_CPU_MAX_TRANSLATIONS 18432u
typedef int (*ENUMA_CPU_TRANSLATE)(void *,ENUMA_U64,ENUMA_U32,ENUMA_U64 *);
// Five terminal codes, with1 reserved for invalid context/contract/callback.
static ENUMA_U32 EnumaCpuStage1(const ENUMA_INHERITED_FRAME *F,ENUMA_CPU_TRANSLATE Translate,void *Ctx) {
  ENUMA_U64 Page,Last,Par; ENUMA_U32 Write,Count=0;
  if(!F||!Translate||F->Requirements!=22||F->Width!=1600||F->Height!=2560||
     F->Stride<6400||(F->Stride&3)||F->Bytes!=(ENUMA_U64)F->Stride*F->Height||
     (F->Base&3)||F->Base<0x9c000000ULL||F->Base>=0x9e400000ULL||
     !F->Bytes||F->Bytes>0x9e400000ULL-F->Base) { return 1; }
  Page=F->Base&~0xfffULL;Last=(F->Base+F->Bytes-1)&~0xfffULL;
  for(;;Page+=4096) {
    for(Write=0;Write<2;Write++) {
      if(Count>=ENUMA_CPU_MAX_TRANSLATIONS) { return 1; } Count++;
      Par=0;
      if(Translate(Ctx,Page,Write,&Par)!=1) { return 1; }
      if(Par&1) { return 2; }
      if((Par&0x000ffffffffff000ULL)!=Page) { return 3; }
      if((Par>>56)!=0xbb) { return 4; }
    }
    if(Page==Last) { break; }
  }
  return 5;
}
// Capture the last successful PAR without changing the original algorithm,
// stopping point, operation count or obligations. Code1 includes all-WT.
typedef struct { ENUMA_CPU_TRANSLATE Translate;void *Ctx;ENUMA_U64 Par; } ENUMA_CPU_ATTRIBUTE_CONTEXT;
static inline int EnumaCpuAttributeCapture(void *Opaque,ENUMA_U64 Page,ENUMA_U32 Write,ENUMA_U64 *Par) {
  ENUMA_CPU_ATTRIBUTE_CONTEXT *C=Opaque;int S=C->Translate(C->Ctx,Page,Write,Par);
  if(S==1) { C->Par=*Par; } return S;
}
static inline ENUMA_U32 EnumaCpuAttribute(const ENUMA_INHERITED_FRAME *F,ENUMA_CPU_TRANSLATE Translate,void *Ctx) {
  ENUMA_CPU_ATTRIBUTE_CONTEXT C={Translate,Ctx,0};ENUMA_U32 A;
  if(!Translate||EnumaCpuStage1(F,EnumaCpuAttributeCapture,&C)!=4) { return 1; }
  A=(ENUMA_U32)(C.Par>>56);
  if(A==0x44) { return 2; }
  if(A==0xff) { return 3; }
  if(A==0||A==4||A==8||A==12) { return 4; }
  return 5;
}
// Independent whole-range semantic-policy audit. Preserve all raw output
// address/fault bits; replace only ATTR in the callback's RAM result so the
// original bound/range/R-W/identity loop can run to completion. No PAR write.
static inline ENUMA_U32 EnumaCpuPolicyNibble(ENUMA_U32 N) {
  if(N==4) { return 2; } // NC
  if((N>=1&&N<=3)||(N>=8&&N<=11)) { return 3; } // WT, either transient hint
  if((N>=5&&N<=7)||N>=12) { return 4; } // WB, either transient hint
  return 5; // device/reserved: not a normal-policy admission
}
typedef struct { ENUMA_CPU_TRANSLATE Translate;void *Ctx;ENUMA_U32 Policy,Mixed; } ENUMA_CPU_POLICY_CONTEXT;
static inline int EnumaCpuPolicyCapture(void *Opaque,ENUMA_U64 Page,ENUMA_U32 Write,ENUMA_U64 *Par) {
  ENUMA_CPU_POLICY_CONTEXT *C=Opaque;int S=C->Translate(C->Ctx,Page,Write,Par);ENUMA_U32 A,P,Q;
  if(S!=1||(*Par&1)) { return S; }
  A=(ENUMA_U32)(*Par>>56);P=EnumaCpuPolicyNibble(A>>4);Q=EnumaCpuPolicyNibble(A&15);
  if(P!=Q||P==5) { C->Mixed=1; }
  if(!C->Policy) { C->Policy=P; } else if(C->Policy!=P) { C->Mixed=1; }
  *Par=(*Par&0x00ffffffffffffffULL)|0xbb00000000000000ULL;
  return S;
}
static inline ENUMA_U32 EnumaCpuWholePolicy(const ENUMA_INHERITED_FRAME *F,ENUMA_CPU_TRANSLATE Translate,void *Ctx) {
  ENUMA_CPU_POLICY_CONTEXT C={Translate,Ctx,0,0};
  if(!Translate||EnumaCpuStage1(F,EnumaCpuPolicyCapture,&C)!=5) { return 1; }
  if(C.Mixed||C.Policy<2||C.Policy>4) { return 5; }
  return C.Policy;
}
// Separate data-read follow-up: exactly one aligned word, no cache operation,
// no data export and no clearing of requirements. A successful AT is not data.
typedef int (*ENUMA_CPU_WORD_READ)(void *,ENUMA_U64,ENUMA_U32 *);
static inline ENUMA_U32 EnumaCpuReadOnce(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Policy,ENUMA_CPU_WORD_READ Read,void *Ctx) {
  ENUMA_U32 Value=0;
  if(Policy!=3) { return Policy==2||Policy==4?2:Policy==5?3:1; }
  if(!F||!Read||F->Requirements!=22||F->Width!=1600||F->Height!=2560||
     F->Stride<6400||(F->Stride&3)||F->Bytes!=(ENUMA_U64)F->Stride*F->Height||
     (F->Base&3)||F->Base<0x9c000000ULL||F->Base>=0x9e400000ULL||
     !F->Bytes||F->Bytes>0x9e400000ULL-F->Base) { return 4; }
  if(Read(Ctx,F->Base,&Value)!=1) { return 4; }
  (void)Value;return 5;
}
typedef int (*ENUMA_CPU_BARRIER)(void *);
typedef struct { ENUMA_CPU_WORD_READ Read;ENUMA_CPU_BARRIER Barrier;void *Ctx;ENUMA_U32 Called; } ENUMA_CPU_BARRIER_CONTEXT;
static inline int EnumaCpuBarrierRead(void *Opaque,ENUMA_U64 Address,ENUMA_U32 *Value) {
  ENUMA_CPU_BARRIER_CONTEXT *C=Opaque;
  if(C->Called) { return 0; } C->Called=1;
  if(C->Barrier(C->Ctx)!=1) { return 0; }
  return C->Read(C->Ctx,Address,Value);
}
static inline ENUMA_U32 EnumaCpuReadWithBarrier(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Policy,ENUMA_CPU_WORD_READ Read,ENUMA_CPU_BARRIER Barrier,void *Ctx) {
  ENUMA_CPU_BARRIER_CONTEXT C={Read,Barrier,Ctx,0};
  if(!Read||!Barrier) { return 4; }
  return EnumaCpuReadOnce(F,Policy,EnumaCpuBarrierRead,&C);
}
// The first word reuses every existing gate and the one pre-read barrier.
// Remaining1023 words cover only128x8 at the decoded origin, skipping padding.
static inline ENUMA_U32 EnumaCpuReadRoi(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Policy,ENUMA_U64 El,ENUMA_CPU_WORD_READ Read,ENUMA_CPU_BARRIER Barrier,void *Ctx) {
  ENUMA_U32 S,X,Y,Value;
  if(El!=4&&El!=8) { return 1; }
  S=EnumaCpuReadWithBarrier(F,Policy,Read,Barrier,Ctx);
  if(S!=5) { return S==1?1:S==2?2:3; }
  for(Y=0;Y<8;Y++) { for(X=0;X<128;X++) {
    if(!X&&!Y) { continue; }
    if(Read(Ctx,F->Base+(ENUMA_U64)Y*F->Stride+(ENUMA_U64)X*4,&Value)!=1) { return 3; }
  } }
  return El==4?4:5;
}
#ifdef __aarch64__
typedef struct { ENUMA_U64 El;ENUMA_U32 Count; } ENUMA_CPU_STAGE1_CONTEXT;
static int EnumaCpuStage1Translate(void *Opaque,ENUMA_U64 Page,ENUMA_U32 Write,ENUMA_U64 *Par) {
  ENUMA_CPU_STAGE1_CONTEXT *C=Opaque;ENUMA_U64 SavedPar,SavedDaif;
  if(!C||!Par||(C->El!=4&&C->El!=8)||(Page&4095)||Page<0x9c000000ULL||
     Page>=0x9e400000ULL||Write>1||C->Count>=ENUMA_CPU_MAX_TRANSLATIONS) { return 0; }
  C->Count++;
  // Preserve interrupt state and PAR; keep each AT/result pair indivisible on
  // this CPU. No pixel load/store, TLBI, cache maintenance or table setter.
  __asm__ volatile("mrs %0, daif\nmsr daifset, #15\nmrs %1, par_el1" : "=r"(SavedDaif),"=r"(SavedPar) :: "memory");
  if(C->El==4) {
    if(Write) { __asm__ volatile("at s1e1w, %1\nisb\nmrs %0, par_el1" : "=r"(*Par) : "r"(Page) : "memory"); }
    else { __asm__ volatile("at s1e1r, %1\nisb\nmrs %0, par_el1" : "=r"(*Par) : "r"(Page) : "memory"); }
  } else {
    if(Write) { __asm__ volatile("at s1e2w, %1\nisb\nmrs %0, par_el1" : "=r"(*Par) : "r"(Page) : "memory"); }
    else { __asm__ volatile("at s1e2r, %1\nisb\nmrs %0, par_el1" : "=r"(*Par) : "r"(Page) : "memory"); }
  }
  __asm__ volatile("msr par_el1, %0\nmsr daif, %1" :: "r"(SavedPar),"r"(SavedDaif) : "memory");
  return 1;
}
static ENUMA_U32 EnumaCpuStage1ArmDetailed(const ENUMA_INHERITED_FRAME *F,ENUMA_U64 *ElOut) {
  ENUMA_CPU_STAGE1_CONTEXT C={0};ENUMA_U64 Sctlr,Tcr,Mmfr,Hcr;
  __asm__ volatile("mrs %0, CurrentEL\nmrs %1, id_aa64mmfr0_el1" : "=r"(C.El),"=r"(Mmfr));
  if(ElOut) { *ElOut=C.El; }
  if((Mmfr&15)>5) { return 1; } // bounded pre-LPA2 PAR layout, <=48-bit PA
  if(C.El==4) {
    __asm__ volatile("mrs %0, sctlr_el1\nmrs %1, tcr_el1" : "=r"(Sctlr),"=r"(Tcr));
    if(Tcr&((1ULL<<39)|(1ULL<<40))) { return 1; } // no hardware AF/dirty writes
  } else if(C.El==8) {
    __asm__ volatile("mrs %0, hcr_el2\nmrs %1, sctlr_el2\nmrs %2, tcr_el2" : "=r"(Hcr),"=r"(Sctlr),"=r"(Tcr));
    if((Hcr&(1ULL<<34))||(Tcr&((1ULL<<21)|(1ULL<<22)))) { return 1; } // exclude VHE, HA/HD
  } else { return 1; }
  if(!(Sctlr&1)) { return 1; }
#if defined(ENUMA_NATIVE_ROUTE_CPU_POLICY) && ENUMA_NATIVE_ROUTE_CPU_POLICY
  return EnumaCpuWholePolicy(F,EnumaCpuStage1Translate,&C);
#elif defined(ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE) && ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE
  return EnumaCpuAttribute(F,EnumaCpuStage1Translate,&C);
#else
  return EnumaCpuStage1(F,EnumaCpuStage1Translate,&C);
#endif
}
static inline ENUMA_U32 EnumaCpuStage1Arm(const ENUMA_INHERITED_FRAME *F) { return EnumaCpuStage1ArmDetailed(F,0); }
#if defined(ENUMA_NATIVE_ROUTE_CPU_READ) && ENUMA_NATIVE_ROUTE_CPU_READ && (!defined(ENUMA_NATIVE_ROUTE_CPU_PATTERN) || !ENUMA_NATIVE_ROUTE_CPU_PATTERN)
typedef struct { ENUMA_U64 Expected;ENUMA_U32 Reads;volatile ENUMA_U32 Sink;ENUMA_U32 Barriers,Stride; } ENUMA_CPU_WORD_CONTEXT;
static int EnumaCpuWordArm(void *Opaque,ENUMA_U64 Address,ENUMA_U32 *Value) {
  ENUMA_CPU_WORD_CONTEXT *C=Opaque;
  ENUMA_U64 Expected;
  if(!C) { return 0; }
#if defined(ENUMA_NATIVE_ROUTE_CPU_ROI) && ENUMA_NATIVE_ROUTE_CPU_ROI
  if(C->Reads>=1024||C->Stride<6400||(C->Stride&3)) { return 0; }
  Expected=C->Expected+(ENUMA_U64)(C->Reads/128)*C->Stride+(ENUMA_U64)(C->Reads%128)*4;
#else
  if(C->Reads) { return 0; } Expected=C->Expected;
#endif
  if(!Value||Address!=Expected||(Address&3)||
     Address<0x9c000000ULL||Address>0x9e3ffffcULL) { return 0; }
  C->Reads++;
  *Value=*(volatile ENUMA_U32 *)Address;
  C->Sink=*Value; // private CPU-stack value dependency; never exported
  return 1;
}
#if defined(ENUMA_NATIVE_ROUTE_CPU_BARRIER) && ENUMA_NATIVE_ROUTE_CPU_BARRIER
static int EnumaCpuBarrierArm(void *Opaque) {
  ENUMA_CPU_WORD_CONTEXT *C=Opaque;
  if(!C||C->Barriers) { return 0; } C->Barriers++;
  __asm__ volatile("dsb sy" ::: "memory");
  return 1;
}
#endif
static inline ENUMA_U32 EnumaCpuReadArmProbe(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Policy) {
  ENUMA_CPU_WORD_CONTEXT C={F?F->Base:0,0,0,0,F?F->Stride:0};
#if defined(ENUMA_NATIVE_ROUTE_CPU_BARRIER) && ENUMA_NATIVE_ROUTE_CPU_BARRIER
  return EnumaCpuReadWithBarrier(F,Policy,EnumaCpuWordArm,EnumaCpuBarrierArm,&C);
#else
  return EnumaCpuReadOnce(F,Policy,EnumaCpuWordArm,&C);
#endif
}
#if defined(ENUMA_NATIVE_ROUTE_CPU_ROI) && ENUMA_NATIVE_ROUTE_CPU_ROI
static ENUMA_U32 EnumaCpuReadRoiArmProbe(const ENUMA_INHERITED_FRAME *F,ENUMA_U32 Policy,ENUMA_U64 El) {
  ENUMA_CPU_WORD_CONTEXT C={F?F->Base:0,0,0,0,F?F->Stride:0};
  return EnumaCpuReadRoi(F,Policy,El,EnumaCpuWordArm,EnumaCpuBarrierArm,&C);
}
#endif
#endif
#endif
#endif
