// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_GOP_PUBLISH_H
#define ENUMA_GOP_PUBLISH_H
#include "InheritedGop.h"
#include "InheritedGopAccess.h"
typedef struct ENUMA_GOP_PUBLICATION ENUMA_GOP_PUBLICATION;
// Out must initially be NULL. Same admission/validation as EnumaGopCreate;
// never clears requirements. GOP+the BDS display device path share one new
// owned handle. No connection, SetMode, global console alias, or pixel access.
// On a non-error warning Out is retained: caller still owns live interfaces.
EFI_STATUS EnumaGopPublish(CONST ENUMA_INHERITED_FRAME *,ENUMA_GOP_VALIDATE,VOID *,ENUMA_GOP_PUBLICATION **Out);
EFI_STATUS EnumaGopPublishWtOpaque(CONST ENUMA_INHERITED_FRAME *,ENUMA_GOP_VALIDATE,VOID *,CONST ENUMA_GOP_ACCESS *,ENUMA_GOP_PUBLICATION **Out);
EFI_HANDLE EnumaGopPublicationHandle(CONST ENUMA_GOP_PUBLICATION *);
// Caller disconnects consumers first. Failure retains every object and handle;
// do not destroy callback Context/frame storage until successful unpublication.
EFI_STATUS EnumaGopUnpublish(ENUMA_GOP_PUBLICATION **);
#endif
