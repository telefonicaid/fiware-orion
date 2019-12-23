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
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextValueExpand.h"           // orionldContextValueExpand
#include "orionld/kjTree/kjTreeToMetadata.h"                     // Own interface



// -----------------------------------------------------------------------------
//
// metadataAdd - from orionldAttributeTreat.cpp - needs its own module under orionld/common
//
extern bool metadataAdd(ConnectionInfo* ciP, ContextAttribute* caP, KjNode* nodeP, char* caName);



// -----------------------------------------------------------------------------
//
// kjTreeToMetadata -
//
bool kjTreeToMetadata(ConnectionInfo* ciP, ContextAttribute* caP, KjNode* nodeP, char* caName, char** detailP)
{
  //
  // Expand sub-attribute name
  //
  bool  valueMayBeExpanded  = false;

  nodeP->name = orionldContextItemExpand(orionldState.contextP, nodeP->name, &valueMayBeExpanded, true, NULL);

  if (valueMayBeExpanded == true)
    orionldContextValueExpand(nodeP);

  if (caP->metadataVector.lookupByName(nodeP->name) != NULL)
  {
    LM_E(("Duplicated attribute property '%s' for attribute '%s'", nodeP->name, caP->name.c_str()));
    orionldErrorResponseCreate(OrionldBadRequestData, "Duplicated attribute property", nodeP->name);
    *detailP = (char*) "Duplicated attribute property";
    return false;
  }

  if (metadataAdd(ciP, caP, nodeP, caName) == false)
  {
    LM_E(("Error adding metadata '%s' to attribute", nodeP->name));
    *detailP = (char*) "Error adding metadata to attribute";
    orionldErrorResponseCreate(OrionldBadRequestData, "Error adding metadata to an attribute", nodeP->name);
    return false;
  }

  return true;
}
