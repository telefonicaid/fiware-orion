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
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi/ContextAttribute.h"                               // ContextAttribute

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"           // orionldSubAttributeExpand
#include "orionld/legacyDriver/metadataAdd.h"                    // metadataAdd
#include "orionld/legacyDriver/kjTreeToMetadata.h"               // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToMetadata -
//
bool kjTreeToMetadata(ContextAttribute* caP, KjNode* nodeP, char* attributeName, char** detailP)
{
  //
  // A sub-attribute must be a JSON object (except if key-values, but that's for GET only in NGSI-LD  - for now ...)
  // Exceptions are:
  //   - createdAt (to be ignored)
  //   - modifiedAt (to be ignored)
  //   - observedAt (should ne made an exception but doesn't seem necessary right now ...)
  //   - unitCode    -"-
  //   - datasetId   -"-
  //
  if ((strcmp(nodeP->name, "createdAt") == 0) || (strcmp(nodeP->name, "modifiedAt") == 0))
    return true;  // OK, just ignored

  //
  // About these last three (observedAt, unitCode, datasetId) ...
  // My gut tells me I should implement special cases for the three but all tests are working ...
  // And I'm using all three of them!
  //
  // I'll probably have to revisit this soon enough ...
  //

  if (nodeP->type != KjObject)
  {
    *detailP = (char*) "sub-attribute must be a JSON object";
    orionldError(OrionldBadRequestData, *detailP, nodeP->name, 400);
    return false;
  }

  //
  // Expand sub-attribute name
  //
  nodeP->name = orionldSubAttributeExpand(orionldState.contextP, nodeP->name, true, NULL);

  if (caP->metadataVector.lookupByName(nodeP->name) != NULL)
  {
    orionldError(OrionldBadRequestData, "Duplicated attribute property", nodeP->name, 400);
    *detailP = (char*) "Duplicated attribute property";
    return false;
  }

  if (metadataAdd(caP, nodeP, attributeName) == false)
  {
    // metadataAdd calls orionldError
    *detailP = (char*) "Error adding sub-attribute to attribute";
    return false;
  }

  return true;
}
