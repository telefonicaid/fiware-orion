/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/db/dbEntityAttributesGet.h"                    // dbEntityAttributesGet
#include "orionld/context/orionldAttributeExpand.h"              // orionldAttributeExpand
#include "orionld/serviceRoutines/orionldGetEntityAttribute.h"   // Own Interface



// ----------------------------------------------------------------------------
//
// orionldGetEntityAttribute -
//
bool orionldGetEntityAttribute(ConnectionInfo* ciP)
{
  OrionldProblemDetails  pd;

  char* attrLongName        = orionldAttributeExpand(orionldState.contextP, orionldState.wildcard[0], true, NULL);
  orionldState.responseTree = dbEntityAttributesGet(&pd, attrLongName, true);

  if (orionldState.responseTree == NULL)
  {
    // dbEntityAttributesGet calls orionldErrorResponseCreate
    LM_E(("dbEntityAttributesGet: %s: %s", pd.title, pd.detail));
    return false;
  }

  return true;
}
