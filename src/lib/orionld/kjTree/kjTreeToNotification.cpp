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
#include <string>                                              // std::string
#include <vector>                                              // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "apiTypesV2/Subscription.h"                           // Subscription

#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/kjTree/kjTreeToStringList.h"                 // kjTreeToStringList
#include "orionld/kjTree/kjTreeToEndpoint.h"                   // kjTreeToEndpoint
#include "orionld/kjTree/kjTreeToNotification.h"               // Own interface



// -----------------------------------------------------------------------------
//
// pcheckSubscriptionAcceptAndFormat - check that the 'format' and the 'accept' are compatible
//
// format                                    valid 'accept'
// ----------------------------------        ---------------------------------------------
// NGSI_V1_LEGACY                            application/json
// NGSI_V2_NORMALIZED                        application/json
// NGSI_V2_KEYVALUES                         application/json
// NGSI_V2_VALUES                            application/json
// NGSI_V2_UNIQUE_VALUES                     application/json
// NGSI_V2_CUSTOM                            application/json
// NGSI_LD_V1_NORMALIZED                     application/json, application/ld+json, application/geo+json
// NGSI_LD_V1_KEYVALUES                      application/json, application/ld+json, application/geo+json
// NGSI_LD_V1_V2_NORMALIZED                  application/json
// NGSI_LD_V1_V2_KEYVALUES                   application/json
// NGSI_LD_V1_V2_NORMALIZED_COMPACT          application/json
// NGSI_LD_V1_V2_KEYVALUES_COMPACT           application/json
//
static bool pcheckSubscriptionAcceptAndFormat(RenderFormat format, MimeType accept)
{
  switch (format)
  {
  case NGSI_V1_LEGACY:
  case NGSI_V2_NORMALIZED:
  case NGSI_V2_KEYVALUES:
  case NGSI_V2_VALUES:
  case NGSI_V2_UNIQUE_VALUES:
  case NGSI_V2_CUSTOM:
    LM_W(("Bad Input (invalid notification-format for an NGSI-LD subscription)"));
    return false;
    break;

  case NGSI_LD_V1_NORMALIZED:
  case NGSI_LD_V1_KEYVALUES:
    if ((accept != JSON) && (accept != JSONLD) && (accept != GEOJSON))
    {
      LM_W(("Bad Input (invalid notification-accept MimeType for an NGSI-LD notification) - '%s'", mimeTypeToLongString(accept)));
      return false;
    }
    return true;
    break;

  case NGSI_LD_V1_V2_NORMALIZED:
  case NGSI_LD_V1_V2_KEYVALUES:
  case NGSI_LD_V1_V2_NORMALIZED_COMPACT:
  case NGSI_LD_V1_V2_KEYVALUES_COMPACT:
    if (accept != JSON)
    {
      LM_W(("Bad Input (invalid notification-accept MimeType for a cross NGSI-LD to NGSIv2 notification) - '%s'", mimeTypeToLongString(accept)));
      return false;
    }
    return true;
    break;

  case NO_FORMAT:
    break;
  }

  LM_W(("Bad Input (unknown notification-format for an NGSI-LD subscription)"));

  return false;
}



// -----------------------------------------------------------------------------
//
// formatExtract -
//
static bool formatExtract(char* format, ngsiv2::Subscription* subP)
{
  if      (strcmp(format, "keyValues")                     == 0) subP->attrsFormat = NGSI_LD_V1_KEYVALUES;
  else if (strcmp(format, "normalized")                    == 0) subP->attrsFormat = NGSI_LD_V1_NORMALIZED;
  else if (strcmp(format, "x-ngsiv2-normalized")           == 0) subP->attrsFormat = NGSI_LD_V1_V2_NORMALIZED;
  else if (strcmp(format, "x-ngsiv2-keyValues")            == 0) subP->attrsFormat = NGSI_LD_V1_V2_KEYVALUES;
  else if (strcmp(format, "x-ngsiv2-normalized-compacted") == 0) subP->attrsFormat = NGSI_LD_V1_V2_NORMALIZED_COMPACT;
  else if (strcmp(format, "x-ngsiv2")                      == 0) subP->attrsFormat = NGSI_LD_V1_V2_NORMALIZED_COMPACT;
  else if (strcmp(format, "x-ngsiv2-keyValues-compacted")  == 0) subP->attrsFormat = NGSI_LD_V1_V2_KEYVALUES_COMPACT;
  else
  {
    LM_E(("Invalid value for Notification::format: '%s'", format));
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for Notification::format", format);
    orionldState.httpStatusCode = SccBadRequest;
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToNotification -
//
bool kjTreeToNotification(KjNode* kNodeP, ngsiv2::Subscription* subP, KjNode** endpointPP)
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

      if (kjTreeToStringList(itemP, &subP->notification.attributes) == false)
        return false;
    }
    else if (SCOMPARE7(itemP->name, 'f', 'o', 'r', 'm', 'a', 't', 0))
    {
      DUPLICATE_CHECK(formatP, "Notification::format", itemP->value.s);
      STRING_CHECK(itemP, "Notification::format");

      if (formatExtract(formatP, subP) == false)
        return false;

      if ((subP->attrsFormat == NGSI_LD_V1_V2_KEYVALUES) || (subP->attrsFormat == NGSI_LD_V1_V2_KEYVALUES_COMPACT))
      {
        LM_W(("Non-supported notification format: %s", itemP->value.s));
        orionldErrorResponseCreate(OrionldBadRequestData, "Non-supported notification format", itemP->value.s);
        orionldState.httpStatusCode = 501;
        return false;
      }
    }
    else if (SCOMPARE9(itemP->name, 'e', 'n', 'd', 'p', 'o', 'i', 'n', 't', 0))
    {
      DUPLICATE_CHECK(endpointP, "Notification::endpoint", itemP);
      OBJECT_CHECK(itemP, "Notification::endpoint");

      *endpointPP = itemP;

      if (kjTreeToEndpoint(itemP, &subP->notification.httpInfo) == false)
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
      orionldErrorResponseCreate(OrionldBadRequestData, "Unknown Notification field", itemP->name);
      return false;
    }
  }

  if (pcheckSubscriptionAcceptAndFormat(subP->attrsFormat, subP->notification.httpInfo.mimeType) == false)
  {
    LM_W(("Bad Input (Non-compatible 'format' (%s) and 'accept' (%s) fields)", renderFormatToString(subP->attrsFormat), mimeTypeToLongString(subP->notification.httpInfo.mimeType)));
    orionldErrorResponseCreate(OrionldBadRequestData, "Bad Input", "Non-compatible 'format' and 'accept' fields");
    return false;
  }

  return true;
}
