// SPDX-License-Identifier: BSD-2-Clause-Patent
// Build-bound integrity check, not adversarial authentication. Normalize only
// the exact DIR64 text relocations emitted by the verified PE generator.
#ifndef ENUMA_OEM_LOADED_IDENTITY_H
#define ENUMA_OEM_LOADED_IDENTITY_H
#include "EnumaOemBuildIdentity.h"
STATIC BOOLEAN EnumaOemLoadedIdentity(EFI_LOADED_IMAGE_PROTOCOL *L,UINTN Entry) {
  CONST UINT8 *Text;UINTN I=0,J=0,K;UINT64 Hash=14695981039346656037ULL,Value,Delta;
  if(!L||!L->ImageBase||L->ImageSize!=ENUMA_OEM_ID_SIZE||
     (UINTN)L->ImageBase>MAX_UINTN-ENUMA_OEM_ID_SIZE||
     Entry!=(UINTN)L->ImageBase+ENUMA_OEM_ID_ENTRY){return FALSE;}
  Text=(CONST UINT8 *)L->ImageBase+ENUMA_OEM_ID_TEXT_RVA;
  Delta=(UINT64)(UINTN)L->ImageBase-ENUMA_OEM_ID_BASE;
  while(I<ENUMA_OEM_ID_TEXT_SIZE){
    if(J<sizeof(EnumaOemTextRelocs)/sizeof(UINT32)&&I==EnumaOemTextRelocs[J]){
      CopyMem(&Value,Text+I,8);Value-=Delta;
      for(K=0;K<8;K++){Hash^=(UINT8)(Value>>(K*8));Hash*=1099511628211ULL;}
      I+=8;J++;
    }else{Hash^=Text[I++];Hash*=1099511628211ULL;}
  }
  return Hash==ENUMA_OEM_ID_HASH;
}
#endif
