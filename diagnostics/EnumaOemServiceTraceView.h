// SPDX-License-Identifier: BSD-2-Clause-Patent
// Immutable snapshot replay. No automatic retries, console or pixel writes.
#ifndef ENUMA_OEM_SERVICE_TRACE_VIEW_H
#define ENUMA_OEM_SERVICE_TRACE_VIEW_H
#include "EnumaOemServiceTrace.h"
#include "EnumaBootJournalView.h"
STATIC BOOLEAN EnumaServiceLogValid(CONST ENUMA_SERVICE_TRACE_LOG *L) {
  UINT32 I;CONST ENUMA_SERVICE_EVENT *E;
  if(!L||L->Head>=128||L->Count>128||L->NextSequence<L->Count||
     L->Count!=(L->NextSequence<128?L->NextSequence:128)||L->Dropped!=L->NextSequence-L->Count){return FALSE;}
  for(I=0;I<L->Count;I++){
    E=&L->Events[(L->Head+I)%128];
    if(E->Sequence!=L->NextSequence-L->Count+I||E->Call>=L->NextCall||E->Service<1||E->Service>(ENUMA_OEM_REGISTRATION?4:3)||E->Returned>1||E->HasGuid>1){return FALSE;}
  }
  return TRUE;
}
STATIC BOOLEAN EnumaServiceReplayNext(CONST ENUMA_SERVICE_TRACE_LOG *L,UINT32 *Cursor,ENUMA_BOOT_LOG_SINK Sink,VOID *Context) {
  STATIC CONST CHAR8 *Names[]={"","locate-protocol","handle-protocol","allocate-pool","install-configuration-table"};
  CHAR8 Line[256];UINT32 N=0;CONST ENUMA_SERVICE_EVENT *E;
  if(!Cursor||!Sink||!EnumaServiceLogValid(L)||*Cursor>L->Count){return FALSE;}
  if(!*Cursor){
    EnumaBootViewText(Line,&N,"OEM cached dropped=");EnumaBootViewHex(Line,&N,L->Dropped,16);
    EnumaBootViewText(Line,&N," next=");EnumaBootViewHex(Line,&N,L->NextSequence,16);
    EnumaBootViewText(Line,&N," calls=");EnumaBootViewHex(Line,&N,L->NextCall,16);
  }else{
    E=&L->Events[(L->Head+*Cursor-1)%128];
    EnumaBootViewText(Line,&N,"OEM seq=");EnumaBootViewHex(Line,&N,E->Sequence,16);
    EnumaBootViewText(Line,&N," call=");EnumaBootViewHex(Line,&N,E->Call,16);
    EnumaBootViewText(Line,&N," svc=");EnumaBootViewText(Line,&N,Names[E->Service]);
    EnumaBootViewText(Line,&N," phase=");EnumaBootViewText(Line,&N,E->Returned?"return":"enter");
    EnumaBootViewText(Line,&N," rva=");
    if(E->CallerRva==MAX_UINT64){EnumaBootViewText(Line,&N,"unknown");}else{EnumaBootViewHex(Line,&N,E->CallerRva,16);}
    EnumaBootViewText(Line,&N," s=");
    if(E->Returned){EnumaBootViewHex(Line,&N,E->Status,16);}else{EnumaBootViewText(Line,&N,"not-returned");}
    EnumaBootViewText(Line,&N," guid=");
    if(E->HasGuid){EnumaBootViewGuid(Line,&N,(CONST UINT8 *)&E->Guid);}else{EnumaBootViewText(Line,&N,"-");}
    EnumaBootViewText(Line,&N," value=");EnumaBootViewHex(Line,&N,E->Value,16);
  }
  Line[N++]='\n';Line[N]=0;
  if(!Sink(Context,Line,N)){return FALSE;}(*Cursor)++;return TRUE;
}
#endif
