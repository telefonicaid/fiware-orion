/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>                                              // std::string
#include <vector>                                              // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "apiTypesV2/Subscription.h"                           // Subscription

#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/kjTree/kjTreeToStringList.h"                 // kjTreeToStringList
#include "orionld/kjTree/kjTreeToEndpoint.h"                   // kjTreeToEndpoint
#include "orionld/kjTree/kjTreeToNotification.h"               // Own interface



// -----------------------------------------------------------------------------
//
// formatExtract -
//
static bool formatExtract(ConnectionInfo* ciP, char* format, ngsiv2::Subscription* subP)
{
  if (SCOMPARE10(format, 'k', 'e', 'y', 'V', 'a', 'l', 'u', 'e', 's', 0))
    subP->attrsFormat = NGSI_LD_V1_KEYVALUES;
  else if (SCOMPARE11(format, 'n', 'o', 'r', 'm', 'a', 'l', 'i', 'z', 'e', 'd', 0))
    subP->attrsFormat = NGSI_LD_V1_NORMALIZED;
  else
  {
    LM_E(("Invalid value for Notification::format: '%s'", format));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for Notification::format", format, OrionldDetailsString);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToNotification -
//
bool kjTreeToNotification(ConnectionInfo* ciP, KjNode* kNodeP, ngsiv2::Subscription* subP)
{
  KjNode*   attributesP   = NULL;
  char*     formatP       = NULL;
  KjNode*   endpointP     = NULL;
  KjNode*   itemP;

  // Set default values
  subP->attrsFormat = NGSI_LD_V1_NORMALIZED;

  // Extract the info from the kjTree
  for (itemP = kNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE11(itemP->name, 'a', 't', 't', 'r', 'i', 'b', 'u', 't', 'e', 's', 0))
    {
      DUPLICATE_CHECK(attributesP, "Notification::attributes", itemP);
      ARRAY_CHECK(itemP, "Notification::attributes");

      if (kjTreeToStringList(ciP, itemP, &subP->notification.attributes) == false)
        return false;
    }
    else if (SCOMPARE7(itemP->name, 'f', 'o', 'r', 'm', 'a', 't', 0))
    {
      DUPLICATE_CHECK(formatP, "Notification::format", itemP->value.s);
      STRING_CHECK(itemP, "Notification::format");

      LM_T(LmtNotificationFormat, ("Got a subscription format: '%s'", itemP->value.s));
      if (formatExtract(ciP, formatP, subP) == false)
        return false;
      LM_T(LmtNotificationFormat, ("Extracted subscription format: %d", subP->attrsFormat));
    }
    else if (SCOMPARE9(itemP->name, 'e', 'n', 'd', 'p', 'o', 'i', 'n', 't', 0))
    {
      DUPLICATE_CHECK(endpointP, "Notification::endpoint", itemP);
      OBJECT_CHECK(itemP, "Notification::endpoint");

      if (kjTreeToEndpoint(ciP, itemP, &subP->notification.httpInfo) == false)
        return false;
    }
    else if (SCOMPARE7(itemP->name, 's', 't', 'a', 't', 'u', 's', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE10(itemP->name, 't', 'i', 'm', 'e', 's', 'S', 'e', 'n', 't', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE17(itemP->name, 'l', 'a', 's', 't', 'N', 'o', 't', 'i', 'f', 'i', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE12(itemP->name, 'l', 'a', 's', 't', 'F', 'a', 'i', 'l', 'u', 'r', 'e', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else if (SCOMPARE12(itemP->name, 'l', 'a', 's', 't', 'S', 'u', 'c', 'c', 'e', 's', 's', 0))
    {
      // Ignored field - internal field that cannot be set by requests
    }
    else
    {
      orionldErrorResponseCreate(OrionldBadRequestData, "Unknown Notification field", itemP->name, OrionldDetailsString);
      return false;
    }
  }

  return true;
}

