// SPDX-License-Identifier: BSD-2-Clause-Patent
// Post-fault translation snapshot only: never reads target MMIO or walks pointers.
#ifndef ENUMA_OEM_EXCEPTION_CONTEXT_H
#define ENUMA_OEM_EXCEPTION_CONTEXT_H
STATIC __attribute__((noinline)) VOID ExContextCapture(ENUMA_EXCEPTION_RECORD *R,
    CONST EFI_SYSTEM_CONTEXT_AARCH64 *C){
  R->X0=C->X0;R->X8=C->X8;R->X9=C->X9;R->X10=C->X10;
  R->X20=C->X20;R->X21=C->X21;
  __asm__ volatile("mrs %0, CurrentEL":"=r"(R->El));
  if(R->El!=4){return;}
  __asm__ volatile("mrs %0, ttbr0_el1\nmrs %1, ttbr1_el1\nmrs %2, tcr_el1\nmrs %3, sctlr_el1"
      :"=r"(R->Ttbr0),"=r"(R->Ttbr1),"=r"(R->Tcr),"=r"(R->Sctlr));
  // Do not allow an AT probe to update AF/dirty bits. Refusal is explicit.
  if(!(R->Sctlr&1)||(R->Tcr&((1ULL<<39)|(1ULL<<40)))){return;}
  UINT64 Saved;
  __asm__ volatile("mrs %0, par_el1":"=r"(Saved));
  __asm__ volatile("at s1e1r, %1\nisb\nmrs %0, par_el1":"=r"(R->ParR):"r"(R->Far):"memory");
  __asm__ volatile("at s1e1w, %1\nisb\nmrs %0, par_el1":"=r"(R->ParW):"r"(R->Far):"memory");
  __asm__ volatile("msr par_el1, %0\nisb"::"r"(Saved):"memory");
  R->ProbeValid=1;
}
#endif
