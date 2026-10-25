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
static ENUMA_U32 EnumaCpuStage1Arm(const ENUMA_INHERITED_FRAME *F) {
  ENUMA_CPU_STAGE1_CONTEXT C={0};ENUMA_U64 Sctlr,Tcr,Mmfr,Hcr;
  __asm__ volatile("mrs %0, CurrentEL\nmrs %1, id_aa64mmfr0_el1" : "=r"(C.El),"=r"(Mmfr));
  if((Mmfr&15)>5) { return 1; } // bounded pre-LPA2 PAR layout, <=48-bit PA
  if(C.El==4) {
    __asm__ volatile("mrs %0, sctlr_el1\nmrs %1, tcr_el1" : "=r"(Sctlr),"=r"(Tcr));
    if(Tcr&((1ULL<<39)|(1ULL<<40))) { return 1; } // no hardware AF/dirty writes
  } else if(C.El==8) {
    __asm__ volatile("mrs %0, hcr_el2\nmrs %1, sctlr_el2\nmrs %2, tcr_el2" : "=r"(Hcr),"=r"(Sctlr),"=r"(Tcr));
    if((Hcr&(1ULL<<34))||(Tcr&((1ULL<<21)|(1ULL<<22)))) { return 1; } // exclude VHE, HA/HD
  } else { return 1; }
  if(!(Sctlr&1)) { return 1; }
  return EnumaCpuStage1(F,EnumaCpuStage1Translate,&C);
}
#endif
#endif
