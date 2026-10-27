// SPDX-License-Identifier: BSD-2-Clause-Patent
// Pure saved-word coefficient analysis. Register definitions are facts from
// pinned Xiaomi 52a2f18 sde_hw_mdss.h / sde_hw_lm.c; no kernel code imported.
// Never clears BLEND or proves routing, dim/staging/LM processing, alpha unpack
// or hardware visibility. SourceOpaque is a separate caller-established premise.
#ifndef ENUMA_OPAQUE_BLEND_H
#define ENUMA_OPAQUE_BLEND_H
#include "InheritedScanout.h"
enum {EnumaOpaqueBlendUnknown,EnumaOpaqueBlendNotOpaque,EnumaOpaqueBlendFullSource};
static inline int EnumaBlendOperand(ENUMA_U32 Selector,ENUMA_U32 Constants,ENUMA_U32 SourceOpaque,ENUMA_U32 *Value) {
  if(Selector==0){*Value=(Constants>>16)&255;return 1;}
  if(Selector==1){*Value=Constants&255;return 1;}
  if(Selector==2&&SourceOpaque==1){*Value=255;return 1;}
  return 0; // background pixel alpha, or unestablished foreground pixel alpha
}
static inline ENUMA_U32 EnumaAnalyzeOpaqueBlend(ENUMA_U32 Operation,ENUMA_U32 Constants,ENUMA_U32 SourceOpaque,
  ENUMA_U32 *Foreground,ENUMA_U32 *Background) {
  ENUMA_U32 F,B;
  // Selectors and simple inversion only. Modulation, transparency, dim and
  // every unknown/reserved bit reject; do not guess their interaction.
  if(!Foreground||!Background||SourceOpaque>1||(Operation&~0x707u)||(Constants&~0x00ff00ffu)||
     !EnumaBlendOperand(Operation&3,Constants,SourceOpaque,&F)||
     !EnumaBlendOperand((Operation>>8)&3,Constants,SourceOpaque,&B)){return EnumaOpaqueBlendUnknown;}
  if(Operation&4){F=255-F;}if(Operation&0x400){B=255-B;}
  *Foreground=F;*Background=B;
  return F==255&&B==0?EnumaOpaqueBlendFullSource:EnumaOpaqueBlendNotOpaque;
}
#endif
