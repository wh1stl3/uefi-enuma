// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_OEM_EXCEPTION_PAGE_H
#define ENUMA_OEM_EXCEPTION_PAGE_H
STATIC __attribute__((noinline)) VOID ExPage(ENUMA_LOG_PAGE *P,CONST ENUMA_EXCEPTION_RECORD *R,UINT32 Page){
  UINT64 Values[3];CHAR8 Guid[36];UINT32 N=0,K=0;
  ZeroMem(P,sizeof(*P));for(UINT32 I=0;I<5;I++){for(UINT32 J=0;J<20;J++){P->Text[I][J]=' ';}P->Text[I][20]=0;}
  P->Text[0][0]='E';EnumaBootLogHex(P->Text[0]+1,0x10+Page,2);P->Text[0][3]='C';
  EnumaBootLogHex(P->Text[0]+4,Page==1?R->Type:Page==2?(R->Owner|((UINT64)R->GuidValid<<8)):R->Lr,16);
  Values[0]=Page==1?R->Pc:R->Base;Values[1]=Page==1?R->Esr:R->Rva;Values[2]=Page==1?R->Far:R->Bytes;
  for(UINT32 I=0;I<3;I++){P->Text[I+1][0]="OSC"[I];EnumaBootLogHex(P->Text[I+1]+1,Values[I],16);}
  if(Page==3){
    for(UINT32 I=1;I<4;I++){for(UINT32 J=0;J<20;J++){P->Text[I][J]=' ';}}
    EnumaBootViewGuid(Guid,&N,(CONST UINT8 *)&R->Guid);
    P->Text[1][0]=P->Text[2][0]='G';P->Text[1][1]='0';P->Text[2][1]='1';
    for(UINT32 I=0;I<N;I++){if(Guid[I]!='-'){P->Text[1+K/16][2+K%16]=Guid[I];K++;}}
    P->Text[3][0]='S';EnumaBootLogHex(P->Text[3]+1,R->Spsr,16);
  }
#if ENUMA_OEM_CONTEXT
  if(Page>=4&&Page<=6){
    UINT64 H=Page==4?R->X0:Page==5?R->Ttbr0:(R->El|(R->ProbeValid<<8));
    Values[0]=Page==4?R->X8:Page==5?R->Ttbr1:R->ParR;
    Values[1]=Page==4?R->X9:Page==5?R->Tcr:R->ParW;
    Values[2]=Page==4?R->X10:Page==5?R->Sctlr:R->X20;
    EnumaBootLogHex(P->Text[0]+4,H,16);
    for(UINT32 I=0;I<3;I++){EnumaBootLogHex(P->Text[I+1]+1,Values[I],16);}
  }
#endif
  P->Text[4][0]='G';EnumaBootLogHex(P->Text[4]+1,R->Session,16);P->Ready=TRUE;
}
#endif
