/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
#include "kjson/kjBuilder.h"                                    // kjChildRemove
}

#include "logMsg/logMsg.h"                                      // LM_*

#include "orionld/types/OrionldRenderFormat.h"                  // OrionldRenderFormat
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/context/orionldAttributeExpand.h"             // orionldAttributeExpand
#include "orionld/payloadCheck/PCHECK.h"                        // PCHECK_*
#include "orionld/payloadCheck/fieldPaths.h"                    // SubscriptionNotificationPath, ...
#include "orionld/payloadCheck/pcheckEndpoint.h"                // pCheckEndpoint
#include "orionld/payloadCheck/pCheckNotification.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pCheckNotification -
//
bool pCheckNotification
(
  KjNode*               notificationP,
  bool                  patch,
  KjNode**              uriPP,
  KjNode**              notifierInfoPP,
  bool*                 mqttChangeP,
  KjNode**              showChangesOutP,
  KjNode**              sysAttrsOutP,
  OrionldRenderFormat*  renderFormatP
)
{
  KjNode* attributesP  = NULL;
  KjNode* formatP      = NULL;
  KjNode* endpointP    = NULL;
  KjNode* showChangesP = NULL;
  KjNode* sysAttrsP    = NULL;

  PCHECK_OBJECT(notificationP, 0, NULL, SubscriptionNotificationPath, 400);
  PCHECK_OBJECT_EMPTY(notificationP, 0, NULL, SubscriptionNotificationPath, 400);

  KjNode* nItemP = notificationP->value.firstChildP;
  KjNode* next;
  while (nItemP != NULL)
  {
    next = nItemP->next;

    if (strcmp(nItemP->name, "attributes") == 0)
    {
      PCHECK_DUPLICATE(attributesP, nItemP, 0, NULL, SubscriptionNotificationAttributesPath, 400);
      PCHECK_ARRAY(nItemP, 0, NULL, SubscriptionNotificationAttributesPath, 400);
      PCHECK_ARRAY_EMPTY(nItemP, 0, NULL, SubscriptionNotificationAttributesPath, 400);

      for (KjNode* attrP = nItemP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        PCHECK_STRING(attrP, 0, NULL, SubscriptionNotificationAttributesItemPath, 400);
        attrP->value.s = orionldAttributeExpand(orionldState.contextP, attrP->value.s, true, NULL);
      }
    }
    else if (strcmp(nItemP->name, "format") == 0)
    {
      PCHECK_DUPLICATE(formatP, nItemP, 0, NULL, SubscriptionNotificationFormatPath, 400);
      PCHECK_STRING(formatP, 0, NULL, SubscriptionNotificationFormatPath, 400);
      PCHECK_STRING_EMPTY(formatP, 0, NULL, SubscriptionNotificationFormatPath, 400);

      OrionldRenderFormat rf = renderFormat(formatP->value.s);
      if ((rf == RF_NONE) || (rf == RF_LEGACY))
      {
        orionldError(OrionldBadRequestData, "Invalid value for 'Subscription::notification::format'", formatP->value.s, 400);
        return false;
      }
      *renderFormatP = rf;
    }
    else if (strcmp(nItemP->name, "endpoint") == 0)
    {
      PCHECK_DUPLICATE(endpointP, nItemP, 0, NULL, SubscriptionNotificationEndpointPath, 400);
      PCHECK_OBJECT(endpointP, 0, NULL, SubscriptionNotificationEndpointPath, 400);
      PCHECK_OBJECT_EMPTY(endpointP, 0, NULL, SubscriptionNotificationEndpointPath, 400);

      if (pcheckEndpoint(endpointP, patch, uriPP, notifierInfoPP, mqttChangeP) == false)
        return false;
    }
    else if (strcmp(nItemP->name, "showChanges") == 0)
    {
      PCHECK_DUPLICATE(showChangesP, nItemP, 0, NULL, SubscriptionNotificationShowChangesPath, 400);
      PCHECK_BOOL(showChangesP, 0, NULL, SubscriptionNotificationShowChangesPath, 400);
      *showChangesOutP = showChangesP;
    }
    else if (strcmp(nItemP->name, "sysAttrs") == 0)
    {
      PCHECK_DUPLICATE(sysAttrsP, nItemP, 0, NULL, SubscriptionNotificationSysAttrsPath, 400);
      PCHECK_BOOL(sysAttrsP, 0, NULL, SubscriptionNotificationSysAttrsPath, 400);
      *sysAttrsOutP = sysAttrsP;
      LM_T(LmtSysAttrs, ("Found a 'sysAttrs' in Subscription::notification (%s)", (sysAttrsP->value.b == true)? "true" : "false"));
    }
    else if ((strcmp(nItemP->name, "status")           == 0) ||
             (strcmp(nItemP->name, "timesSent")        == 0) ||
             (strcmp(nItemP->name, "timesFailed")      == 0) ||
             (strcmp(nItemP->name, "lastNotification") == 0) ||
             (strcmp(nItemP->name, "lastSuccess")      == 0) ||
             (strcmp(nItemP->name, "lastFailure")      == 0))
    {
      kjChildRemove(notificationP, nItemP);
    }
    else
    {
      orionldError(OrionldBadRequestData, "Invalid field for Subscription::notification", nItemP->name, 400);
      return false;
    }

    nItemP = next;
  }

  if ((endpointP == NULL) && (patch == false))
  {
    orionldError(OrionldBadRequestData, "Mandatory field missing", SubscriptionNotificationEndpointPath, 400);
    return false;
  }

  if ((formatP != NULL) && (sysAttrsP != NULL))
  {
    if ((sysAttrsP->value.b == true) && (*renderFormatP == RF_KEYVALUES))
    {
      orionldError(OrionldBadRequestData, "Inconsistent fields in Subscription (format=simplified + sysAttrs=true)", SubscriptionNotificationSysAttrsPath, 400);
      return false;
    }
  }

  return true;
}
