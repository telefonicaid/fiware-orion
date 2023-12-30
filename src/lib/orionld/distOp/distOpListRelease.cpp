/*
*
* Copyright 2022 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <curl/curl.h>                                           // curl

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/types/DistOp.h"                                // DistOp
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/distOp/distOpListRelease.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// distOpListRelease -
//
void distOpListRelease(DistOp* distOpList)
{
  DistOp* distOpP = distOpList;

  while (distOpP != NULL)
  {
    if (distOpP->curlHandle != NULL)
    {
      LM_T(LmtLeak, ("Cleaning up a curl handle at %p", distOpP->curlHandle));
      curl_easy_cleanup(distOpP->curlHandle);
      distOpP->curlHandle = NULL;
    }

    if (distOpP->curlHeaders != NULL)
    {
      curl_slist_free_all(distOpP->curlHeaders);
      distOpP->curlHeaders = NULL;
    }

    distOpP = distOpP->next;
  }

  if (orionldState.curlDoMultiP != NULL)
    curl_multi_cleanup(orionldState.curlDoMultiP);
  orionldState.curlDoMultiP = NULL;
}
