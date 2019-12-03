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

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/kjTree/kjTreeToStringList.h"                 // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToStringList -
//
bool kjTreeToStringList(ConnectionInfo* ciP, KjNode* kNodeP, std::vector<std::string>* stringListP)
{
  for (KjNode* attributeP = kNodeP->value.firstChildP; attributeP != NULL; attributeP = attributeP->next)
  {
    char*   expanded;

    STRING_CHECK(attributeP, "String-List item");
    expanded = orionldContextItemExpand(orionldState.contextP, attributeP->value.s, NULL, true, NULL);
    stringListP->push_back(expanded);
  }

  return true;
}
