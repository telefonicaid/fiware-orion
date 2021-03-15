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

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "ngsi/ContextAttribute.h"                               // ContextAttribute
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"           // orionldSubAttributeExpand
#include "orionld/kjTree/kjTreeToMetadata.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// metadataAdd - from kjTreeToContextAttribute.cpp
//
// FIXME: needs its own module under orionld/common
//
extern bool metadataAdd(ContextAttribute* caP, KjNode* nodeP, char* caName);



// -----------------------------------------------------------------------------
//
// kjTreeToMetadata -
//
bool kjTreeToMetadata(ContextAttribute* caP, KjNode* nodeP, char* caName, char** detailP)
{
  //
  // A sub-attribute must be a JSON object (except if key-values, but that's for GET only in NGSI-LD)
  //
  if (nodeP->type != KjObject)
  {
    *detailP = (char*) "sub-attribute must be a JSON object";
    LM_E(("sub-attribute '%s' of '%s' is not a JSON object", nodeP->name, caP->name.c_str()));
    orionldErrorResponseCreate(OrionldBadRequestData, *detailP, nodeP->name);
    return false;
  }

  //
  // Expand sub-attribute name
  //
  nodeP->name = orionldSubAttributeExpand(orionldState.contextP, nodeP->name, true, NULL);

  if (caP->metadataVector.lookupByName(nodeP->name) != NULL)
  {
    LM_E(("Duplicated attribute property '%s' for attribute '%s'", nodeP->name, caP->name.c_str()));
    orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated attribute property", nodeP->name);
    *detailP = (char*) "Duplicated attribute property";
    return false;
  }

  if (metadataAdd(caP, nodeP, caName) == false)
  {
    // metadataAdd calls orionldErrorResponseCreate
    LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
    *detailP = (char*) "Error adding sub-attribute to attribute";
    return false;
  }

  return true;
}
